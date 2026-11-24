// The hardware test: a row on home for each part of the board, and a screen that exercises it — the glass on either
// controller, touch, the buttons and buzzer, the sensors, the battery and power, the card, the microphone. It is a
// bench tool for V1.0 and V2.0 boards, drawn from DESIGN.md's components. Its fixtures' sizes, its timers, the
// refresh benchmark's animation, the page walk, and the type scale and UI kit pages that scroll by pixel are DESIGN.md's
// one registered exception (DESIGN → Exceptions); the widgets on the kit page still turn pages.

#include <algorithm>
#include <array>
#include <atomic>
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <memory>
#include <string>

#include "app_hooks.h"
#include "board.h"
#include "clock_rules.h"
#include "design_tokens.h"
#include "devices.h"
#include "sounds.h"
#include "esp_system.h"
#include "lvgl_display.h"
#include "router/router.h"
#include "ui.h"
#include "widgets/widgets.h"
#include "esp_app_desc.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_psram.h"
#include "esp_timer.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

namespace hw_test {
namespace {

namespace dt = design_tokens;
namespace clock_rules = hw_test::clock_rules;
using ui::Type;
constexpr const char *kTag = "hw_test";

// Each test screen is a route on the router's stack: home's rows push one, Display pushes its sub-screens over itself, and Back
// and holding OK pop, focus back on the control that opened the screen (APP_ARCHITECTURE §7). Only the top screen runs a timer.
lv_timer_t *s_timer = nullptr;  // the top screen's self-update, if it has one
bool s_bench_running = false;   // a benchmark run is cycling the presets: leaving it puts the default back

// --- Navigation -----------------------------------------------------------------------------------------------

void StopTimer()
{
    if (s_timer != nullptr) {
        lv_timer_delete(s_timer);
        s_timer = nullptr;
    }
}

// Every test screen's OnExit — covered by another or popped: its timer stops, the buttons stop reporting to it, and a benchmark
// cut short leaves the default preset, not the last one it tried.
void LeaveScreen(void * = nullptr)
{
    StopTimer();
    lvgl_display::SetKeyObserver(nullptr);
    if (s_bench_running) {
        s_bench_running = false;
        lvgl_display::SetTuning(lvgl_display::DefaultTuning());
    }
}

// Back, and a sub-screen's Display: the screen under this one, focus on what opened this.
void OnBack(lv_event_t *)
{
    router::Pop();
}

// A button that opens a screen pushes its route, named in the button's user data.
void OnPushRoute(lv_event_t *e)
{
    router::PushRoute(static_cast<const char *>(lv_event_get_user_data(e)));
}

// Starts a test screen in the router's root: the glass, its status bar, the status line's band when the screen can get news
// (`news`, DESIGN → Layout → Regions), and a content column, returned.
lv_obj_t *BeginScreen(lv_obj_t *screen, const char *title, bool news = false)
{
    ui::StyleScreen(screen);
    ui::NewStatusBar(screen, title);
    if (news) {
        widgets::MakeStatusLine(screen);
    }
    lv_obj_t *column = lv_obj_create(screen);
    lv_obj_remove_style_all(column);
    lv_obj_set_width(column, lv_pct(100));
    lv_obj_set_flex_grow(column, 1);
    lv_obj_set_flex_flow(column, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_hor(column, dt::kSpacingEdge, 0);
    lv_obj_set_style_pad_ver(column, dt::kSpacingMd, 0);
    lv_obj_set_style_pad_row(column, dt::kSpacingMd, 0);  // a ring reaches 8 px: stops sit 16 apart (DESIGN → Layout → Spacing)
    lv_obj_set_scrollable(column, false);
    return column;
}

// The footer: Back last in the focus order, after the screen's own controls, always at the bottom. Returns it, for a screen
// whose rollers show Cancel in it while one is open (widgets::MakeRollerFooter).
lv_obj_t *EndScreen(lv_obj_t *screen)
{
    lv_obj_t *footer = lv_obj_create(screen);
    lv_obj_remove_style_all(footer);
    lv_obj_set_size(footer, lv_pct(100), dt::kSpacingFooter);
    lv_obj_set_style_pad_hor(footer, dt::kSpacingEdge, 0);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollable(footer, false);
    lv_obj_t *back = widgets::MakeButton(footer, "Back", widgets::ButtonVariant::kOutline, 160);
    lv_obj_add_event_cb(back, OnBack, LV_EVENT_CLICKED, nullptr);
    return footer;
}

// A test screen as the router takes it: built by `create`, entered by `enter` when it starts something, left by LeaveScreen;
// `covered` when an overlay over it should stop what it does on its own.
std::unique_ptr<router::Screen> TestScreen(router::CreateFn create, router::LifecycleFn enter = nullptr,
                                           router::CoveredFn covered = nullptr)
{
    return router::MakeScreen(create, enter, LeaveScreen, covered);
}

// A self-updating screen's timer stops while an overlay covers it, and on uncover reads at once, then keeps its period
// (DESIGN → Motion → Waits).
void PauseWhileCovered(bool covered, void *)
{
    if (s_timer == nullptr) {
        return;
    }
    if (covered) {
        lv_timer_pause(s_timer);
    } else {
        lv_timer_resume(s_timer);
        lv_timer_ready(s_timer);
    }
}

lv_obj_t *Value(lv_obj_t *parent, const char *text)
{
    return ui::NewLabel(parent, Type::kBody, text);
}

lv_obj_t *ButtonRow(lv_obj_t *parent)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_column(row, dt::kSpacingMd, 0);  // neighbours' rings are 8 px each: 16 apart
    lv_obj_set_style_pad_row(row, dt::kSpacingMd, 0);
    lv_obj_set_scrollable(row, false);
    ui::LetRingOut(row);
    return row;
}

lv_obj_t *HalfButton(lv_obj_t *row, const char *label, bool action, lv_event_cb_t cb, void *data = nullptr)
{
    // 208 wide: a half button holds 10 characters
    const auto variant = action ? widgets::ButtonVariant::kAction : widgets::ButtonVariant::kOutline;
    lv_obj_t *button = widgets::MakeButton(row, label, variant, (dt::kSpacingContent - dt::kSpacingMd) / 2);
    if (cb != nullptr) {
        lv_obj_add_event_cb(button, cb, LV_EVENT_CLICKED, data);
    }
    return button;
}

// --- The glass ------------------------------------------------------------------------------------------------

lv_obj_t *s_display_stats = nullptr;
bool s_stats_pending = false;

const char *ModeName(lvgl_display::Refresh mode)
{
    switch (mode) {
    case lvgl_display::Refresh::kPartial: return "partial";
    case lvgl_display::Refresh::kFull: return "full";
    case lvgl_display::Refresh::kGray: return "four-grey";
    default: return "none yet";
    }
}

uint32_t s_partial_presses = 0;  // Partial's presses: each changes the stats, so each press is a small update

void FillStats()
{
    const lvgl_display::Stats s = lvgl_display::GetStats();
    char temp[16] = "unread";
    if (s.temperature_c >= 0) {
        std::snprintf(temp, sizeof(temp), "%d °C", s.temperature_c);
    }
    lv_label_set_text_fmt(s_display_stats,
                          "Last: %s, %" PRIu32 " ms\nLatch %" PRIu32 " · send %" PRIu32 " · power %" PRIu32
                          "\nDrive %" PRIu32 " · skipped %" PRIu32 "\nPartials since full: %" PRIu32 " of %d\n"
                          "Waveform temperature: %s\nPartial pressed: %" PRIu32,
                          ModeName(s.last), s.last_ms, s.latch_ms, s.stream_ms, s.power_on_ms, s.drive_ms, s.skipped,
                          s.partials, dt::kRefreshPartialLimit, temp, s_partial_presses);
}

// After a refresh the person asked for, show its time: once, so the stats' own redraw does not ask for another.
void ShowStatsOnce(lv_timer_t *)
{
    if (s_stats_pending && s_display_stats != nullptr) {
        s_stats_pending = false;
        FillStats();
    }
}

void OnFull(lv_event_t *)
{
    s_stats_pending = true;
    lvgl_display::RequestFull();
    lv_obj_invalidate(lv_screen_active());
}

// A small change, as a real update is: the stats count Partial's presses. An unchanged frame is never sent.
void OnPartial(lv_event_t *)
{
    ++s_partial_presses;
    FillStats();
    s_stats_pending = true;
}

lv_obj_t *s_tuning_button = nullptr;

void SetTuningLabel()
{
    lv_label_set_text_fmt(lv_obj_get_child(s_tuning_button, 0), "Speed: %s",
                          lvgl_display::TuningName(lvgl_display::Tuning()));
}

// Cycles the partial-refresh presets of the controller fitted; a full refresh clears what the last one left.
void OnTuning(lv_event_t *)
{
    const int next = (lvgl_display::Tuning() + 1) % lvgl_display::TuningCount();
    if (lvgl_display::SetTuning(next) == ESP_OK) {
        SetTuningLabel();
        lvgl_display::RequestFull();
        s_stats_pending = true;
    }
}

void OnPattern(lv_event_t *e)
{
    // A one-pixel checkerboard of levels 0 and 3 — a 50% ghosting fixture, not the UI's one-in-four and three-in-four
    // dithers — then the same area plain, to show ghosting after partials (STICKY_EPAPER_WAVEFORMS §2).
    lv_obj_t *box = static_cast<lv_obj_t *>(lv_event_get_user_data(e));
    static bool dithered = false;
    dithered = !dithered;
    lv_obj_set_style_bg_opa(box, dithered ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    s_stats_pending = true;
}

void DrawChecker(lv_event_t *e)
{
    lv_obj_t *box = lv_event_get_target_obj(e);
    if (lv_obj_get_style_bg_opa(box, LV_PART_MAIN) == LV_OPA_TRANSP) {
        return;
    }
    lv_layer_t *layer = lv_event_get_layer(e);
    lv_area_t area;
    lv_obj_get_content_coords(box, &area);
    lv_draw_rect_dsc_t dot;
    lv_draw_rect_dsc_init(&dot);
    dot.bg_color = ui::Level(dt::kColorBlack);
    for (int32_t y = area.y1; y <= area.y2; ++y) {
        for (int32_t x = area.x1 + ((y + area.x1) & 1); x <= area.x2; x += 2) {
            const lv_area_t px = {x, y, x, y};
            lv_draw_rect(layer, &dot, &px);
        }
    }
}

void CreateDisplay(lv_obj_t *screen, lv_group_t *, void *)
{
    lv_obj_t *content = BeginScreen(screen, "Display");
    const lvgl_display::Stats s = lvgl_display::GetStats();
    // The heading and its note are a pair: spacing.sm apart, where blocks sit spacing.md apart (DESIGN → Layout → Spacing).
    lv_obj_t *heading = lv_obj_create(content);
    lv_obj_remove_style_all(heading);
    lv_obj_set_size(heading, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(heading, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(heading, dt::kSpacingSm, 0);
    lv_obj_set_scrollable(heading, false);
    ui::LetRingOut(heading);  // not clipped: a glyph's left bearing reaches 2 px past its label, as the column lets it
    ui::NewLabel(heading, Type::kHeadline, s.ssd2677 ? "SSD2677 · V2.0" : "SSD1677 · V1.0");
    ui::NewLabel(heading, Type::kCaption,
                 s.ssd2677 ? "Matte glass. The controller answered 0x07 to its ID read."
                           : "Glossy glass. The controller did not answer the SSD2677's ID.",
                 true);
    // Six short lines in caption, so the readout, the controls and Back all fit above the footer.
    s_display_stats = ui::NewLabel(content, Type::kCaption, "");
    FillStats();

    lv_obj_t *row = ButtonRow(content);
    HalfButton(row, "Full", true, OnFull);  // a half button holds 10 characters of label (STICKY_DESIGN_TOKENS §4)
    HalfButton(row, "Partial", false, OnPartial);
    // Display's sub-screens push over it, and pop back to it with focus on the button that opened them.
    HalfButton(row, "Four greys", false, OnPushRoute, const_cast<char *>("hw.display.grays"));
    HalfButton(row, "Type scale", false, OnPushRoute, const_cast<char *>("hw.display.type-scale"));
    HalfButton(row, "UI kit", false, OnPushRoute, const_cast<char *>("hw.display.kit"));
    lv_obj_t *pattern_button = HalfButton(row, "Pattern", false, nullptr);  // in the row, before Speed below it

    if (lvgl_display::TuningCount() > 1) {
        // Full width, with its state in its words: a half button holds 10 characters and the presets are longer.
        s_tuning_button = widgets::MakeButton(content, "", widgets::ButtonVariant::kOutline, lv_pct(100));
        lv_obj_add_event_cb(s_tuning_button, OnTuning, LV_EVENT_CLICKED, nullptr);
        SetTuningLabel();
    }

    // The ghosting box: a plain outline, filled with the checkerboard each time Pattern is pressed. A fixture, not a control.
    lv_obj_t *checker = lv_obj_create(content);
    lv_obj_remove_style_all(checker);
    lv_obj_set_size(checker, lv_pct(100), 48);
    lv_obj_set_style_border_width(checker, dt::kSpacingStrokeDefault, 0);
    lv_obj_set_style_border_color(checker, ui::Level(dt::kColorOutline), 0);
    lv_obj_set_style_pad_all(checker, dt::kSpacingStrokeDefault, 0);
    lv_obj_set_style_bg_opa(checker, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(checker, DrawChecker, LV_EVENT_DRAW_MAIN_END, nullptr);
    lv_obj_add_event_cb(pattern_button, OnPattern, LV_EVENT_CLICKED, checker);
    EndScreen(screen);
}

// On top again, after a sub-screen too: the stats read again, and their once-after-a-refresh update runs.
void EnterDisplay(void *)
{
    FillStats();
    s_timer = lv_timer_create(ShowStatsOnce, 1500, nullptr);
}

void CreateGrays(lv_obj_t *screen, lv_group_t *, void *)
{
    // Four bands, black to white, drawn in two bits a pixel. The frame after it is drawn full, one bit.
    lv_obj_t *content = BeginScreen(screen, "Four greys");
    static const char *const kNames[] = {"Level 0 · #000000", "Level 1 · #555555", "Level 2 · #AAAAAA",
                                         "Level 3 · #FFFFFF"};
    for (uint8_t level = 0; level < 4; ++level) {
        lv_obj_t *band = lv_obj_create(content);
        lv_obj_remove_style_all(band);
        lv_obj_set_size(band, lv_pct(100), 112);
        lv_obj_set_style_bg_color(band, ui::Fill(level), 0);
        lv_obj_set_style_bg_opa(band, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(band, dt::kSpacingStrokeHairline, 0);
        lv_obj_set_style_border_color(band, ui::Level(dt::kColorOutline), 0);
        lv_obj_t *label = lv_label_create(band);
        lv_label_set_text(label, kNames[level]);
        lv_obj_set_style_text_font(label, ui::Font(Type::kLabel), 0);
        lv_obj_set_style_text_color(label, ui::Level(level < 2 ? dt::kColorWhite : dt::kColorBlack), 0);
        lv_obj_center(label);
    }
    ui::NewLabel(content, Type::kCaption, "Bands run black to white. Back draws the next frame in full.", true);
    lv_obj_t *row = ButtonRow(content);
    HalfButton(row, "Display", false, OnBack);
    EndScreen(screen);
}

void EnterGrays(void *)
{
    lvgl_display::RequestGray();
}

// A sample on the type scale page: a column that takes focus for UP and DOWN, ringed, and brought into view at once —
// LVGL's scroll on focus animates, which the glass would draw frame by frame (DESIGN → Exceptions).
void OnSpecimenFocused(lv_event_t *e)
{
    lv_obj_scroll_to_view(lv_event_get_target_obj(e), LV_ANIM_OFF);
}

lv_obj_t *NewSpecimen(lv_obj_t *parent)
{
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_remove_style_all(box);
    lv_obj_set_scrollable(box, false);
    lv_obj_set_size(box, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(box, dt::kSpacingSm, 0);
    lv_obj_set_style_pad_row(box, dt::kSpacingXs, 0);
    lv_obj_set_style_radius(box, dt::kRoundedLg, 0);  // no fill: its corners shape its ring
    lv_obj_add_event_cb(box, OnSpecimenFocused, LV_EVENT_FOCUSED, nullptr);
    ui::RingOutside(box);  // outside the box, so the ring never covers the sample
    lv_group_add_obj(lvgl_display::Group(), box);
    return box;
}

// The type scale: every primitive type token — Bitcount Single at one bit — with its values and the roles that use it
// (STICKY_DESIGN_TOKENS §3). The page scrolls; UP and DOWN step through the entries.
void CreateTypeScale(lv_obj_t *screen, lv_group_t *, void *)
{
    struct Primitive {
        const char *name;
        dt::TypeRole tokens;
    };
    static constexpr Primitive kScale[] = {
        {"scale-20-semibold", dt::kTypographyScale20Semibold},
        {"scale-20-bold", dt::kTypographyScale20Bold},
        {"scale-30-semibold", dt::kTypographyScale30Semibold},
        {"scale-30-bold", dt::kTypographyScale30Bold},
        {"scale-40-semibold", dt::kTypographyScale40Semibold},
        {"scale-40-bold", dt::kTypographyScale40Bold},
        {"scale-50-semibold", dt::kTypographyScale50Semibold},
        {"scale-50-bold", dt::kTypographyScale50Bold},
        {"scale-60-semibold", dt::kTypographyScale60Semibold},
        {"scale-60-bold", dt::kTypographyScale60Bold},
        {"scale-70-semibold", dt::kTypographyScale70Semibold},
        {"scale-70-bold", dt::kTypographyScale70Bold},
        {"scale-80-semibold", dt::kTypographyScale80Semibold},
        {"scale-80-bold", dt::kTypographyScale80Bold},
        {"scale-100-semibold", dt::kTypographyScale100Semibold},
        {"scale-100-bold", dt::kTypographyScale100Bold},
        {"scale-120-semibold", dt::kTypographyScale120Semibold},
        {"scale-120-bold", dt::kTypographyScale120Bold},
        {"scale-160-semibold", dt::kTypographyScale160Semibold},
        {"scale-160-bold", dt::kTypographyScale160Bold},
    };
    struct RoleName {
        const char *name;
        dt::TypeRole tokens;
    };
    static constexpr RoleName kRoles[] = {
        {"hero", dt::kTypographyHero},   {"display", dt::kTypographyDisplay}, {"headline", dt::kTypographyHeadline},
        {"body", dt::kTypographyBody},   {"label", dt::kTypographyLabel},     {"caption", dt::kTypographyCaption},
    };
    lv_obj_t *content = BeginScreen(screen, "Type scale");
    lv_obj_set_scrollable(content, true);
    lv_obj_set_scroll_dir(content, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_OFF);
    // touch-exception: on e-ink a throw or a bounce is a run of partial refreshes, an animation DESIGN.md forbids;
    // a drag moves the page and stops when it does (DESIGN → Exceptions).
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLL_MOMENTUM);  // touch-exception: no throw on the glass
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLL_ELASTIC);   // touch-exception: no bounce on the glass
    ui::NewLabel(content, Type::kCaption, "Bitcount Single, one bit a pixel. 100 px and up holds figures only.", true);
    int16_t last_size = 0;
    for (const Primitive &p : kScale) {
        const lv_font_t *font = ui::Bitcount(p.tokens.size_px, p.tokens.weight);
        if (font == nullptr) {
            continue;
        }
        if (last_size != 0 && p.tokens.size_px != last_size) {
            ui::NewDivider(content);
        }
        last_size = p.tokens.size_px;
        lv_obj_t *entry = NewSpecimen(content);
        lv_obj_t *sample = lv_label_create(entry);
        lv_label_set_text(sample, p.tokens.size_px >= 100 ? "9:41" : "Follow up with Sam");
        lv_obj_set_style_text_font(sample, font, 0);
        lv_obj_set_style_text_line_space(sample, std::max<int32_t>(0, p.tokens.line_px - lv_font_get_line_height(font)), 0);
        lv_obj_set_width(sample, lv_pct(100));
        char roles[48] = "";
        for (const RoleName &r : kRoles) {
            if (r.tokens.size_px == p.tokens.size_px && r.tokens.weight == p.tokens.weight) {
                std::snprintf(roles + std::strlen(roles), sizeof roles - std::strlen(roles), "%s%s",
                              roles[0] != '\0' ? ", " : " · ", r.name);
            }
        }
        char tokens[96];
        std::snprintf(tokens, sizeof tokens, "%s · %d / %d · %d%s", p.name, p.tokens.size_px, p.tokens.line_px,
                      p.tokens.weight, roles);
        ui::NewLabel(entry, Type::kCaption, tokens, true);
    }
    lv_obj_t *row = ButtonRow(content);
    HalfButton(row, "Display", false, OnBack);
    EndScreen(screen);
}

// --- The UI kit ----------------------------------------------------------------------------------------------

// The kit's page: every widget of DESIGN.md's kit, live, on one page that scrolls by pixel (DESIGN → Exceptions). The list
// view and the text view on it turn pages, as they do everywhere: a swipe over one is that view's, never the page's.
// A focused widget comes into view at once with its ring, by the theme's ui::ShowWhenFocused — never LVGL's animated
// scroll, which the glass would draw frame by frame.

// A slider's name, a `caption` kept spacing.md above its track (the page's gap) so the ring never touches it, and 28 px under
// the track before it (the page's gap and this margin).
lv_obj_t *KitName(lv_obj_t *content, const char *name)
{
    lv_obj_t *label = ui::NewLabel(content, Type::kCaption, name, true);
    lv_obj_set_style_margin_top(label, dt::kSpacingMd - dt::kSpacingXs, 0);
    return label;
}

// The reason a disabled control is disabled, in words under it.
void KitReason(lv_obj_t *content, const char *reason)
{
    ui::NewLabel(content, Type::kCaption, reason, true);
}

// A row of rollers `spacing.lg` apart, which is what a roller's open ring (10 px out) leaves room for, and lets the rings out.
lv_obj_t *RollerRow(lv_obj_t *parent)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(row, dt::kSpacingLg, 0);
    lv_obj_set_scrollable(row, false);
    ui::LetRingOut(row);
    return row;
}

// A column of controls that sit spacing.md apart and let their rings out.
lv_obj_t *KitColumn(lv_obj_t *content)
{
    lv_obj_t *column = lv_obj_create(content);
    lv_obj_remove_style_all(column);
    lv_obj_set_size(column, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(column, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(column, dt::kSpacingMd, 0);
    lv_obj_set_scrollable(column, false);
    ui::LetRingOut(column);
    return column;
}

void OnKitChip(lv_event_t *e)
{
    lv_obj_t *chip = lv_event_get_target_obj(e);
    lv_obj_t *row = lv_obj_get_parent(chip);
    const uint32_t n = lv_obj_get_child_count(row);
    for (uint32_t i = 0; i < n; ++i) {
        lv_obj_t *other = lv_obj_get_child(row, static_cast<int32_t>(i));
        lv_obj_set_state(other, LV_STATE_CHECKED, other == chip);
    }
}

// Step advances the progress bar by ten per cent and the step bar by a step, failing the fourth once, so every state shows.
struct KitProgress {
    widgets::ProgressBar bar;
    widgets::StepBar steps;
    lv_obj_t *dots;
    lv_obj_t *words;
    int tenths;
};
KitProgress s_kit_progress;

void ShowKitProgress()
{
    KitProgress &k = s_kit_progress;
    widgets::SetProgress(k.bar, k.tenths * 10);
    const int step = k.tenths / 2;  // 0 to 5 over the ten presses
    for (int i = 0; i < k.steps.count; ++i) {
        const widgets::StepState state = i < step                     ? widgets::StepState::kDone
                                         : i == step && k.tenths % 2   ? widgets::StepState::kRunning
                                                                       : widgets::StepState::kWaiting;
        widgets::SetStep(k.steps, i, i == 3 && step > 3 ? widgets::StepState::kFailed : state, 500);
    }
    widgets::SetPageDot(k.dots, k.tenths % 5);
    lv_label_set_text_fmt(k.words, "%d%% · step %d of 5%s · page %d of 5", k.tenths * 10, step < 5 ? step + 1 : 5,
                          step > 3 ? " · step 4 failed" : "", k.tenths % 5 + 1);
}

void OnKitStep(lv_event_t *)
{
    s_kit_progress.tenths = (s_kit_progress.tenths + 1) % 11;
    ShowKitProgress();
}

// The paged views' readouts, "Page 2 of 3", in words beside the indicator.
struct KitPaged {
    widgets::ListView list;
    widgets::TextView text;
    lv_obj_t *list_words;
    lv_obj_t *text_words;
};
KitPaged s_kit_paged;

void ShowKitPages(lv_event_t *)
{
    KitPaged &k = s_kit_paged;
    lv_label_set_text_fmt(k.list_words, "Page %d of %d", widgets::ListViewPage(k.list) + 1, widgets::ListViewPages(k.list));
    lv_label_set_text_fmt(k.text_words, "Page %d of %d", widgets::TextViewPage(k.text) + 1, widgets::TextViewPages(k.text));
}

// The kit's keyboard sample, a full-glass overlay over the kit page: what was typed, and the field — the widgets' own text or
// password input fills the keyboard's slot, and the keyboard appends, erases and reads through it (DESIGN → Exceptions: the kit
// page is a bench fixture). The kit page stays under it, so closing the keyboard comes back to the button or field that opened
// it, and the page's words are set in place.
std::string s_kit_name = "Maple";            // the plain field's text, kept for its next opening; a password is never kept
std::string s_kit_result = "Nothing typed yet.";
bool s_kit_password = false;
widgets::TextInput s_kit_input{};            // the field in the keyboard's slot while it is open
lv_obj_t *s_kit_keyboard = nullptr;          // the keyboard's screen while it is open
lv_obj_t *s_kit_result_label = nullptr;      // the kit page's line saying what happened
widgets::TextInput s_kit_named{};            // the kit page's field that shows the kept name

// The field's own button changed the text or its view behind the keyboard's back: the count line reads it again.
void OnKitFieldChanged(lv_event_t *)
{
    widgets::KeyboardTextChanged(s_kit_keyboard);
}

// The keyboard is done, and the kit page is on the glass again: what happened, in words on it.
void KitKeyboardDone(bool accepted, void *)
{
    s_kit_keyboard = nullptr;
    char words[96];
    if (!accepted) {
        std::snprintf(words, sizeof words, "Cancelled; nothing kept.");
    } else if (s_kit_password) {
        std::snprintf(words, sizeof words, "Joined with a password of %u characters.", static_cast<unsigned>(std::strlen(widgets::InputText(s_kit_input))));
    } else {
        s_kit_name = widgets::InputText(s_kit_input);
        std::snprintf(words, sizeof words, "Name is %s.", s_kit_name.c_str());
        widgets::SetInputText(s_kit_named, s_kit_name.c_str());
    }
    s_kit_result = words;
    lv_label_set_text(s_kit_result_label, s_kit_result.c_str());
}

// The kit page's OnExit: a keyboard over it was closed with no answer first (a screen change dismisses the overlays), so the
// page forgets it here as well as in KitKeyboardDone.
void LeaveKit(void *)
{
    s_kit_keyboard = nullptr;
    LeaveScreen();
}

void OnKitKeyboard(lv_event_t *e)
{
    s_kit_password = lv_event_get_user_data(e) != nullptr;
    widgets::KeyboardConfig config{};
    config.title = s_kit_password ? "Join HomeNet" : "Device name";
    config.min_length = s_kit_password ? 8 : 0;
    config.max_length = 63;
    config.action = s_kit_password ? "Join" : "Save";
    config.done = KitKeyboardDone;
    const widgets::Keyboard keyboard = widgets::OpenKeyboard(config);
    if (keyboard.screen == nullptr) {
        return;
    }
    s_kit_keyboard = keyboard.screen;
    // The real field in the keyboard's slot, the keyboard's target; the field is the walk's first stop and its button the second, and the field says when the
    // eye or the cross changed what the keyboard shows.
    s_kit_input = s_kit_password ? widgets::MakePasswordInput(keyboard.field_slot, "Type the password", dt::kSpacingContent, true)
                                 : widgets::MakeTextInput(keyboard.field_slot, "Add a name", dt::kSpacingContent, true);
    if (!s_kit_password) {
        widgets::SetInputText(s_kit_input, s_kit_name.c_str());
    }
    lv_obj_add_event_cb(s_kit_input.field, OnKitFieldChanged, LV_EVENT_VALUE_CHANGED, nullptr);
    widgets::SetKeyboardTarget(keyboard, widgets::InputKeyboardTarget(s_kit_input));
    widgets::KeyboardJoinField(keyboard, s_kit_input.field, s_kit_input.button);
}

// The kit's real dialog: a destructive one, so it opens on Cancel; what was answered, in words under its button.
lv_obj_t *s_kit_dialog_label = nullptr;

void KitDialogDone(bool accepted, void *)
{
    lv_label_set_text(s_kit_dialog_label, accepted ? "Deleted." : "Kept.");
}

void OnKitDialog(lv_event_t *)
{
    widgets::DialogConfig config{};
    config.title = "Delete this note?";
    config.message = "It goes from the device and the card.";
    config.action = "Delete note";
    config.cancel = "Cancel";
    config.destructive = true;
    config.done = KitDialogDone;
    widgets::OpenDialog(config);
}

void CreateKit(lv_obj_t *screen, lv_group_t *, void *)
{
    lv_obj_t *content = BeginScreen(screen, "UI kit");
    lv_obj_set_scrollable(content, true);
    lv_obj_set_scroll_dir(content, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_OFF);
    // touch-exception: on e-ink a throw or a bounce is a run of partial refreshes, an animation DESIGN.md forbids.
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLL_MOMENTUM);  // touch-exception: no throw on the glass
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLL_ELASTIC);   // touch-exception: no bounce on the glass

    widgets::MakeSectionHeading(content, "Buttons");
    lv_obj_t *stack = KitColumn(content);
    widgets::MakeButton(stack, "Record", widgets::ButtonVariant::kAction, dt::kSpacingButtonWidth);
    widgets::MakeButton(stack, "Later", widgets::ButtonVariant::kTonal, dt::kSpacingButtonWidth);
    widgets::MakeButton(stack, "Skip", widgets::ButtonVariant::kOutline, dt::kSpacingButtonWidth);
    widgets::MakeButton(stack, "Undo", widgets::ButtonVariant::kGhost, dt::kSpacingButtonWidth);
    lv_obj_add_state(widgets::MakeButton(stack, "Delete", widgets::ButtonVariant::kAction, dt::kSpacingButtonWidth),
                     LV_STATE_DISABLED);
    KitReason(content, "Nothing to delete");
    widgets::MakeSectionHeading(content, "Compact buttons");
    lv_obj_t *compact = ButtonRow(content);
    widgets::MakeButton(compact, "Save", widgets::ButtonVariant::kAction);
    widgets::MakeButton(compact, "Later", widgets::ButtonVariant::kTonal);
    widgets::MakeButton(compact, "Skip", widgets::ButtonVariant::kOutline);
    widgets::MakeButton(compact, "Undo", widgets::ButtonVariant::kGhost);

    // A glyph before the words: each in its button's ink, the half button's widest with one 7 characters; disabled, the
    // glyph is `text-disabled` with the words.
    widgets::MakeSectionHeading(content, "Buttons with a glyph");
    lv_obj_t *glyphs = KitColumn(content);
    widgets::MakeButton(glyphs, "Record", widgets::ButtonVariant::kAction, dt::kSpacingButtonWidth, widgets::Glyph::kMic);
    widgets::MakeButton(glyphs, "Play", widgets::ButtonVariant::kTonal, dt::kSpacingButtonWidth, widgets::Glyph::kPlay);
    widgets::MakeButton(glyphs, "Back", widgets::ButtonVariant::kOutline, dt::kSpacingButtonWidth, widgets::Glyph::kBack);
    lv_obj_add_state(widgets::MakeButton(glyphs, "Record", widgets::ButtonVariant::kAction, dt::kSpacingButtonWidth,
                                         widgets::Glyph::kMic),
                     LV_STATE_DISABLED);
    KitReason(content, "No card to record to");
    lv_obj_t *glyph_compact = ButtonRow(content);
    widgets::MakeButton(glyph_compact, "Save", widgets::ButtonVariant::kAction, 0, widgets::Glyph::kCheck);
    widgets::MakeButton(glyph_compact, "Retry", widgets::ButtonVariant::kTonal, 0, widgets::Glyph::kRefresh);
    widgets::MakeButton(glyph_compact, "Refresh", widgets::ButtonVariant::kOutline, (dt::kSpacingContent - dt::kSpacingMd) / 2,
                        widgets::Glyph::kRefresh);  // a half button's widest word beside a glyph

    // The icon button (DESIGN → Exceptions): no word, so the footer's hint says the action; the four variants, then all
    // four disabled.
    widgets::MakeSectionHeading(content, "Icon buttons");
    lv_obj_t *icons = ButtonRow(content);
    widgets::MakeIconButton(icons, widgets::IconAction::kPlay, widgets::ButtonVariant::kAction, "Play");
    widgets::MakeIconButton(icons, widgets::IconAction::kPause, widgets::ButtonVariant::kTonal, "Pause");
    widgets::MakeIconButton(icons, widgets::IconAction::kBack, widgets::ButtonVariant::kOutline, "Go back");
    widgets::MakeIconButton(icons, widgets::IconAction::kClose, widgets::ButtonVariant::kGhost, "Close");
    lv_obj_t *icons_off = ButtonRow(content);
    for (widgets::ButtonVariant variant : {widgets::ButtonVariant::kAction, widgets::ButtonVariant::kTonal,
                                           widgets::ButtonVariant::kOutline, widgets::ButtonVariant::kGhost}) {
        lv_obj_add_state(widgets::MakeIconButton(icons_off, widgets::IconAction::kStop, variant, "Stop"),
                         LV_STATE_DISABLED);
    }
    KitReason(content, "Nothing to stop");

    widgets::MakeSectionHeading(content, "Toggle rows");  // ux-exception: the kit page names the component type
    widgets::MakeToggleRow(content, "Wi-Fi", true);
    widgets::MakeToggleRow(content, "Grid", false);
    widgets::MakeToggleRow(content, "Wi-Fi sync", true);
    widgets::SetToggleRowEnabled(widgets::MakeToggleRow(content, "Background", false), false, "Turn Wi-Fi on first");
    widgets::MakeSectionHeading(content, "Toggle icon rows");  // ux-exception: the kit page names the component type
    widgets::MakeToggleIconRow(content, "Sound", true);
    widgets::MakeToggleIconRow(content, "Keys", false);
    widgets::SetToggleIconRowEnabled(widgets::MakeToggleIconRow(content, "Alerts", false), false, "Muted by the system");

    // A checkbox row is one setting among settings, a radio button row one choice of a short list: the toggle's look, the
    // control alone ringed and no state words (DESIGN → Components → The kit).
    widgets::MakeSectionHeading(content, "Checkbox rows");
    widgets::MakeCheckboxRow(content, "Keep recordings", true);
    widgets::MakeCheckboxRow(content, "Send a summary", false);
    widgets::MakeCheckboxRow(content, "Tag it automatically", true);
    widgets::SetCheckboxRowEnabled(widgets::MakeCheckboxRow(content, "Share with the team", false), false, "Sign in first");
    widgets::SetCheckboxRowEnabled(widgets::MakeCheckboxRow(content, "Back up the notes", true), false,
                                   "Turn Wi-Fi on first");
    widgets::MakeSectionHeading(content, "Radio button rows");
    // A group's name stands `group` under its heading and over the group before it (the page's gap and the margin), its rows
    // an `item` apart.
    lv_obj_t *repeat_name = ui::NewLabel(content, Type::kCaption, "Repeat", true);
    lv_obj_set_style_margin_top(repeat_name, dt::kSpacingGroup - dt::kSpacingItem, 0);
    lv_obj_t *repeat = KitColumn(content);
    lv_obj_set_style_pad_row(repeat, dt::kSpacingItem, 0);
    widgets::MakeRadioButtonRow(repeat, "Every day", true);
    widgets::MakeRadioButtonRow(repeat, "Every week", false);
    widgets::MakeRadioButtonRow(repeat, "Never", false);
    lv_obj_t *send_name = ui::NewLabel(content, Type::kCaption, "Send by", true);
    lv_obj_set_style_margin_top(send_name, dt::kSpacingGroup - dt::kSpacingItem, 0);
    lv_obj_t *send = KitColumn(content);
    lv_obj_set_style_pad_row(send, dt::kSpacingItem, 0);
    widgets::MakeRadioButtonRow(send, "Email", false);
    widgets::MakeRadioButtonRow(send, "Text message", true);
    widgets::SetRadioButtonRowEnabled(widgets::MakeRadioButtonRow(send, "Push", false), false, "Turn notifications on first");
    widgets::MakeSectionHeading(content, "Rows", widgets::HeadingX::kRowWords);
    lv_obj_t *rows = KitColumn(content);
    widgets::MakeValueRow(rows, "Language", "English (United Kingdom)");
    lv_obj_add_state(widgets::MakeValueRow(rows, "Backup", "Off").root, LV_STATE_DISABLED);
    KitReason(content, "Turn Wi-Fi on first");
    lv_obj_t *serial = widgets::MakeDetailRow(content, "Serial number", "FU-0001-2026").root;
    lv_obj_set_style_margin_top(serial, dt::kSpacingGroup - dt::kSpacingItem, 0);  // a detail row stands `group` under the rows above

    widgets::MakeSectionHeading(content, "Chips");
    lv_obj_t *chips = ButtonRow(content);
    for (const char *kind : {"All", "Idea", "Note"}) {
        lv_obj_t *chip = widgets::MakeChip(chips, kind);
        lv_obj_add_event_cb(chip, OnKitChip, LV_EVENT_CLICKED, nullptr);
    }
    lv_obj_add_state(widgets::MakeChip(chips, "To-do"), LV_STATE_DISABLED);  // a disabled chip: the dither, no edge
    lv_obj_add_state(lv_obj_get_child(chips, 0), LV_STATE_CHECKED);
    widgets::MakeSectionHeading(content, "Segment");
    static const char *const kKinds[] = {"Ideas", "To-dos", "Notes"};
    widgets::MakeSegment(content, kKinds, 3, 1);  // the page's 16 px gap puts the next track 80 px on
    lv_obj_t *kinds_off = widgets::MakeSegment(content, kKinds, 3, 1);
    widgets::SetSegmentEnabled(kinds_off, false);
    KitReason(content, "Not while recording");

    widgets::MakeSectionHeading(content, "Progress, steps and pages: Step turns each");
    s_kit_progress = {};
    s_kit_progress.bar = widgets::MakeProgressBar(content, dt::kSpacingContent, true);
    s_kit_progress.steps = widgets::MakeStepBar(content, dt::kSpacingContent, 5);
    s_kit_progress.dots = widgets::MakePageDots(content, 5, 0);
    s_kit_progress.words = ui::NewLabel(content, Type::kCaption, "", true);
    lv_obj_t *step = widgets::MakeButton(content, "Step", widgets::ButtonVariant::kOutline, dt::kSpacingButtonWidth);
    lv_obj_add_event_cb(step, OnKitStep, LV_EVENT_CLICKED, nullptr);
    s_kit_progress.tenths = 4;
    ShowKitProgress();

    widgets::MakeSectionHeading(content, "Radio rows");
    lv_obj_t *sizes = KitColumn(content);
    for (const char *size : {"Small", "Medium", "Large"}) {
        widgets::MakeRadioRow(sizes, size);  // the rows that share a parent are one group: choosing one clears the others
    }
    widgets::SetRadioChosen(lv_obj_get_child(sizes, 1), true);
    lv_obj_add_state(widgets::MakeRadioRow(sizes, "Huge"), LV_STATE_DISABLED);  // disabled: the dither, no edge, its words faint
    widgets::MakeSectionHeading(content, "Choice cards");
    ui::NewLabel(content, Type::kBody, "What is it?");
    lv_obj_t *kinds_cards = KitColumn(content);
    struct Kind {
        const char *name;
        const char *meaning;
    };
    for (const Kind &kind : {Kind{"Idea", "something to think about"}, Kind{"To-do", "something to do"},
                             Kind{"Note", nullptr}}) {
        widgets::MakeChoiceCard(kinds_cards, kind.name, false, kind.meaning);
    }
    widgets::SetChoiceChosen(lv_obj_get_child(kinds_cards, 0), true);
    lv_obj_t *keeps = KitColumn(content);
    lv_obj_t *keep = widgets::MakeChoiceCard(keeps, "Keep the recording after it is transcribed", true);
    widgets::SetChoiceChosen(keep, true);  // a card of any number flips itself
    widgets::MakeChoiceCard(keeps, "Share follow-ups with my phone", true, "Only on your Wi-Fi");
    lv_obj_add_state(widgets::MakeChoiceCard(keeps, "Back up to the card", true, "No card inserted"), LV_STATE_DISABLED);

    widgets::MakeSectionHeading(content, "Sliders: OK opens, UP raises, DOWN lowers, OK closes");
    KitName(content, "Brightness");
    widgets::MakeRangeSlider(content, 0, 100, 10, 60, "%");
    KitName(content, "Sound");
    widgets::MakeRangeSlider(content, 0, 100, 10, 40, "%");
    KitName(content, "Auto sleep");
    widgets::MakeRangeSlider(content, 0, 60, 5, 30, " min");
    KitName(content, "Volume");
    widgets::MakeRangeSlider(content, 0, 100, 10, 0, "%", "Off");
    KitName(content, "Contrast");
    widgets::MakeRangeSlider(content, 0, 100, 5, 5, "%");
    KitName(content, "Background");
    widgets::SetRangeSliderEnabled(widgets::MakeRangeSlider(content, 0, 100, 10, 40, "%"), false);
    KitReason(content, "Turn the background on first");

    s_kit_paged = {};
    KitPaged &paged = s_kit_paged;
    widgets::MakeSectionHeading(content, "List view: a swipe or DOWN turns a page", widgets::HeadingX::kRowWords);
    paged.list_words = ui::NewLabel(content, Type::kCaption, "", true);
    paged.list = widgets::MakeListView(content, 4, dt::kSpacingTarget);
    char name[16];
    for (int i = 1; i <= 12; ++i) {
        std::snprintf(name, sizeof name, "Follow-up %d", i);
        widgets::MakeValueRow(paged.list.root, name, i % 3 == 0 ? "Done" : "Open");
    }
    widgets::UpdateListView(paged.list);
    lv_obj_add_event_cb(paged.list.root, ShowKitPages, LV_EVENT_VALUE_CHANGED, nullptr);

    widgets::MakeSectionHeading(content, "Text view: OK, then UP and DOWN or a swipe");
    paged.text_words = ui::NewLabel(content, Type::kCaption, "", true);
    paged.text = widgets::MakeTextView(
        content, 6,
        "Call Sam about the venue on Thursday. Ask whether the hall holds sixty, and whether the kitchen can stay "  // ux-exception: a paragraph of sentences, each starting with a capital
        "open until ten. Send the deposit once the date is fixed, and copy Priya on the invoice. Book the van for the "
        "Saturday morning, and check the parking permit covers the loading bay. Remind the band to bring their own "
        "leads. Ask Sam for the key, and whether the stage can be set the night before.");
    lv_obj_add_event_cb(paged.text.root, ShowKitPages, LV_EVENT_VALUE_CHANGED, nullptr);
    ShowKitPages(nullptr);

    // Wi-Fi network rows: the network in use, then the networks in range as a list view three a page.
    widgets::MakeSectionHeading(content, "Wi-Fi network rows", widgets::HeadingX::kRowWords);
    widgets::MakeNetworkRow(content, "HomeNet", true, 3, widgets::NetworkState::kConnected);
    widgets::MakeNetworkRow(content, "Office", true, 2, widgets::NetworkState::kConnecting);
    lv_obj_t *networks_words = ui::NewLabel(content, Type::kCaption, "Networks in range", true);
    lv_obj_set_style_margin_top(networks_words, dt::kSpacingMd - dt::kSpacingXs, 0);
    const widgets::ListView networks = widgets::MakeListView(content, 3, dt::kSpacingListRowHeight);
    struct Network {
        const char *name;
        bool locked;
        int signal;
    };
    for (const Network &n : {Network{"Guest", false, 3}, Network{"Neighbours 5G", true, 2}, Network{"CafeNet", false, 1},
                             Network{"Pretty Fly for a Wi-Fi (guest, 5 GHz)", true, 1}, Network{"DIRECT-printer", true, 1},
                             Network{"Spare room", true, 0}}) {
        widgets::MakeNetworkRow(networks.root, n.name, n.locked, n.signal, widgets::NetworkState::kRest);
    }
    widgets::UpdateListView(networks);

    // The failed-join dialog, static: the offset in the dark dither falls down and to the right (DESIGN → Elevation & Depth).
    widgets::MakeSectionHeading(content, "Dialog");
    widgets::MakeDialog(content, "Office", "Wrong password. Check it and try again.", "Try again", "Cancel");
    lv_obj_t *dialogs = ButtonRow(content);
    HalfButton(dialogs, "Delete", false, OnKitDialog);
    s_kit_dialog_label = ui::NewLabel(content, Type::kCaption, "Opens a real dialog, on Cancel.", true);

    // Dropdowns: OK opens the select sheet over the lower part of the page. Three options, eight (one a long one, which
    // wraps), and a disabled field with its reason.
    widgets::MakeSectionHeading(content, "Dropdowns: OK opens the sheet", widgets::HeadingX::kRowWords);
    static const char *const kRepeat[] = {"Never", "Every day", "Every week"};
    static const char *const kRemind[] = {"Never", "In an hour", "This afternoon", "This evening", "Tomorrow", "In 2 days",
                                          "Next week", "On the first Monday of next month, in the morning"};
    static const char *const kAlert[] = {"A tick", "A chime"};
    widgets::MakeDropdown(content, "Repeat", kRepeat, 3, 0);
    widgets::MakeDropdown(content, "Remind me", kRemind, 8, 4);
    widgets::SetDropdownEnabled(widgets::MakeDropdown(content, "Alert", kAlert, 2, 0), false, "Turn on repeats first");

    // Rollers: Count stops at its ends, Hour goes round. OK opens one, UP and DOWN step, OK keeps, holding OK or the footer's Cancel
    // puts the value back; while one is open the footer shows Cancel and the hint in Back's place.
    widgets::MakeSectionHeading(content, "Rollers: OK opens, UP or DOWN steps, OK keeps");
    lv_obj_t *rollers = RollerRow(content);
    lv_obj_set_style_margin_top(rollers, dt::kSpacingXs, 0);
    const widgets::Roller kit_count = widgets::MakeNumberRoller(rollers, "Count", 1, 12, 1, 1, false, 1);
    const widgets::Roller kit_hour = widgets::MakeNumberRoller(rollers, "Hour", 0, 23, 1, 2, true, 0);
    lv_obj_t *roller_words = ui::NewLabel(rollers, Type::kCaption, "Count stops at 1 and 12.\nHour wraps round.", true);
    lv_obj_set_width(roller_words, 1);
    lv_obj_set_flex_grow(roller_words, 1);
    lv_obj_set_style_align(roller_words, LV_ALIGN_BOTTOM_LEFT, 0);

    // The horizontal pager: whole chips a page, dots and their count under it; UP and DOWN walk, a swipe left or right turns.
    widgets::MakeSectionHeading(content, "Horizontal pager: DOWN or a swipe turns a page");
    const widgets::HorizontalPager pager = widgets::MakeHorizontalPager(content);
    for (const char *kind : {"All", "Idea", "To-do", "Note", "Follow-up"}) {
        lv_obj_add_event_cb(widgets::MakeChip(pager.root, kind), OnKitChip, LV_EVENT_CLICKED, nullptr);
    }
    lv_obj_add_state(lv_obj_get_child(pager.root, 0), LV_STATE_CHECKED);
    widgets::UpdateHorizontalPager(pager);

    // The text input and the password input in every state: OK on the frame opens the keyboard, and the cross or the eye is the next stop.
    widgets::MakeSectionHeading(content, "Text inputs: OK opens the keyboard");
    KitName(content, "Empty: the placeholder and, on focus, the caret");
    widgets::SetInputActivateHandler(widgets::MakeTextInput(content, "Add a name"), OnKitKeyboard, nullptr);
    KitName(content, "With text: the cross clears it");
    s_kit_named = widgets::MakeTextInput(content, "Add a name");
    widgets::SetInputText(s_kit_named, s_kit_name.c_str());
    KitName(content, "Disabled");
    widgets::SetInputEnabled(widgets::MakeTextInput(content, "Add a name"), false);
    KitReason(content, "Sign in first");
    KitName(content, "Password, empty");
    widgets::SetInputActivateHandler(widgets::MakePasswordInput(content, "Type the password"), OnKitKeyboard, reinterpret_cast<void *>(1));
    KitName(content, "Password, hidden: the eye shows it");
    widgets::SetInputText(widgets::MakePasswordInput(content, "Type the password"), "homenet42");
    KitName(content, "Password, shown: the eye hides it");
    const widgets::TextInput shown = widgets::MakePasswordInput(content, "Type the password");
    widgets::SetInputText(shown, "homenet42");
    widgets::SetInputShown(shown, true);
    KitName(content, "Password, disabled");
    widgets::SetInputEnabled(widgets::MakePasswordInput(content, "Type the password"), false);
    KitReason(content, "Turn Wi-Fi on first");

    // The keyboard, its own screen: a password to join a network, and a plain name kept for the next opening.
    widgets::MakeSectionHeading(content, "Keyboard: OK opens it");
    lv_obj_t *keyboards = ButtonRow(content);
    HalfButton(keyboards, "Password", false, OnKitKeyboard, reinterpret_cast<void *>(1));
    HalfButton(keyboards, "Name", false, OnKitKeyboard, nullptr);
    s_kit_result_label = ui::NewLabel(content, Type::kCaption, s_kit_result.c_str(), true);

    widgets::MakeSectionHeading(content, "Status blocks", widgets::HeadingX::kRowWords);
    lv_obj_t *statuses = KitColumn(content);
    lv_obj_set_style_pad_row(statuses, dt::kSpacingXl, 0);
    widgets::MakeStatusBlock(statuses, "Allowed", "Your answer reached the agent.", widgets::StatusMark::kCheck);
    widgets::MakeStatusBlock(statuses, "Timed out", "before an answer came", widgets::StatusMark::kClock);
    widgets::MakeStatusBlock(statuses, "Link lost", "The bridge is not answering.", widgets::StatusMark::kLinkOff);
    widgets::MakeStatusBlock(statuses, "No follow-ups yet", "Hold AI or press Record.");

    lv_obj_t *row = ButtonRow(content);
    HalfButton(row, "Display", false, OnBack);
    lv_obj_t *footer = EndScreen(screen);
    lv_obj_t *roller_bar = widgets::MakeRollerFooter(footer);
    widgets::SetRollerFooter(kit_count, roller_bar);
    widgets::SetRollerFooter(kit_hour, roller_bar);
}

// --- Refresh benchmark: an animation ---------------------------------------------------------------------------

// A bar grows and shrinks across its track as fast as the glass can draw it, one step a frame, for each speed setting
// in turn. The frame period is measured wall to wall — render, pack, send and drive — so it is the animation's real
// rate. Each setting starts from a full refresh; the full refresh after every 20 partials is left out of the times.
constexpr int kAnimFrames = 30;        // partial frames timed per setting
constexpr int kAnimStepPx = 36;        // the bar's step: twelve to cross the track
constexpr int kAnimTrackPx = dt::kSpacingContent - 2 * dt::kSpacingStrokeDefault;  // the bar's travel, inside the track's outline

struct Anim {
    int preset = 0;
    int frames = 0;                    // timed partials so far for this setting
    bool clearing = true;              // the setting's first, full frame is on its way
    int bar = 0;
    int dir = 1;
    uint32_t seen = 0;                 // refreshes counted when the last frame was asked for
    int64_t asked_us = 0;              // when it was asked for
    uint32_t period_ms[kAnimFrames] = {};
    char results[512] = {};
};
Anim s_anim;
lv_obj_t *s_anim_live = nullptr;
lv_obj_t *s_anim_results = nullptr;
lv_obj_t *s_anim_bar = nullptr;
lv_obj_t *s_anim_run = nullptr;

uint32_t RefreshCount()
{
    const lvgl_display::Stats st = lvgl_display::GetStats();
    return st.full_count + st.partial_count + st.gray_count + st.errors + st.skipped;
}

uint32_t Median(uint32_t *v, int n)
{
    if (n <= 0) {
        return 0;
    }
    std::sort(v, v + n);
    return v[n / 2];
}

// The next frame: the bar one step on, bouncing at the track's ends.
void AnimStep()
{
    s_anim.bar += s_anim.dir * kAnimStepPx;
    if (s_anim.bar >= kAnimTrackPx) {
        s_anim.bar = kAnimTrackPx;
        s_anim.dir = -1;
    } else if (s_anim.bar <= 0) {
        s_anim.bar = 0;
        s_anim.dir = 1;
    }
    lv_obj_set_width(s_anim_bar, std::max(s_anim.bar, 1));
    s_anim.seen = RefreshCount();
    s_anim.asked_us = esp_timer_get_time();
}

void AnimStartPreset()
{
    lvgl_display::SetTuning(s_anim.preset);
    s_anim.frames = 0;
    s_anim.clearing = true;
    lv_label_set_text_fmt(s_anim_live, "%s · starting", lvgl_display::TuningName(s_anim.preset));
    lvgl_display::RequestFull();
    AnimStep();
}

void AnimFinishPreset()
{
    const uint32_t period = Median(s_anim.period_ms, s_anim.frames);
    const uint32_t centi_fps = period > 0 ? 100000 / period : 0;
    ESP_LOGI(kTag, "animation %-14s %4" PRIu32 " ms a frame · %" PRIu32 ".%02" PRIu32 " fps",
             lvgl_display::TuningName(s_anim.preset), period, centi_fps / 100, centi_fps % 100);
    const size_t used = std::strlen(s_anim.results);
    std::snprintf(s_anim.results + used, sizeof(s_anim.results) - used,
                  "%s: %" PRIu32 " ms · %" PRIu32 ".%" PRIu32 " fps\n", lvgl_display::TuningName(s_anim.preset), period,
                  centi_fps / 100, (centi_fps % 100) / 10);
}

bool s_anim_continuous = false;  // `anim` on the console: the bar runs on the current preset until `stop`

void AnimTick(lv_timer_t *)
{
    if (RefreshCount() == s_anim.seen) {
        return;  // the frame asked for is not on the glass yet
    }
    const lvgl_display::Stats st = lvgl_display::GetStats();
    const uint32_t period = static_cast<uint32_t>((esp_timer_get_time() - s_anim.asked_us) / 1000);
    if (s_anim.clearing) {
        s_anim.clearing = false;
    } else if (st.last == lvgl_display::Refresh::kPartial) {
        s_anim.period_ms[s_anim.frames++] = period;
        const uint32_t centi_fps = period > 0 ? 100000 / period : 0;
        lv_label_set_text_fmt(s_anim_live, "%s · %" PRIu32 " ms · %" PRIu32 ".%" PRIu32 " fps · frame %d of %d",
                              lvgl_display::TuningName(s_anim.preset), period, centi_fps / 100, (centi_fps % 100) / 10,
                              s_anim.frames, kAnimFrames);
    }
    if (s_anim_continuous) {
        if (s_anim.frames >= kAnimFrames) {
            s_anim.frames = 0;  // the bar runs on; the live line keeps counting a window of frames
        }
        AnimStep();
        return;
    }
    if (s_anim.frames >= kAnimFrames) {
        AnimFinishPreset();
        if (++s_anim.preset < lvgl_display::TuningCount()) {
            AnimStartPreset();
            return;
        }
        StopTimer();
        s_bench_running = false;
        lvgl_display::SetTuning(lvgl_display::DefaultTuning());
        lv_label_set_text(s_anim_live, "Done. Median frame, each setting:");
        lv_label_set_text(s_anim_results, s_anim.results);
        lv_obj_set_hidden(s_anim_run, false);
        lvgl_display::RequestFull();
        return;
    }
    AnimStep();
}

void OnAnimRun(lv_event_t *)
{
    s_anim_continuous = false;
    s_anim = Anim{};
    lv_obj_set_hidden(s_anim_run, true);
    lv_label_set_text(s_anim_results, "");
    StopTimer();
    s_bench_running = true;
    AnimStartPreset();
    s_timer = lv_timer_create(AnimTick, 5, nullptr);
}

void CreateBenchmark(lv_obj_t *screen, lv_group_t *, void *)
{
    lv_obj_t *content = BeginScreen(screen, "Refresh benchmark");
    ui::NewLabel(content, Type::kCaption,
                        "A bar grows and shrinks as fast as the glass can draw it, 30 frames for each speed setting. "
                        "Watch its edge for ghosting. Back stops the run.",
                        true);
    s_anim_live = Value(content, "Not run yet");

    // The track: an outline the width of the content; the bar inside it, black, from its left edge.
    lv_obj_t *track = lv_obj_create(content);
    lv_obj_remove_style_all(track);
    lv_obj_set_size(track, dt::kSpacingContent, 96 + 2 * dt::kSpacingStrokeDefault);
    lv_obj_set_style_border_width(track, dt::kSpacingStrokeDefault, 0);
    lv_obj_set_style_border_color(track, ui::Level(dt::kColorOutline), 0);
    lv_obj_set_scrollable(track, false);
    s_anim_bar = lv_obj_create(track);
    lv_obj_remove_style_all(s_anim_bar);
    lv_obj_set_pos(s_anim_bar, 0, 0);
    lv_obj_set_size(s_anim_bar, 1, 96);
    lv_obj_set_style_bg_color(s_anim_bar, ui::Level(dt::kColorSurfaceAction), 0);
    lv_obj_set_style_bg_opa(s_anim_bar, LV_OPA_COVER, 0);

    s_anim_results = Value(content, "");
    lv_obj_t *row = ButtonRow(content);
    s_anim_run = HalfButton(row, "Run", true, OnAnimRun);
    EndScreen(screen);
}

// --- Touch ----------------------------------------------------------------------------------------------------

lv_obj_t *s_touch_label = nullptr;
lv_obj_t *s_touch_dot = nullptr;

void OnTouch(lv_event_t *)
{
    const lvgl_display::TouchPoint t = lvgl_display::LastTouch();
    lv_obj_set_pos(s_touch_dot, t.x - dt::kSpacingLg / 2, t.y - dt::kSpacingLg / 2);
    lv_obj_set_hidden(s_touch_dot, false);
    lv_label_set_text_fmt(s_touch_label, "Raw %d, %d, on screen %d, %d", t.raw_x, t.raw_y, t.x, t.y);
}

void CreateTouch(lv_obj_t *screen, lv_group_t *, void *)
{
    lv_obj_t *content = BeginScreen(screen, "Touch");
    if (!lvgl_display::TouchReady()) {
        Value(content, "The touch panel did not answer. Check the log for the GT911.");
        EndScreen(screen);
        return;
    }
    ui::NewLabel(content, Type::kBody, "Tap each corner mark. The dot lands where you tap.");
    s_touch_label = Value(content, "No tap yet");

    // A layer over the whole screen takes the taps; the corner marks show where the edges should register.
    lv_obj_t *pad = lv_obj_create(screen);
    lv_obj_remove_style_all(pad);
    lv_obj_set_floating(pad, true);
    lv_obj_set_size(pad, lv_pct(100), lv_display_get_vertical_resolution(nullptr) - dt::kSpacingFooter);
    lv_obj_set_pos(pad, 0, 0);
    lv_obj_set_clickable(pad, true);
    lv_obj_add_event_cb(pad, OnTouch, LV_EVENT_PRESSED, nullptr);  // touch-exception: the test shows touch-down itself
    constexpr int32_t kMarkRight = dt::kSpacingEdge + dt::kSpacingContent - dt::kSpacing2xl;  // flush with the right margin
    const int32_t marks[][2] = {{dt::kSpacingEdge, 260}, {kMarkRight, 260}, {dt::kSpacingEdge, 640}, {kMarkRight, 640}};
    for (const auto &m : marks) {
        lv_obj_t *mark = lv_obj_create(pad);
        lv_obj_remove_style_all(mark);
        lv_obj_set_size(mark, dt::kSpacing2xl, dt::kSpacing2xl);
        lv_obj_set_pos(mark, m[0], m[1]);
        lv_obj_set_style_border_width(mark, dt::kSpacingStrokeDefault, 0);
        lv_obj_set_style_border_color(mark, ui::Level(dt::kColorOutline), 0);
        lv_obj_set_style_radius(mark, dt::kRoundedFull, 0);
        lv_obj_set_clickable(mark, false);
    }
    s_touch_dot = lv_obj_create(pad);
    lv_obj_remove_style_all(s_touch_dot);
    lv_obj_set_size(s_touch_dot, dt::kSpacingLg, dt::kSpacingLg);
    lv_obj_set_style_radius(s_touch_dot, dt::kRoundedFull, 0);
    lv_obj_set_style_bg_color(s_touch_dot, ui::Level(dt::kColorSurfaceAction), 0);
    lv_obj_set_style_bg_opa(s_touch_dot, LV_OPA_COVER, 0);
    lv_obj_set_hidden(s_touch_dot, true);
    lv_obj_set_clickable(s_touch_dot, false);
    EndScreen(screen);
}

// --- Buttons and buzzer ---------------------------------------------------------------------------------------

lv_obj_t *s_keys_label = nullptr;
uint32_t s_keys_seen = 0;

void OnKey(uint32_t key)
{
    const char *name = key == LV_KEY_PREV ? "UP" : key == LV_KEY_NEXT ? "DOWN" : key == LV_KEY_ENTER ? "OK" : "other";
    ++s_keys_seen;
    lv_label_set_text_fmt(s_keys_label, "Last: %s · %" PRIu32 " presses", name, s_keys_seen);
}

void OnTick(lv_event_t *)
{
    devices::Tick();
}

void OnTone(lv_event_t *)
{
    devices::Tone(1000);
}

// The sound words: each a row, the word and its sound's name as the row's words; OK or a tap plays it, cutting the one
// playing (DESIGN → Accessibility). Four rows a page, the rest a swipe or a step away (DESIGN → Layout → Paging).
constexpr int32_t kSoundRowsPerPage = 4;

void OnSoundRow(lv_event_t *e)
{
    devices::Play(static_cast<const devices::sound::NamedSound *>(lv_event_get_user_data(e))->sound);
}

void CreateButtons(lv_obj_t *screen, lv_group_t *, void *)
{
    lv_obj_t *content = BeginScreen(screen, "Buttons and buzzer");
    ui::NewLabel(content, Type::kBody, "Every press ticks. OK on a sound plays it. Hold OK for a second to go back.");
    s_keys_label = Value(content, "Last: none");
    s_keys_seen = 0;
    lv_obj_t *row = ButtonRow(content);
    HalfButton(row, "Tick", true, OnTick);
    HalfButton(row, "Tone, 1 s", false, OnTone);
    if (!devices::Found().buzzer) {
        Value(content, "The buzzer did not start. Check the log.");
    }
    const widgets::ListView sounds = widgets::MakeListView(content, kSoundRowsPerPage, dt::kSpacingTarget);
    for (const devices::sound::NamedSound &named : devices::sound::kSounds) {
        const widgets::ValueRow r = widgets::MakeValueRow(sounds.root, named.name, named.what);
        lv_obj_add_event_cb(r.root, OnSoundRow, LV_EVENT_CLICKED, const_cast<devices::sound::NamedSound *>(&named));
    }
    widgets::UpdateListView(sounds);
    EndScreen(screen);
}

void EnterButtons(void *)
{
    lvgl_display::SetKeyObserver(OnKey);
}

// --- Sensors --------------------------------------------------------------------------------------------------

lv_obj_t *s_sensors_label = nullptr;

void FillSensors(lv_timer_t * = nullptr)
{
    char ambient[64] = "SHT40: absent";
    devices::Ambient a;
    if (devices::ReadAmbient(&a) == ESP_OK) {
        std::snprintf(ambient, sizeof(ambient), "%.1f °C · %.0f%% RH", a.temperature_c, a.humidity_pct);
    }
    char motion[128] = "IMU: absent";
    devices::Motion m;
    if (devices::ReadMotion(&m) == ESP_OK) {
        std::snprintf(motion, sizeof(motion), "Accel g %.2f %.2f %.2f\nGyro dps %.1f %.1f %.1f", m.accel_g[0],
                      m.accel_g[1], m.accel_g[2], m.gyro_dps[0], m.gyro_dps[1], m.gyro_dps[2]);
    }
    char clock[48] = "RTC: absent";
    std::tm t{};
    if (devices::ReadClock(&t) == ESP_OK) {
        std::strftime(clock, sizeof(clock), "RTC %Y-%m-%d %H:%M:%S", &t);
    }
    lv_label_set_text_fmt(s_sensors_label, "%s\n%s\n%s", ambient, motion, clock);
}

void OnReadSensors(lv_event_t *)
{
    FillSensors();
}

void CreateSensors(lv_obj_t *screen, lv_group_t *, void *)
{
    lv_obj_t *content = BeginScreen(screen, "Sensors");
    s_sensors_label = Value(content, "");
    FillSensors();
    ui::NewLabel(content, Type::kCaption, "Reads again every 5 seconds.", true);
    lv_obj_t *row = ButtonRow(content);
    HalfButton(row, "Read now", true, OnReadSensors);
    EndScreen(screen);
}

void EnterSensors(void *)
{
    s_timer = lv_timer_create(FillSensors, dt::kSelfUpdateMinMs, nullptr);
}

// --- Clock ------------------------------------------------------------------------------------------------------

// "Now 08:12", read once as the screen opens and again after a set (no ticking, so no repaint runs under an open roller), the Hour
// and Minute rollers — 24-hour until a 12 or 24 hour setting exists — and Set clock, which writes them to the RTC with seconds at 0 and
// the date as read, then says so on the status line. A roller kept changes only the screen's draft, never the RTC.
struct ClockScreen {
    lv_obj_t *now = nullptr;
    widgets::Roller hour{};
    widgets::Roller minute{};
    std::tm date{};       // the date the RTC held when last read, kept for a set that finds the clock unreadable
    bool dated = false;  // false while the RTC has read nothing (never set, or absent): a set starts from 2026-01-01
};
ClockScreen s_clock;

bool ReadRtc(std::tm *out)
{
    return devices::ReadClock(out) == ESP_OK;
}

// "Now 08:12" for the RTC's reading, or "Now not set" when it reads nothing.
void ShowClockNow(const std::tm *reading)
{
    char words[24] = "Now not set";
    if (reading != nullptr) {
        clock_rules::NowWords(words, sizeof words, *reading);
    }
    lv_label_set_text(s_clock.now, words);
}

// Set clock: the chosen hour and minute, seconds 0, with the date the RTC holds as it is written (read again here, so a set after
// midnight keeps today's). The rollers are never touched: a failed set leaves what the person chose for another try.
void OnSetClock(lv_event_t *)
{
    const clock_rules::SetOutcome out = clock_rules::Set(
        ReadRtc, [](const std::tm &when) { return devices::SetClock(when) == ESP_OK; }, s_clock.dated ? &s_clock.date : nullptr,
        widgets::RollerValue(s_clock.hour), widgets::RollerValue(s_clock.minute));
    if (!out.ok) {
        router::Say(router::StatusKind::kError, "Could not set the clock");
        return;
    }
    s_clock.date = out.date;
    s_clock.dated = out.dated;
    ShowClockNow(&out.shown);
    char words[40];
    clock_rules::SetWords(words, sizeof words, out.shown);
    router::Say(router::StatusKind::kSuccess, words);
}

void CreateClock(lv_obj_t *screen, lv_group_t *, void *)
{
    lv_obj_t *content = BeginScreen(screen, "Clock", true);
    s_clock = ClockScreen{};
    s_clock.now = ui::NewLabel(content, Type::kLabel, "");
    std::tm t{};
    const bool read = ReadRtc(&t);  // once, as the screen opens
    if (read) {
        s_clock.date = t;
        s_clock.dated = true;
    }
    ShowClockNow(read ? &t : nullptr);
    lv_obj_t *rollers = RollerRow(content);
    lv_obj_set_style_margin_top(rollers, dt::kSpacingMd, 0);
    s_clock.hour = widgets::MakeNumberRoller(rollers, "Hour", 0, 23, 1, 2, true, read ? t.tm_hour : 0);
    s_clock.minute = widgets::MakeNumberRoller(rollers, "Minute", 0, 59, 1, 2, true, read ? t.tm_min : 0);
    lv_obj_t *buttons = ButtonRow(content);
    lv_obj_set_style_margin_top(buttons, dt::kSpacingLg, 0);
    HalfButton(buttons, "Set clock", true, OnSetClock);
    lv_obj_t *footer = EndScreen(screen);
    lv_obj_t *bar = widgets::MakeRollerFooter(footer);
    widgets::SetRollerFooter(s_clock.hour, bar);
    widgets::SetRollerFooter(s_clock.minute, bar);
}

// --- Welcome ----------------------------------------------------------------------------------------------------

// The carousel as onboarding would use it: four cards, one at a time; a swipe or Back and Next turn them, Skip and Done leave, and so
// does holding OK. The words are illustrative; real onboarding copy is the ux-content-writer's.
constexpr widgets::CarouselCard kWelcomeCards[] = {
    {"Say it, and it is kept", "Hold the AI button and speak. FolloUp writes down what you said."},
    {"What matters stays in view", "The follow-ups you pin stay on the glass, even while FolloUp sleeps."},
    {"Everything is on the card",
     "Your notes are kept on the MicroSD card, so they are yours to take. Lift the card out to copy them to a computer."},
    {"Ready when you are", "Press Done, then hold the AI button and say your first follow-up."},
};

void LeaveWelcome(void *)
{
    router::Pop();
}

void CreateWelcome(lv_obj_t *screen, lv_group_t *, void *)
{
    lv_obj_t *content = BeginScreen(screen, "Welcome");
    widgets::MakeCarousel(screen, content, kWelcomeCards, 4, LeaveWelcome, nullptr);
}

// --- Battery and power ----------------------------------------------------------------------------------------

lv_obj_t *s_power_label = nullptr;
lv_obj_t *s_charge_button = nullptr;

// What started the board, decided once at boot from the RTC's flag, USB and the chip's reset reason, so plugging USB in
// later never changes it (STICKY_HARDWARE_SPECS §5.2, §6).
const char *s_started_by = "restart";

void NoteStart()
{
    switch (esp_reset_reason()) {
    case ESP_RST_POWERON:  // the RTC's flag counts only here: a power-on from off is the one start it can cause
        s_started_by = devices::RtcStartedBoard() ? "RTC timer" : board::UsbPresent() ? "USB" : "OK button";
        break;
    case ESP_RST_DEEPSLEEP: s_started_by = "deep-sleep wake"; break;
    default: s_started_by = "restart"; break;
    }
    ESP_LOGI(kTag, "started by: %s", s_started_by);
}

void FillPower(lv_timer_t * = nullptr)
{
    char gauge[160] = "Gauge: absent";
    devices::Battery b;
    if (devices::ReadBattery(&b) == ESP_OK) {
        std::snprintf(gauge, sizeof(gauge), "%d%% · %d mV · %d mA\n%d of %d mAh · health %d%%\n%.1f °C · %d cycles",
                      b.percent, b.voltage_mv, b.current_ma, b.remaining_mah, b.full_mah, b.health_pct,
                      b.temperature_c, b.cycles);
    }
    const int vin = board::PowerInputMillivolts();
    lv_label_set_text_fmt(s_power_label, "%s\nUSB %s (%d mV at the pin)\nCharger %s, STAT %s\nStarted by: %s", gauge,
                          board::UsbPresent() ? "in" : "out", vin, board::ChargingEnabled() ? "on" : "off",
                          board::ChargeStatusLow() ? "low, charging" : "high", s_started_by);
}

void OnCharge(lv_event_t *)
{
    board::SetChargingEnabled(!board::ChargingEnabled());
    lv_label_set_text(lv_obj_get_child(s_charge_button, 0),
                      board::ChargingEnabled() ? "Charge off" : "Charge on");
    FillPower();
}

// How the board leaves: deep sleep, power off, or power off with the RTC set to turn it on again.
enum class LeaveBy : uintptr_t { kSleep, kPowerOff, kPowerOffTimed };
constexpr uint8_t kRtcWakeS = 60;  // DESIGN → Exceptions: a hardware-test timer; the Off, 60 s button and the
                                   // Battery page's caption spell it out too, so change all three together

// The screen the board leaves on, in place of every screen above home: its title and how it comes back.
void CreateLeave(lv_obj_t *screen, lv_group_t *, void *user)
{
    const auto how = static_cast<LeaveBy>(reinterpret_cast<uintptr_t>(user));
    const bool power_off = how != LeaveBy::kSleep;
    ui::StyleScreen(screen);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(screen, dt::kSpacingEdge, 0);
    lv_obj_set_style_pad_row(screen, dt::kSpacingPairLg, 0);  // the display over its hint
    lv_obj_t *title = ui::NewLabel(screen, Type::kDisplay, power_off ? "Off" : "Asleep");
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    char timed[64];
    std::snprintf(timed, sizeof timed, "Turns on in %u s. Hold OK to turn on now.", unsigned{kRtcWakeS});
    lv_obj_t *hint = ui::NewLabel(screen, Type::kBody,
                                  how == LeaveBy::kPowerOffTimed ? timed
                                  : power_off                    ? "Hold OK to turn on."
                                                                 : "Press OK, UP or DOWN to wake.");
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
}

// The frame left on the glass when the board leaves: drawn in full, then the panel sleeps (DESIGN → Motion).
constexpr uint32_t kSilenceSettleMs = 10;  // many of the buzzer task's 1 ms steps, so its duty 0 is out before sleep

void Leave(LeaveBy how)
{
    // A sound still playing would leave the buzzer's pad high under the hold deep sleep puts on it, the coil drawing all
    // night (STICKY_HARDWARE_SPECS §5.3); this one is cut here and the last press's tick once more below.
    devices::Silence();
    const bool power_off = how != LeaveBy::kSleep;
    // An RTC interrupt left low would power the board straight back on (STICKY_HARDWARE_SPECS §5.3); set it first, so
    // the glass promises only what is set. On USB the board restarts, and the start clears the countdown.
    if (how == LeaveBy::kPowerOffTimed && (board::UsbPresent() || devices::SetRtcWake(kRtcWakeS) != ESP_OK)) {
        how = LeaveBy::kPowerOff;
    }
    if (how != LeaveBy::kPowerOffTimed && devices::ClearRtcWake() != ESP_OK) {
        ESP_LOGE(kTag, "RTC interrupts not cleared: the board may turn on again");
    }
    // The Battery screen's OnExit stops its timer; the screens above home are let go, the board never comes back to them.
    if (router::GoTo(router::MakeScreen(CreateLeave, nullptr, nullptr, nullptr,
                                        reinterpret_cast<void *>(static_cast<uintptr_t>(how)))) != ESP_OK) {
        ESP_LOGE(kTag, "no memory for the leaving screen: the glass keeps the last one");
    }
    lvgl_display::RequestFull();
    lvgl_display::Redraw();
    const bool panel_asleep = lvgl_display::SleepPanel() == ESP_OK;
    if (!panel_asleep) {
        ESP_LOGE(kTag, "the panel did not sleep: its power stays on");
    }
    devices::Silence();  // a press during the full refresh ticked; the buzzer task wakes within a step to stop it
    vTaskDelay(pdMS_TO_TICKS(kSilenceSettleMs));
    if (power_off) {
        board::PowerOff();  // returns only on battery when the rail did not fall
        ESP_LOGW(kTag, "the rail did not fall: deep sleep instead");
        // Only the buttons wake deep sleep, so a countdown left running would mislead the next start. A timed frame
        // already on the glass keeps its "Turns on in 60 s" here: the panel is asleep, and this path needs a fault.
        if (devices::ClearRtcWake() != ESP_OK) {
            ESP_LOGE(kTag, "RTC interrupts not cleared before deep sleep");
        }
    }
    board::EnterDeepSleep(!panel_asleep);
    ESP_LOGW(kTag, "still running after deep sleep");
}

void LeaveSoon(lv_timer_t *timer)
{
    const auto how = static_cast<LeaveBy>(reinterpret_cast<uintptr_t>(lv_timer_get_user_data(timer)));
    lv_timer_delete(timer);
    Leave(how);
}

void OnSleep(lv_event_t *)
{
    // From a timer, not this click: the press has to finish before the board waits for the buttons to be released.
    lv_timer_create(LeaveSoon, 50, reinterpret_cast<void *>(LeaveBy::kSleep));
}

void OnPowerOff(lv_event_t *)
{
    lv_timer_create(LeaveSoon, 50, reinterpret_cast<void *>(LeaveBy::kPowerOff));
}

void OnPowerOffTimed(lv_event_t *)
{
    lv_timer_create(LeaveSoon, 50, reinterpret_cast<void *>(LeaveBy::kPowerOffTimed));
}

void CreatePower(lv_obj_t *screen, lv_group_t *, void *)
{
    lv_obj_t *content = BeginScreen(screen, "Battery and power");
    s_power_label = Value(content, "");
    FillPower();
    lv_obj_t *row = ButtonRow(content);
    s_charge_button = HalfButton(row, board::ChargingEnabled() ? "Charge off" : "Charge on", false, OnCharge);
    HalfButton(row, "Deep sleep", false, OnSleep);
    HalfButton(row, "Power off", true, OnPowerOff);
    HalfButton(row, "Off, 60 s", false, OnPowerOffTimed);
    ui::NewLabel(content, Type::kCaption,
                 "On USB, power off restarts the board. On battery it turns off; hold OK to turn on. On battery, "
                 "Off, 60 s turns it on again by itself.",
                 true);
    EndScreen(screen);
}

void EnterPower(void *)
{
    s_timer = lv_timer_create(FillPower, dt::kSelfUpdateMinMs, nullptr);
}

// --- Storage --------------------------------------------------------------------------------------------------

lv_obj_t *s_card_label = nullptr;

void OnProbeCard(lv_event_t *)
{
    lv_label_set_text(s_card_label, "Reading the card…");
    lv_refr_now(nullptr);  // on the glass before the mount blocks
    devices::Card card;
    const esp_err_t err = devices::ProbeCard(&card);
    // The label keeps the facts, in place; the outcome is news, said in the status line under the title, which sounds
    // success or error as its words are drawn (DESIGN → Accessibility → Every outcome in words). The words start on the outcome.
    if (!card.inserted) {
        lv_label_set_text(s_card_label, "MicroSD: no card");
        router::Say(router::StatusKind::kError, "No card. Insert one to read it.");
    } else if (err != ESP_OK) {
        lv_label_set_text_fmt(s_card_label, "MicroSD: did not mount (%s)", esp_err_to_name(err));
        router::Say(router::StatusKind::kError, "The card did not mount. Check it is FAT32, then try again.");
    } else {
        lv_label_set_text_fmt(s_card_label, "%s · %llu MiB\n%d entries in the root%s", card.name,
                              static_cast<unsigned long long>(card.capacity_mib), card.entries,
                              card.entries >= 99 ? " or more" : "");
        router::Say(router::StatusKind::kSuccess, "Card read.");
    }
}

void CreateStorage(lv_obj_t *screen, lv_group_t *, void *)
{
    lv_obj_t *content = BeginScreen(screen, "Storage", true);
    uint32_t flash = 0;
    esp_flash_get_size(nullptr, &flash);
    lv_obj_t *mem = Value(content, "");
    lv_label_set_text_fmt(mem, "Flash %" PRIu32 " MB · PSRAM %u MB\nPSRAM free %u KB",  // ux-exception: units
                          flash / (1024 * 1024),
                          static_cast<unsigned>(esp_psram_get_size() / (1024 * 1024)),
                          static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
    ui::NewDivider(content);
    s_card_label = Value(content, "MicroSD: not read yet");
    lv_obj_t *row = ButtonRow(content);
    HalfButton(row, "Read card", true, OnProbeCard);
    ui::NewLabel(content, Type::kCaption, "Mounts, reads and unmounts. It never formats the card.", true);
    EndScreen(screen);
}

// --- Microphone -----------------------------------------------------------------------------------------------

lv_obj_t *s_mic_label = nullptr;
lv_obj_t *s_mic_bar = nullptr;

// Listen records one second, between sound.recording-start and sound.recording-stop (DESIGN → Motion → Sound). The start
// sound plays at the press, cutting its tick, while "Listening…" is drawn; the microphone settles for at least as long as the
// sound has left, dropping what it hears meanwhile, so the recording never holds the buzzer. A recording that fails ends on
// the error sound instead of the stop.
void OnListen(lv_event_t *)
{
    const int64_t started_us = esp_timer_get_time();
    devices::Play(devices::sound::Word::kRecordingStart);
    lv_label_set_text(s_mic_label, "Listening…");
    lv_refr_now(nullptr);  // on the glass before the second of recording blocks
    const uint32_t played_ms = static_cast<uint32_t>((esp_timer_get_time() - started_us) / 1000);
    const uint32_t start_ms = devices::sound::LengthMs(devices::sound::SoundOf(devices::sound::Word::kRecordingStart));
    float rms = 0;
    int peak = 0;
    esp_err_t err = devices::StartListening(start_ms > played_ms ? start_ms - played_ms : 0);
    if (err == ESP_OK) {
        err = devices::ReadLevel(1000, &rms, &peak);
        devices::StopListening();
    }
    if (err != ESP_OK) {
        devices::Play(devices::sound::Word::kError);
        lv_label_set_text_fmt(s_mic_label, "The microphone did not read (%s). Check the log, then try again.",
                              esp_err_to_name(err));
        lv_obj_set_width(s_mic_bar, dt::kSpacingStrokeDefault);  // no level shown that this press did not hear
        return;
    }
    devices::Play(devices::sound::Word::kRecordingStop);
    lv_label_set_text_fmt(s_mic_label, "Level %.0f RMS · peak %d", rms, peak);
    const int32_t width = std::min<int32_t>(dt::kSpacingContent, static_cast<int32_t>(rms * dt::kSpacingContent / 4000));
    lv_obj_set_width(s_mic_bar, std::max<int32_t>(dt::kSpacingStrokeDefault, width));
}

void CreateMicrophone(lv_obj_t *screen, lv_group_t *, void *)
{
    lv_obj_t *content = BeginScreen(screen, "Microphone");
    ui::NewLabel(content, Type::kBody, "Press Listen, then speak or clap. It records one second.");  // ux-exception: Listen is the button's name
    s_mic_label = Value(content, "Not heard yet");
    s_mic_bar = lv_obj_create(content);
    lv_obj_remove_style_all(s_mic_bar);
    lv_obj_set_size(s_mic_bar, dt::kSpacingStrokeDefault, 32);
    lv_obj_set_style_bg_color(s_mic_bar, ui::Level(dt::kColorSurfaceAction), 0);
    lv_obj_set_style_bg_opa(s_mic_bar, LV_OPA_COVER, 0);
    lv_obj_t *row = ButtonRow(content);
    HalfButton(row, "Listen", true, OnListen);
    EndScreen(screen);
}

// --- Reminders ------------------------------------------------------------------------------------------------

// A screen with one task: a choice that applies at once. The rows are one group (they share a parent), so choosing one clears the
// others and says so with one change; the screen stays, the status line says what is set, and there is no Save: choosing another
// is the undo (DESIGN → Components → The kit, Radio row). Nothing is stored — this is a bench screen — so the choice lasts until
// the next boot. The status words are placeholders for the ux-content-writer.
struct Reminder {
    const char *name;
    const char *said;
};
constexpr Reminder kReminders[] = {
    {"Never", "Reminders are off."},
    {"Every day", "Reminders repeat every day."},
    {"Every week", "Reminders repeat every week."},
    {"Every two weeks, on a Monday", "Reminders repeat every two weeks, on a Monday."},
    {"Every month", "Reminders repeat every month."},
};
size_t s_reminder = 2;  // the row chosen: every week to begin with

void OnReminder(lv_event_t *e)
{
    const Reminder *chosen = static_cast<const Reminder *>(lv_event_get_user_data(e));
    s_reminder = static_cast<size_t>(chosen - kReminders);
    router::Say(router::StatusKind::kSuccess, chosen->said);
}

void CreateReminders(lv_obj_t *screen, lv_group_t *, void *)
{
    lv_obj_t *content = BeginScreen(screen, "Reminders", true);
    widgets::MakeSectionHeading(content, "How often?");
    lv_obj_t *rows = KitColumn(content);
    for (const Reminder &reminder : kReminders) {
        lv_obj_t *row = widgets::MakeRadioRow(rows, reminder.name);
        lv_obj_add_event_cb(row, OnReminder, LV_EVENT_VALUE_CHANGED, const_cast<Reminder *>(&reminder));
    }
    widgets::SetRadioChosen(lv_obj_get_child(rows, static_cast<int32_t>(s_reminder)), true);  // sends nothing: the screen opens quietly
    EndScreen(screen);
}

// --- About ----------------------------------------------------------------------------------------------------

// A fact is a name in `caption` over its value in `label` between `spacing.sm` above and below: the detail row's own height when
// each takes one line, six to a page; a longer value takes more room and the page, fewer rows (DESIGN → Layout → Paging).
constexpr int32_t kAboutRowsPerPage = 6;
int32_t AboutRowHeight()
{
    return 2 * dt::kSpacingSm + ui::LineBox(Type::kCaption) + ui::LineBox(Type::kLabel);
}

// The facts the firmware knows about itself and the board, a name over its value, as many pages as they take: no row takes the
// focus, so each page is one stop with the ring round its facts, and "1 of 2" says the page.
void CreateAbout(lv_obj_t *screen, lv_group_t *, void *)
{
    lv_obj_t *content = BeginScreen(screen, "About");
    const esp_app_desc_t *app = esp_app_get_description();
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    uint8_t mac[6] = {};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    const lvgl_display::Stats stats = lvgl_display::GetStats();
    uint32_t flash = 0;
    esp_flash_get_size(nullptr, &flash);

    const widgets::ListView list =
        widgets::MakeListView(content, kAboutRowsPerPage, AboutRowHeight(), widgets::IndicatorCentre::kView, widgets::PageWords::kUnder);
    char built[40], address[24], flash_size[16], psram_size[16], chip_rev[40];
    std::snprintf(built, sizeof built, "%s %s", app->date, app->time);
    std::snprintf(address, sizeof address, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    std::snprintf(flash_size, sizeof flash_size, "%" PRIu32 " MB", flash / (1024 * 1024));  // ux-exception: units
    std::snprintf(psram_size, sizeof psram_size, "%u MB", static_cast<unsigned>(esp_psram_get_size() / (1024 * 1024)));  // ux-exception: units
    std::snprintf(chip_rev, sizeof chip_rev, "%s, revision %d.%d", CONFIG_IDF_TARGET, chip.revision / 100, chip.revision % 100);
    const struct {
        const char *name;
        const char *value;
    } facts[] = {
        {"Product", app->project_name},
        {"Firmware", app->version},
        {"Built", built},
        {"Board", stats.ssd2677 ? "reTerminal Sticky V2.0" : "reTerminal Sticky V1.0"},
        {"Panel", stats.ssd2677 ? "SSD2677, 480 x 800" : "SSD1677, 480 x 800"},
        {"Chip", chip_rev},
        {"ESP-IDF", app->idf_ver},
        {"Wi-Fi address", address},
        {"Flash", flash_size},
        {"PSRAM", psram_size},
        {"Touch", lvgl_display::TouchReady() ? "Ready" : "Not found"},
        {"Turned 180°", lvgl_display::Turned180() ? "Yes" : "No"},  // ux-exception: the board's own name for it
    };
    for (const auto &fact : facts) {
        widgets::MakeDetailRow(list.root, fact.name, fact.value);
    }
    widgets::UpdateListView(list);  // before the footer: the pages' stops come first in the walk, Back last
    EndScreen(screen);
}

// --- The serial console: a bench tool's commands --------------------------------------------------------------

// Lines typed on the console (UART0, the CH343P) run bench commands, so a test can be started and read from a
// computer (tests/bench/): `bench` opens the refresh benchmark and runs it, its results to the log; `anim` runs its bar
// without end; `pages [n]` walks the home rows; `stop` ends either; `speed [n]` picks the partial refresh's tuning;
// `key up|down|ok|hold [n]` presses a button through the real input path; `shot` sends the one-bit frame on the glass;
// `sound [name|band]` lists, plays or measures the buzzer's sounds; `help` lists them.

// The glass as base64 lines, each marked so a log line landing between them is told apart:
//   @shot begin 800 480 mono1   then   @shot <64 base64 characters>   …   then   @shot end <bytes> <crc32>
void SendShot()
{
    static const char kB64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    constexpr size_t kBytes = 800 * 480 / 8;
    if (!lvgl_display::Lock()) {
        return;
    }
    const uint8_t *frame = lvgl_display::GlassFrame();
    if (frame == nullptr) {
        lvgl_display::Unlock();
        std::printf("@shot none\n");
        return;
    }
    uint8_t *copy = static_cast<uint8_t *>(heap_caps_malloc(kBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (copy != nullptr) {
        std::memcpy(copy, frame, kBytes);
    }
    lvgl_display::Unlock();
    if (copy == nullptr) {
        std::printf("@shot none\n");
        return;
    }
    uint32_t crc = 0xFFFFFFFFu;  // CRC-32, as zlib's crc32
    for (size_t i = 0; i < kBytes; ++i) {
        crc ^= copy[i];
        for (int b = 0; b < 8; ++b) {
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
        }
    }
    std::printf("@shot begin 800 480 mono1\n");
    char line[80];
    for (size_t at = 0; at < kBytes; at += 48) {
        size_t o = 0;
        for (size_t i = at; i < at + 48 && i < kBytes; i += 3) {
            const uint32_t v = (copy[i] << 16) | (copy[i + 1] << 8) | copy[i + 2];
            line[o++] = kB64[(v >> 18) & 63];
            line[o++] = kB64[(v >> 12) & 63];
            line[o++] = kB64[(v >> 6) & 63];
            line[o++] = kB64[v & 63];
        }
        line[o] = '\0';
        std::printf("@shot %s\n", line);
        // stdout spins on the UART's FIFO: yield after each line, or the idle task starves and the task watchdog
        // fires partway through the five-second shot.
        vTaskDelay(1);
    }
    std::printf("@shot end %u %08lx\n", static_cast<unsigned>(kBytes), static_cast<unsigned long>(~crc));
    fflush(stdout);
    heap_caps_free(copy);
}
void RunBenchmark()
{
    if (lvgl_display::Lock()) {
        if (router::GoToRoute("hw.benchmark") == ESP_OK) {
            OnAnimRun(nullptr);
        }
        lvgl_display::Unlock();
    }
}

// `anim`: the benchmark's bar, moving without end on the preset now in use, for shots and photos of the glass.
void RunAnimation()
{
    if (lvgl_display::Lock()) {
        if (router::GoToRoute("hw.benchmark") != ESP_OK) {
            lvgl_display::Unlock();
            return;
        }
        s_anim = Anim{};
        s_anim.preset = lvgl_display::Tuning();
        s_anim_continuous = true;
        lv_obj_set_hidden(s_anim_run, true);
        AnimStartPreset();
        s_timer = lv_timer_create(AnimTick, 5, nullptr);
        lvgl_display::Unlock();
    }
}

void StopAnimation()
{
    if (lvgl_display::Lock()) {
        StopTimer();
        s_anim_continuous = false;
        lvgl_display::Unlock();
    }
}

void SetSpeed(const char *arg)
{
    const int preset = std::atoi(arg);
    if (lvgl_display::Lock()) {
        const esp_err_t err = lvgl_display::SetTuning(preset);
        lvgl_display::Unlock();
        std::printf("speed %d: %s (%s)\n", preset, lvgl_display::TuningName(lvgl_display::Tuning()),
                    esp_err_to_name(err));
    }
}

// `key up|down|ok|hold [n]`: presses a button n times (1 to 20) through the real input path, so a bench script can walk
// the screens a person would press to; each press waits out a full refresh before the next, so no OK lands in one. As on the
// buttons, with no ring on the glass — a screen just opened, or after a touch — the first press only shows the ring
// (DESIGN → Focus and input): `key down` there rings, and the next `key down` moves.
void PressKeys(const char *arg)
{
    constexpr int kBetweenMs = 1800;  // over a full refresh on either board, 1.58 s at most (STICKY_EPAPER_WAVEFORMS §4)
    char which[8] = {};
    int count = 1;
    if (std::sscanf(arg, "%7s %d", which, &count) < 1) {
        std::printf("key: up, down, ok or hold, and a count\n");
        return;
    }
    using lvgl_display::Button;
    const bool hold = std::strcmp(which, "hold") == 0;
    Button button;
    if (std::strcmp(which, "up") == 0) {
        button = Button::kUp;
    } else if (std::strcmp(which, "down") == 0) {
        button = Button::kDown;
    } else if (std::strcmp(which, "ok") == 0 || hold) {
        button = Button::kOk;
    } else {
        std::printf("key: up, down, ok or hold, and a count\n");
        return;
    }
    count = std::clamp(count, 1, 20);
    for (int i = 0; i < count; ++i) {
        if (i > 0) {
            vTaskDelay(pdMS_TO_TICKS(kBetweenMs));
        }
        lvgl_display::PressButton(button, hold);
    }
    std::printf("key %s x%d\n", which, count);
}

// `sound`: lists the sound words; `sound <name>` plays one, as its row on Buttons and buzzer does; `sound band` measures the
// buzzer's band with the board's own microphone (STICKY_HARDWARE_SPECS §4.2). The microphone is powered once and settles
// before anything is read — a fresh power-up swings louder than the room, which made every reading but 4 kHz look quieter
// than silence — then the room with the buzzer off, then each pitch the same way: a held full square, a moment for the coil
// to ring up, the level, silence, a moment for the room to go still. In dB against the 4 kHz resonance. The microphone sits a
// few centimetres from the buzzer in the same case, so the numbers are the board's own, its case and the microphone's
// response with them; the microphone's 16 kHz filter may take some of 7 kHz.
constexpr uint16_t kBandHz[] = {1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000, 7000};
constexpr uint32_t kBandListenMs = 300;
constexpr uint32_t kBandRingUpMs = 50;   // the coil up to its level before the reading
constexpr uint32_t kBandStillMs = 100;   // the coil and the room still before the next
constexpr uint32_t kBandSettleMs = 500;  // the microphone's first settling, longer than a level's: one run, every reading
// Each pitch sounds this long: the ring-up, then the reading, with room over.
constexpr uint16_t kBandHoldMs = 600;
static_assert(kBandRingUpMs + kBandListenMs < kBandHoldMs, "each reading ends while its pitch still sounds");

constexpr size_t kBandCount = sizeof(kBandHz) / sizeof(kBandHz[0]);

// One held note per pitch, a table that outlives its playing, as devices::Play asks.
constexpr std::array<devices::sound::Note, kBandCount> BandNotes()
{
    std::array<devices::sound::Note, kBandCount> notes = {};
    for (size_t i = 0; i < kBandCount; ++i) {
        notes[i] = {devices::sound::Wave::kPulse50, kBandHz[i], 0, kBandHoldMs, 255};
    }
    return notes;
}
constexpr std::array<devices::sound::Note, kBandCount> kBandNotes = BandNotes();

void MeasureBand()
{
    devices::Silence();
    if (devices::StartListening(kBandSettleMs) != ESP_OK) {
        std::printf("sound band: the microphone did not answer\n");
        return;
    }
    float quiet = 0;
    float rms[kBandCount] = {};
    int peak = 0;
    float at_resonance = 0;
    esp_err_t err = devices::ReadLevel(kBandListenMs, &quiet, &peak);
    // The microphone is read without a break, so its buffer never overflows and every reading is of its own moment; what
    // it hears while the coil rings up and while the room goes still is read and dropped.
    float dropped = 0;
    for (size_t i = 0; i < kBandCount && err == ESP_OK; ++i) {
        devices::Play({&kBandNotes[i], 1});
        err = devices::ReadLevel(kBandRingUpMs, &dropped, &peak);
        if (err == ESP_OK) {
            err = devices::ReadLevel(kBandListenMs, &rms[i], &peak);
        }
        devices::Silence();
        if (err == ESP_OK) {
            err = devices::ReadLevel(kBandStillMs, &dropped, &peak);
        }
        if (kBandHz[i] == devices::sound::kResonanceHz) {
            at_resonance = rms[i];
        }
    }
    devices::StopListening();
    if (err != ESP_OK) {
        std::printf("sound band: the microphone stopped (%s)\n", esp_err_to_name(err));
        return;
    }
    std::printf("sound band: room %.1f rms\n", quiet);
    for (size_t i = 0; i < kBandCount; ++i) {
        const float db = at_resonance > 0 && rms[i] > 0 ? 20.0f * std::log10(rms[i] / at_resonance) : 0.0f;
        std::printf("sound band: %5u Hz  %8.1f rms  %+5.1f dB\n", static_cast<unsigned>(kBandHz[i]), rms[i], db);
    }
}

void SoundCommand(const char *arg)
{
    using devices::sound::kSounds;
    if (arg[0] == '\0') {
        for (const devices::sound::NamedSound &n : kSounds) {
            std::printf("sound %-16s %s, %s, %u ms\n", n.id, n.name, n.what,
                        static_cast<unsigned>(devices::sound::LengthMs(n.sound)));
        }
        return;
    }
    if (std::strcmp(arg, "band") == 0) {
        MeasureBand();
        return;
    }
    for (const devices::sound::NamedSound &n : kSounds) {
        if (std::strcmp(arg, n.id) == 0) {
            devices::Play(n.sound);
            std::printf("sound %s: %s, %s\n", n.id, n.name, n.what);
            return;
        }
    }
    std::printf("sound: no sound \"%s\" (try: sound)\n", arg);
}

void StartPages(const char *arg);  // the page walk, below the home rows it opens
std::atomic<bool> s_pages_stop{false};
std::atomic<bool> s_pages_running{false};  // one walk at a time

void ConsoleTask(void *)
{
    char line[32];
    size_t len = 0;
    for (;;) {
        uint8_t c = 0;
        if (uart_read_bytes(UART_NUM_0, &c, 1, portMAX_DELAY) != 1) {
            continue;
        }
        if (c != '\n' && c != '\r') {
            if (len < sizeof(line) - 1) {
                line[len++] = static_cast<char>(c);
            }
            continue;
        }
        line[len] = '\0';
        len = 0;
        if (std::strcmp(line, "bench") == 0) {
            ESP_LOGI(kTag, "console: running the refresh benchmark");
            RunBenchmark();
        } else if (std::strcmp(line, "shot") == 0) {
            SendShot();
        } else if (std::strcmp(line, "anim") == 0) {
            RunAnimation();
        } else if (std::strcmp(line, "stop") == 0) {
            StopAnimation();
            s_pages_stop = true;
        } else if (std::strcmp(line, "pages") == 0 || std::strncmp(line, "pages ", 6) == 0) {
            StartPages(line[5] == ' ' ? line + 6 : "");
        } else if (std::strncmp(line, "speed ", 6) == 0) {
            SetSpeed(line + 6);
        } else if (std::strcmp(line, "speed") == 0) {
            for (int i = 0; i < lvgl_display::TuningCount(); ++i) {
                std::printf("speed %d: %s%s\n", i, lvgl_display::TuningName(i), i == lvgl_display::Tuning() ? " (now)" : "");
            }
        } else if (std::strncmp(line, "key ", 4) == 0) {
            PressKeys(line + 4);
        } else if (std::strcmp(line, "sound") == 0 || std::strncmp(line, "sound ", 6) == 0) {
            SoundCommand(line[5] == ' ' ? line + 6 : "");
        } else if (std::strcmp(line, "help") == 0) {
            std::printf("commands: bench, anim, pages [n], stop, speed [n], key up|down|ok|hold [n], sound [name|band], shot, "
                        "help\n");
        } else if (line[0] != '\0') {
            ESP_LOGW(kTag, "console: unknown command \"%s\" (try: help)", line);
        }
    }
}

void StartConsole()
{
    if (uart_driver_install(UART_NUM_0, 256, 0, 0, nullptr, 0) != ESP_OK) {
        ESP_LOGW(kTag, "console: UART0 driver did not start; no commands");
        return;
    }
    xTaskCreate(ConsoleTask, "hw_test_console", 4096, nullptr, 2, nullptr);
    ESP_LOGI(kTag, "console ready: bench, anim, pages, stop, speed, key, sound, shot, help");
}

// --- Home -----------------------------------------------------------------------------------------------------

// The test screens as routes, ids `hw.<screen>`, and Display's sub-screens under it: what a home row, Display's buttons and the
// bench console push (router/route.h).
std::unique_ptr<router::Screen> DisplayScreen() { return TestScreen(CreateDisplay, EnterDisplay); }
std::unique_ptr<router::Screen> GraysScreen() { return TestScreen(CreateGrays, EnterGrays); }
std::unique_ptr<router::Screen> TypeScaleScreen() { return TestScreen(CreateTypeScale); }
std::unique_ptr<router::Screen> KitScreen() { return router::MakeScreen(CreateKit, nullptr, LeaveKit); }
std::unique_ptr<router::Screen> BenchmarkScreen() { return TestScreen(CreateBenchmark); }
std::unique_ptr<router::Screen> TouchScreen() { return TestScreen(CreateTouch); }
std::unique_ptr<router::Screen> ButtonsScreen() { return TestScreen(CreateButtons, EnterButtons); }
std::unique_ptr<router::Screen> SensorsScreen() { return TestScreen(CreateSensors, EnterSensors, PauseWhileCovered); }
std::unique_ptr<router::Screen> ClockScreenRoute() { return TestScreen(CreateClock); }
std::unique_ptr<router::Screen> WelcomeScreen() { return TestScreen(CreateWelcome); }
std::unique_ptr<router::Screen> PowerScreen() { return TestScreen(CreatePower, EnterPower, PauseWhileCovered); }
std::unique_ptr<router::Screen> StorageScreen() { return TestScreen(CreateStorage); }
std::unique_ptr<router::Screen> MicrophoneScreen() { return TestScreen(CreateMicrophone); }
std::unique_ptr<router::Screen> AboutScreen() { return TestScreen(CreateAbout); }
std::unique_ptr<router::Screen> RemindersScreen() { return TestScreen(CreateReminders); }

constexpr router::Route kRoutes[] = {
    {"hw.display", router::NoParam<DisplayScreen>},
    {"hw.display.grays", router::NoParam<GraysScreen>},
    {"hw.display.type-scale", router::NoParam<TypeScaleScreen>},
    {"hw.display.kit", router::NoParam<KitScreen>},
    {"hw.benchmark", router::NoParam<BenchmarkScreen>},
    {"hw.touch", router::NoParam<TouchScreen>},
    {"hw.buttons", router::NoParam<ButtonsScreen>},
    {"hw.sensors", router::NoParam<SensorsScreen>},
    {"hw.clock", router::NoParam<ClockScreenRoute>},
    {"hw.welcome", router::NoParam<WelcomeScreen>},
    {"hw.power", router::NoParam<PowerScreen>},
    {"hw.storage", router::NoParam<StorageScreen>},
    {"hw.microphone", router::NoParam<MicrophoneScreen>},
    {"hw.about", router::NoParam<AboutScreen>},
    {"hw.reminders", router::NoParam<RemindersScreen>},
};

// A kBoardUp step, before anything is drawn: the routes are reachable by the time home is (APP_ARCHITECTURE §3).
void AddRoutes()
{
    if (router::AddRoutes({kRoutes, sizeof(kRoutes) / sizeof(kRoutes[0])}) != ESP_OK) {
        ESP_LOGE(kTag, "the test screens' routes were refused: home's rows open nothing");
    }
}

struct Entry {
    const char *label;
    const char *supporting;
    const char *route;
};
constexpr Entry kEntries[] = {
    {"Display", "Refresh modes, greys, ghosting", "hw.display"},
    {"Refresh benchmark", "A bar, timed at each speed", "hw.benchmark"},
    {"Touch", "Where a tap lands", "hw.touch"},
    {"Buttons and buzzer", "UP, DOWN, OK and the tick", "hw.buttons"},
    {"Sensors", "Temperature, motion, clock", "hw.sensors"},
    {"Clock", "Read and set the RTC", "hw.clock"},
    {"Welcome", "A first-run carousel", "hw.welcome"},
    {"Battery and power", "Gauge, charger, sleep, power off", "hw.power"},
    {"Storage", "Flash, PSRAM and the card", "hw.storage"},
    {"Microphone", "A second of sound level", "hw.microphone"},
    {"Reminders", "A choice that applies at once", "hw.reminders"},
    {"About", "Firmware and board", "hw.about"},
};

void OnEntry(lv_event_t *e)
{
    router::PushRoute(static_cast<const Entry *>(lv_event_get_user_data(e))->route);
}

// Home's rows: the content column's width, 80 high, 16 apart with no rule between, seven to a page; the rest turn with a
// swipe or when the focus leaves the page (DESIGN → Layout → Paging).
constexpr int32_t kHomeRowsPerPage = 7;

void BuildHome(lv_obj_t *home)
{
    lv_obj_t *column = lv_obj_create(home);
    lv_obj_remove_style_all(column);
    lv_obj_set_width(column, lv_pct(100));
    lv_obj_set_flex_grow(column, 1);
    lv_obj_set_style_pad_hor(column, dt::kSpacingEdge, 0);
    lv_obj_set_style_pad_ver(column, dt::kSpacingMd, 0);
    lv_obj_set_scrollable(column, false);
    ui::LetRingOut(column);
    const widgets::ListView list = widgets::MakeListView(column, kHomeRowsPerPage, dt::kSpacingListRowHeight, widgets::IndicatorCentre::kGlass);
    for (const Entry &entry : kEntries) {
        ui::NewRow(list.root, entry.label, entry.supporting, OnEntry, const_cast<Entry *>(&entry));
    }
    widgets::UpdateListView(list);
}

// The page walk: opens each home row in turn and goes back, a few seconds on each, so a long run of page changes can
// be watched on the glass for ghosting (STICKY_DESIGN_TOKENS §5). The benchmark's row is left out: it animates.
constexpr uint32_t kPageDwellMs = 3000;

void PagesTask(void *arg)
{
    const int count = static_cast<int>(reinterpret_cast<intptr_t>(arg));
    constexpr int kRows = static_cast<int>(sizeof(kEntries) / sizeof(kEntries[0]));
    int row = 0;
    for (int k = 0; k < count && !s_pages_stop; ++k) {
        if (std::strcmp(kEntries[row].route, "hw.benchmark") == 0) {
            row = (row + 1) % kRows;
        }
        ESP_LOGI(kTag, "pages: %d of %d, %s", k + 1, count, kEntries[row].label);
        if (lvgl_display::Lock()) {
            router::PopToRoot();
            router::PushRoute(kEntries[row].route);
            lvgl_display::Unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(kPageDwellMs));
        if (lvgl_display::Lock()) {
            router::PopToRoot();
            lvgl_display::Unlock();
        }
        if (s_pages_stop) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(kPageDwellMs));
        row = (row + 1) % kRows;
    }
    ESP_LOGI(kTag, "pages: done");
    s_pages_running = false;
    vTaskDelete(nullptr);
}

void StartPages(const char *arg)
{
    if (s_pages_running.exchange(true)) {
        ESP_LOGW(kTag, "pages: a walk is running — stop it first");
        return;
    }
    const long n = arg[0] != '\0' ? std::strtol(arg, nullptr, 0) : 40;
    s_pages_stop = false;
    if (xTaskCreate(PagesTask, "hw_test_pages", 4096, reinterpret_cast<void *>(static_cast<intptr_t>(n > 0 ? n : 40)),
                    2, nullptr) != pdPASS) {
        s_pages_running = false;
        ESP_LOGE(kTag, "pages: no memory for the walk's task");
    }
}

}  // namespace

void Install()
{
    app_hooks::OnHomeBuilt(BuildHome);
    app_hooks::OnBoot(app_hooks::Phase::kBoardUp, AddRoutes);
    app_hooks::OnBoot(app_hooks::Phase::kReady, NoteStart);
    app_hooks::OnBoot(app_hooks::Phase::kReady, StartConsole);
}

}  // namespace hw_test
