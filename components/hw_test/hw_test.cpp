// The hardware test: a row on home for each part of the board, and a screen that exercises it — the glass on either
// controller, touch, the buttons and buzzer, the sensors, the battery and power, the card, the microphone. It is a
// bench tool for V1.0 and V2.0 boards, drawn from DESIGN.md's components. Its fixtures' sizes, its timers and the
// refresh benchmark's animation are DESIGN.md's one registered exception (DESIGN → Exceptions).

#include <algorithm>
#include <atomic>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "app_hooks.h"
#include "board.h"
#include "design_tokens.h"
#include "devices.h"
#include "lvgl_display.h"
#include "ui.h"
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
using ui::Type;
constexpr const char *kTag = "hw_test";

lv_obj_t *s_home = nullptr;  // home's screen, kept while a test screen shows
constexpr size_t kMaxHomeRows = 9;
lv_obj_t *s_home_rows[kMaxHomeRows] = {};
size_t s_home_row_count = 0;
lv_obj_t *s_current = nullptr;  // the test screen showing, or nullptr on home
lv_timer_t *s_timer = nullptr;  // the showing screen's self-update, if it has one
lv_obj_t *s_opener = nullptr;   // the home row that opened the screen showing: focus goes back to it
bool s_bench_running = false;   // a benchmark run is cycling the presets: leaving it puts the default back

// --- Navigation -----------------------------------------------------------------------------------------------

void StopTimer()
{
    if (s_timer != nullptr) {
        lv_timer_delete(s_timer);
        s_timer = nullptr;
    }
}

// Leaving a screen: its timer stops, the buttons stop reporting to it, and a benchmark cut short leaves the default
// preset, not the last one it tried.
void LeaveScreen()
{
    StopTimer();
    lvgl_display::SetKeyObserver(nullptr);
    if (s_bench_running) {
        s_bench_running = false;
        lvgl_display::SetTuning(lvgl_display::DefaultTuning());
    }
}

void ShowHome()
{
    if (s_current == nullptr) {
        return;
    }
    LeaveScreen();
    lvgl_display::ClearGroup();
    lv_screen_load(s_home);
    lv_obj_delete(s_current);
    s_current = nullptr;
    for (size_t i = 0; i < s_home_row_count; ++i) {
        lv_group_add_obj(lvgl_display::Group(), s_home_rows[i]);
    }
    if (s_opener != nullptr) {
        lv_group_focus_obj(s_opener);  // the next step starts where the last one did
    }
    lvgl_display::SetBackHandler(nullptr);
}

void OnBack(lv_event_t *)
{
    ShowHome();
}

// Starts a test screen: the glass, its status bar, and a content column; Back goes home.
lv_obj_t *BeginScreen(const char *title, lv_obj_t **content)
{
    if (s_current == nullptr) {
        s_opener = lv_group_get_focused(lvgl_display::Group());  // a tap on a row focuses it too
    }
    LeaveScreen();
    lvgl_display::ClearGroup();
    lv_obj_t *screen = ui::NewScreen();
    ui::NewStatusBar(screen, title);
    lv_obj_t *column = lv_obj_create(screen);
    lv_obj_remove_style_all(column);
    lv_obj_set_width(column, lv_pct(100));
    lv_obj_set_flex_grow(column, 1);
    lv_obj_set_flex_flow(column, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_hor(column, dt::kSpacingEdge, 0);
    lv_obj_set_style_pad_ver(column, dt::kSpacingMd, 0);
    lv_obj_set_style_pad_row(column, dt::kSpacingSm, 0);
    lv_obj_set_scrollable(column, false);
    *content = column;
    return screen;
}

// The footer: Back last in the focus order, after the screen's own controls, always at the bottom.
void EndScreen(lv_obj_t *screen)
{
    lv_obj_t *footer = lv_obj_create(screen);
    lv_obj_remove_style_all(footer);
    lv_obj_set_size(footer, lv_pct(100), dt::kSpacingFooter);
    lv_obj_set_style_pad_hor(footer, dt::kSpacingEdge, 0);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollable(footer, false);
    lv_obj_t *back = ui::NewButton(footer, "Back", false, OnBack, nullptr);
    lv_obj_set_width(back, 160);

    lv_obj_t *old = s_current;
    s_current = screen;
    lv_screen_load(screen);
    if (old != nullptr) {
        lv_obj_delete(old);
    }
    lvgl_display::SetBackHandler(ShowHome);
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
    lv_obj_set_style_pad_column(row, dt::kSpacingSm, 0);
    lv_obj_set_style_pad_row(row, dt::kSpacingSm, 0);
    lv_obj_set_scrollable(row, false);
    return row;
}

lv_obj_t *HalfButton(lv_obj_t *row, const char *label, bool action, lv_event_cb_t cb, void *data = nullptr)
{
    lv_obj_t *button = ui::NewButton(row, label, action, cb, data);
    lv_obj_set_width(button, (dt::kSpacingContent - dt::kSpacingSm) / 2);
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

void FillStats()
{
    const lvgl_display::Stats s = lvgl_display::GetStats();
    char temp[16] = "unread";
    if (s.temperature_c >= 0) {
        std::snprintf(temp, sizeof(temp), "%d °C", s.temperature_c);
    }
    lv_label_set_text_fmt(s_display_stats,
                          "Last: %s, %" PRIu32 " ms\nLatch %" PRIu32 " · send %" PRIu32 " · power %" PRIu32
                          " · drive %" PRIu32 "\nPartials since full: %" PRIu32 " of %d · skipped %" PRIu32 "\n"
                          "Waveform temperature: %s",
                          ModeName(s.last), s.last_ms, s.latch_ms, s.stream_ms, s.power_on_ms, s.drive_ms, s.partials,
                          dt::kRefreshPartialLimit, s.skipped, temp);
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

// A small change, as a real update is: the button counts its own presses. An unchanged frame is never sent.
void OnPartial(lv_event_t *e)
{
    static uint32_t presses = 0;
    lv_obj_t *button = lv_event_get_target_obj(e);
    lv_label_set_text_fmt(lv_obj_get_child(button, 0), "Partial · %" PRIu32, ++presses);
    s_stats_pending = true;
}

void ShowDisplay();

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

void OnGray(lv_event_t *);

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

void ShowDisplay()
{
    lv_obj_t *content;
    lv_obj_t *screen = BeginScreen("Display", &content);
    const lvgl_display::Stats s = lvgl_display::GetStats();
    ui::NewLabel(content, Type::kHeadline, s.ssd2677 ? "SSD2677 · V2.0" : "SSD1677 · V1.0");
    ui::NewLabel(content, Type::kCaption,
                        s.ssd2677 ? "Matte glass. The controller answered 0x07 to its ID read."
                                  : "Glossy glass. The controller did not answer the SSD2677's ID.",
                        true);
    s_display_stats = Value(content, "");
    FillStats();

    lv_obj_t *row = ButtonRow(content);
    HalfButton(row, "Full refresh", true, OnFull);
    HalfButton(row, "Partial refresh", false, OnPartial);
    HalfButton(row, "Four greys", false, OnGray);
    lv_obj_t *pattern_button = HalfButton(row, "Pattern", false, nullptr);  // in the row, before Speed below it

    if (lvgl_display::TuningCount() > 1) {
        s_tuning_button = ui::NewButton(content, "", false, OnTuning, nullptr);
        lv_obj_set_width(s_tuning_button, lv_pct(100));
        SetTuningLabel();
    }

    // The ghosting box: a plain outline, filled with the checkerboard each time Pattern is pressed.
    lv_obj_t *checker = lv_obj_create(content);
    lv_obj_remove_style_all(checker);
    lv_obj_set_size(checker, lv_pct(100), 96);
    lv_obj_set_style_border_width(checker, dt::kSpacingStrokeDefault, 0);
    lv_obj_set_style_border_color(checker, ui::Level(dt::kColorOutline), 0);
    lv_obj_set_style_pad_all(checker, dt::kSpacingStrokeDefault, 0);
    lv_obj_set_style_bg_opa(checker, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(checker, DrawChecker, LV_EVENT_DRAW_MAIN_END, nullptr);
    lv_obj_add_event_cb(pattern_button, OnPattern, LV_EVENT_CLICKED, checker);

    // Orientation: this word must read at the top of the glass, the right way up.
    ui::NewLabel(content, Type::kCaption, "This screen's title is at the top edge, above the buttons.", true);

    s_timer = lv_timer_create(ShowStatsOnce, 1500, nullptr);
    EndScreen(screen);
}

void OnGrayBack(lv_event_t *)
{
    ShowDisplay();
}

void OnGray(lv_event_t *)
{
    // Four bands, black to white, drawn in two bits a pixel. The frame after it is drawn full, one bit.
    lv_obj_t *content;
    lv_obj_t *screen = BeginScreen("Four greys", &content);
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
    HalfButton(row, "Display", false, OnGrayBack);
    lvgl_display::RequestGray();
    EndScreen(screen);
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

void ShowBenchmark()
{
    lv_obj_t *content;
    lv_obj_t *screen = BeginScreen("Refresh benchmark", &content);
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
    lv_label_set_text_fmt(s_touch_label, "Raw %d, %d → screen %d, %d", t.raw_x, t.raw_y, t.x, t.y);
}

void ShowTouch()
{
    lv_obj_t *content;
    lv_obj_t *screen = BeginScreen("Touch", &content);
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

void ShowButtons()
{
    lv_obj_t *content;
    lv_obj_t *screen = BeginScreen("Buttons and buzzer", &content);
    ui::NewLabel(content, Type::kBody,
                        "UP and DOWN move the ring. OK presses. Hold OK for a second to go back. Every press ticks.");
    s_keys_label = Value(content, "Last: none");
    s_keys_seen = 0;
    lv_obj_t *row = ButtonRow(content);
    HalfButton(row, "Tick", true, OnTick);
    HalfButton(row, "Tone, 1 s", false, OnTone);
    lvgl_display::SetKeyObserver(OnKey);
    if (!devices::Found().buzzer) {
        Value(content, "The buzzer did not start. Check the log.");
    }
    EndScreen(screen);
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

void ShowSensors()
{
    lv_obj_t *content;
    lv_obj_t *screen = BeginScreen("Sensors", &content);
    s_sensors_label = Value(content, "");
    FillSensors();
    ui::NewLabel(content, Type::kCaption, "Reads again every 5 seconds.", true);
    lv_obj_t *row = ButtonRow(content);
    HalfButton(row, "Read now", true, OnReadSensors);
    s_timer = lv_timer_create(FillSensors, dt::kSelfUpdateMinMs, nullptr);
    EndScreen(screen);
}

// --- Battery and power ----------------------------------------------------------------------------------------

lv_obj_t *s_power_label = nullptr;
lv_obj_t *s_charge_button = nullptr;

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
    lv_label_set_text_fmt(s_power_label, "%s\nUSB %s (%d mV at the pin)\nCharger %s, STAT %s", gauge,
                          board::UsbPresent() ? "in" : "out", vin, board::ChargingEnabled() ? "on" : "off",
                          board::ChargeStatusLow() ? "low, charging" : "high");
}

void OnCharge(lv_event_t *)
{
    board::SetChargingEnabled(!board::ChargingEnabled());
    lv_label_set_text(lv_obj_get_child(s_charge_button, 0),
                      board::ChargingEnabled() ? "Stop charging" : "Allow charging");
    FillPower();
}

// The frame left on the glass when the board leaves: drawn in full, then the panel sleeps (DESIGN → Motion).
void Leave(bool power_off)
{
    StopTimer();
    lvgl_display::ClearGroup();
    lv_obj_t *screen = ui::NewScreen();
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(screen, dt::kSpacingEdge, 0);
    lv_obj_t *title = ui::NewLabel(screen, Type::kDisplay, power_off ? "Off" : "Asleep");
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_t *hint = ui::NewLabel(
        screen, Type::kBody, power_off ? "Press OK to turn on." : "Press OK, UP or DOWN to wake.");
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_t *old = s_current;
    s_current = screen;
    lv_screen_load(screen);
    if (old != nullptr) {
        lv_obj_delete(old);
    }
    lvgl_display::RequestFull();
    lvgl_display::Redraw();
    lvgl_display::SleepPanel();
    if (power_off) {
        board::PowerOff();  // returns only when USB keeps the board on and it did not restart
    } else {
        board::EnterDeepSleep();
    }
    ESP_LOGW(kTag, "still running after %s", power_off ? "power off" : "deep sleep");
}

void LeaveSoon(lv_timer_t *timer)
{
    const bool power_off = lv_timer_get_user_data(timer) != nullptr;
    lv_timer_delete(timer);
    Leave(power_off);
}

void OnSleep(lv_event_t *)
{
    // From a timer, not this click: the press has to finish before the board waits for the buttons to be released.
    lv_timer_create(LeaveSoon, 50, nullptr);
}

void OnPowerOff(lv_event_t *)
{
    lv_timer_create(LeaveSoon, 50, reinterpret_cast<void *>(1));
}

void ShowPower()
{
    lv_obj_t *content;
    lv_obj_t *screen = BeginScreen("Battery and power", &content);
    s_power_label = Value(content, "");
    FillPower();
    lv_obj_t *row = ButtonRow(content);
    s_charge_button = HalfButton(row, board::ChargingEnabled() ? "Stop charging" : "Allow charging", false, OnCharge);
    HalfButton(row, "Deep sleep", false, OnSleep);
    HalfButton(row, "Power off", true, OnPowerOff);
    ui::NewLabel(content, Type::kCaption,
                        "On USB, power off restarts the board. On battery it turns off; press OK to turn on.", true);
    s_timer = lv_timer_create(FillPower, dt::kSelfUpdateMinMs, nullptr);
    EndScreen(screen);
}

// --- Storage --------------------------------------------------------------------------------------------------

lv_obj_t *s_card_label = nullptr;

void OnProbeCard(lv_event_t *)
{
    lv_label_set_text(s_card_label, "Reading the card…");
    lv_refr_now(nullptr);  // on the glass before the mount blocks
    devices::Card card;
    const esp_err_t err = devices::ProbeCard(&card);
    if (!card.inserted) {
        lv_label_set_text(s_card_label, "No card. Insert one and try again.");
    } else if (err != ESP_OK) {
        lv_label_set_text_fmt(s_card_label, "The card did not mount (%s). Check it is FAT32, then try again.",
                              esp_err_to_name(err));
    } else {
        lv_label_set_text_fmt(s_card_label, "%s · %llu MiB\n%d entries in the root%s", card.name,
                              static_cast<unsigned long long>(card.capacity_mib), card.entries,
                              card.entries >= 99 ? " or more" : "");
    }
}

void ShowStorage()
{
    lv_obj_t *content;
    lv_obj_t *screen = BeginScreen("Storage", &content);
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

void OnListen(lv_event_t *)
{
    lv_label_set_text(s_mic_label, "Listening…");
    lv_refr_now(nullptr);  // on the glass before the second of recording blocks
    float rms = 0;
    int peak = 0;
    const esp_err_t err = devices::ListenLevel(1000, &rms, &peak);
    if (err != ESP_OK) {
        lv_label_set_text_fmt(s_mic_label, "The microphone did not read (%s). Check the log, then try again.",
                              esp_err_to_name(err));
        lv_obj_set_width(s_mic_bar, dt::kSpacingStrokeDefault);  // no level shown that this press did not hear
        return;
    }
    lv_label_set_text_fmt(s_mic_label, "Level %.0f RMS · peak %d", rms, peak);
    const int32_t width = std::min<int32_t>(dt::kSpacingContent, static_cast<int32_t>(rms * dt::kSpacingContent / 4000));
    lv_obj_set_width(s_mic_bar, std::max<int32_t>(dt::kSpacingStrokeDefault, width));
}

void ShowMicrophone()
{
    lv_obj_t *content;
    lv_obj_t *screen = BeginScreen("Microphone", &content);
    ui::NewLabel(content, Type::kBody, "Press Listen, then speak or clap. It records one second.");
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

// --- About ----------------------------------------------------------------------------------------------------

void ShowAbout()
{
    lv_obj_t *content;
    lv_obj_t *screen = BeginScreen("About", &content);
    const esp_app_desc_t *app = esp_app_get_description();
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    uint8_t mac[6] = {};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    const lvgl_display::Stats s = lvgl_display::GetStats();
    lv_obj_t *about = Value(content, "");
    lv_label_set_text_fmt(about,
                          "%s %s\nESP-IDF %s\n%s rev %d.%d\nMAC %02X:%02X:%02X:%02X:%02X:%02X\n%s, touch %s\n"  // ux-exception: names
                          "Turned 180°: %s",
                          app->project_name, app->version, app->idf_ver, CONFIG_IDF_TARGET, chip.revision / 100,
                          chip.revision % 100, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                          s.ssd2677 ? "SSD2677 (V2.0)" : "SSD1677 (V1.0)", lvgl_display::TouchReady() ? "ok" : "absent",
                          lvgl_display::Turned180() ? "yes" : "no");
    EndScreen(screen);
}

// --- The serial console: a bench tool's commands --------------------------------------------------------------

// Lines typed on the console (UART0, the CH343P) run bench commands, so a test can be started and read from a
// computer (tests/bench/): `bench` opens the refresh benchmark and runs it, its results to the log; `anim` runs its bar
// without end; `pages [n]` walks the home rows; `stop` ends either; `speed [n]` picks the partial refresh's tuning;
// `shot` sends the one-bit frame on the glass; `help` lists them.

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
        ShowBenchmark();
        OnAnimRun(nullptr);
        lvgl_display::Unlock();
    }
}

// `anim`: the benchmark's bar, moving without end on the preset now in use, for shots and photos of the glass.
void RunAnimation()
{
    if (lvgl_display::Lock()) {
        ShowBenchmark();
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
        } else if (std::strcmp(line, "help") == 0) {
            std::printf("commands: bench, anim, pages [n], stop, speed [n], shot, help\n");
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
    ESP_LOGI(kTag, "console ready: bench, anim, pages, stop, speed, shot, help");
}

// --- Home -----------------------------------------------------------------------------------------------------

struct Entry {
    const char *label;
    const char *supporting;
    void (*show)();
};
constexpr Entry kEntries[] = {
    {"Display", "Refresh modes, greys, ghosting", ShowDisplay},
    {"Refresh benchmark", "An animation, timed for each speed", ShowBenchmark},
    {"Touch", "Where a tap lands", ShowTouch},
    {"Buttons and buzzer", "UP, DOWN, OK and the tick", ShowButtons},
    {"Sensors", "Temperature, humidity, motion, clock", ShowSensors},
    {"Battery and power", "Gauge, charger, sleep, power off", ShowPower},
    {"Storage", "Flash, PSRAM and the card", ShowStorage},
    {"Microphone", "A second of sound level", ShowMicrophone},
    {"About", "Firmware and board", ShowAbout},
};
static_assert(sizeof(kEntries) / sizeof(kEntries[0]) <= kMaxHomeRows, "raise kMaxHomeRows");

void OnEntry(lv_event_t *e)
{
    const auto *entry = static_cast<const Entry *>(lv_event_get_user_data(e));
    entry->show();
}

void BuildHome(lv_obj_t *home)
{
    s_home = lv_obj_get_screen(home);
    s_home_row_count = 0;
    for (const Entry &entry : kEntries) {
        s_home_rows[s_home_row_count++] =
            ui::NewRow(home, entry.label, entry.supporting, OnEntry, const_cast<Entry *>(&entry));
    }
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
        if (kEntries[row].show == ShowBenchmark) {
            row = (row + 1) % kRows;
        }
        ESP_LOGI(kTag, "pages: %d of %d, %s", k + 1, count, kEntries[row].label);
        if (lvgl_display::Lock()) {
            kEntries[row].show();
            lvgl_display::Unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(kPageDwellMs));
        if (lvgl_display::Lock()) {
            ShowHome();
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
    app_hooks::OnBoot(app_hooks::Phase::kReady, StartConsole);
}

}  // namespace hw_test
