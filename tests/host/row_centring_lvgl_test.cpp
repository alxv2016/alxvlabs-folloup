// Host test that a row's content is centred on its height, on LVGL's real layout with the real widgets and fonts, on the
// computer with no board: every kind of row the kit has — a toggle row, a toggle icon row, a checkbox row, a radio button row, a
// value row, a dropdown, a Home row, a radio row, a choice card, a Wi-Fi network row, a list view's rows, and the select sheet's
// options — is built with one line of content and with more, and the centre of its content block must be within 1 px of the
// row's own centre (DESIGN → Components → The kit: a row's content is centred on its height; STICKY_DESIGN_TOKENS §9).
//
// LVGL sizes a flex track to its tallest child, so a row's minimum height on the root alone pads below the content; the pattern
// that holds is a content column that takes the row's height and centres its lines. This test is what keeps a later change from
// drifting a row without anyone seeing. The content block is the box round the row's visible leaf objects — its words (line
// boxes included), its images and its controls; each case prints the offset it measured, in px, positive below the row's centre.
// run.sh builds LVGL's sources from managed_components once, into build/host/lvgl, and the sources named below, against stand-ins
// for the ESP-IDF headers (tests/host/stubs).
// host-sources: components/ui/ui.cpp components/ui/line_box.cpp components/ui/fonts/bitcount/*.c components/widgets/marks/marks.c components/widgets/src/*.cpp components/widgets/src/internal/*.cpp components/router/router.cpp components/router/overlays.cpp components/router/focus.cpp components/router/touch.cpp components/router/routes.cpp components/router/screen.cpp components/router/status.cpp
// Build and run: bash tests/host/run.sh

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "design_tokens.h"
#include "lvgl.h"
#include "lvgl_display.h"
#include "router/router.h"
#include "ui.h"
#include "widgets/widgets.h"

namespace dt = design_tokens;

// What the widgets take from lvgl_display, which is not linked: the focus group, and the few calls the keyboard and the sheet make.
namespace {
lv_group_t *s_group = nullptr;
void (*s_back)() = nullptr;
}  // namespace
namespace lvgl_display {
lv_group_t *Group() { return s_group; }
void ClearGroup()
{
    lv_group_set_editing(s_group, false);
    lv_group_remove_all_objs(s_group);
}
// The router binds a screen's or an overlay's group here: the buttons and LVGL's default group follow it, as on the board.
void SetGroup(lv_group_t *group)
{
    if (group == nullptr) {
        return;
    }
    s_group = group;
    for (lv_indev_t *indev = lv_indev_get_next(nullptr); indev != nullptr; indev = lv_indev_get_next(indev)) {
        lv_indev_set_group(indev, group);  // a keypad's only: LVGL ignores it for a pointer
    }
    lv_group_set_default(group);
}
void SetBackHandler(void (*back)()) { s_back = back; }
// The router's read hooks, kept so a test can run a key read through the focus rules as LVGL's read would.
void (*s_key_hook)(lv_indev_t *, lv_indev_data_t *) = nullptr;
void (*s_touch_hook)(lv_indev_t *, lv_indev_data_t *) = nullptr;
void SetKeyReadHook(void (*hook)(lv_indev_t *, lv_indev_data_t *)) { s_key_hook = hook; }
void SetTouchReadHook(void (*hook)(lv_indev_t *, lv_indev_data_t *)) { s_touch_hook = hook; }
void SetPartialCounting(PartialCounting) {}
void NoteTyping() {}
void RequestFull() {}
void PlayCue(Cue) {}
}  // namespace lvgl_display

namespace {

int s_failures = 0;
int s_checks = 0;

void Check(bool ok, const std::string &what)
{
    ++s_checks;
    if (!ok) {
        ++s_failures;
        std::printf("FAIL  %s\n", what.c_str());
    }
}

void Flush(lv_display_t *display, const lv_area_t *, uint8_t *)
{
    lv_display_flush_ready(display);
}

uint32_t Ticks()
{
    return 0;
}

// --- Measuring ---------------------------------------------------------------------------------------------------------

struct Span {
    int32_t y1 = 1 << 20, y2 = -(1 << 20);
    bool any = false;
};

// The visible leaves under `obj`: an object with no child and a size is content — a label, an image, a bar, a thumb.
void Leaves(lv_obj_t *obj, Span &span, bool root)
{
    if (!root && lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN)) {
        return;
    }
    const uint32_t n = lv_obj_get_child_count(obj);
    if (n == 0 && !root) {
        lv_area_t a;
        lv_obj_get_coords(obj, &a);
        if (lv_area_get_width(&a) > 0 && lv_area_get_height(&a) > 0) {
            span.y1 = std::min(span.y1, a.y1);
            span.y2 = std::max(span.y2, a.y2);
            span.any = true;
        }
        return;
    }
    for (uint32_t i = 0; i < n; ++i) {
        Leaves(lv_obj_get_child(obj, static_cast<int32_t>(i)), span, false);
    }
}

// The offset, in px, of the content block's centre from the row's: twice the difference of the sums, halved with the odd pixel kept.
int Offset(lv_obj_t *row)
{
    lv_obj_update_layout(row);
    lv_area_t r;
    lv_obj_get_coords(row, &r);
    Span span;
    Leaves(row, span, true);
    if (!span.any) {
        return 1 << 20;
    }
    // Centres in half pixels, so an odd box is not rounded away: (y1 + y2 + 1) / 2 is the centre of an inclusive span.
    const int twice_content = span.y1 + span.y2 + 1;
    const int twice_row = r.y1 + r.y2 + 1;
    return (twice_content - twice_row) / 2;
}

// --- A page of rows ----------------------------------------------------------------------------------------------------

lv_obj_t *s_screen = nullptr;
lv_obj_t *s_column = nullptr;

void NewPage()
{
    if (s_screen != nullptr) {
        lvgl_display::ClearGroup();
        lv_obj_t *blank = lv_obj_create(nullptr);
        lv_screen_load(blank);
        lv_obj_delete(s_screen);  // LVGL's pool is 64 KB by default: a page goes before the next is built
    }
    s_screen = ui::NewScreen();
    s_column = lv_obj_create(s_screen);
    lv_obj_remove_style_all(s_column);
    lv_obj_set_width(s_column, lv_pct(100));
    lv_obj_set_flex_grow(s_column, 1);
    lv_obj_set_flex_flow(s_column, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_hor(s_column, dt::kSpacingEdge, 0);
    lv_obj_set_style_pad_row(s_column, dt::kSpacingMd, 0);
    lv_obj_set_scrollable(s_column, false);
    lv_screen_load(s_screen);
}

void Measure(const std::string &what, lv_obj_t *row, int expected_height = -1)
{
    lv_obj_update_layout(s_screen);
    const int offset = Offset(row);
    const int height = lv_obj_get_height(row);
    std::printf("  %-58s row %3d px  offset %+d px\n", what.c_str(), height, offset);
    Check(std::abs(offset) <= 1, what + ": content centre within 1 px of the row's");
    if (expected_height > 0) {
        Check(height == expected_height, what + ": the row is " + std::to_string(expected_height) + " px high");
    }
}

constexpr const char *kLong = "Keep my follow-ups in step with my phone, and tell me when they are done";  // wraps to more than one line

void TestSettingRows()
{
    std::printf("setting rows\n");
    NewPage();
    Measure("toggle row, one line", widgets::MakeToggleRow(s_column, "Wi-Fi", true).root, 64);
    Measure("toggle row, two lines", widgets::MakeToggleRow(s_column, kLong, true).root);
    widgets::ToggleRow off = widgets::MakeToggleRow(s_column, "Background", false);
    widgets::SetToggleRowEnabled(off, false, "Turn Wi-Fi on first");
    Measure("toggle row, disabled with its reason", off.root, 64);
    Measure("toggle icon row, one line", widgets::MakeToggleIconRow(s_column, "Sound", true).root, 64);
    Measure("toggle icon row, two lines", widgets::MakeToggleIconRow(s_column, kLong, false).root);
    Measure("checkbox row, one line", widgets::MakeCheckboxRow(s_column, "Keep recordings", true).root, 64);
    Measure("checkbox row, two lines", widgets::MakeCheckboxRow(s_column, kLong, false).root);
    widgets::CheckboxRow no_box = widgets::MakeCheckboxRow(s_column, "Share with the team", false);
    widgets::SetCheckboxRowEnabled(no_box, false, "Sign in first");
    Measure("checkbox row, disabled with its reason", no_box.root, 64);
    Measure("radio button row, one line", widgets::MakeRadioButtonRow(s_column, "Every day", true).root, 64);
    Measure("radio button row, two lines", widgets::MakeRadioButtonRow(s_column, kLong, false).root);
}

void TestValueRows()
{
    std::printf("value rows and dropdowns\n");
    NewPage();
    Measure("value row, one line", widgets::MakeValueRow(s_column, "Language", "English").root, 64);
    Measure("value row, a value cut with three stops", widgets::MakeValueRow(s_column, "Language", "English (United Kingdom)").root, 64);
    lv_obj_t *disabled = widgets::MakeValueRow(s_column, "Backup", "Off").root;
    lv_obj_add_state(disabled, LV_STATE_DISABLED);
    Measure("value row, disabled", disabled, 64);
    static const char *const kOptions[] = {"Never", "Every day", "Every week"};
    widgets::Dropdown *dropdown = widgets::MakeDropdown(s_column, "Repeat", kOptions, 3, 1);
    Measure("dropdown field", dropdown->field, 64);
    widgets::SetDropdownEnabled(dropdown, false, "Turn on repeats first");
    Measure("dropdown field, disabled", dropdown->field, 64);
}

void TestHomeRows()
{
    std::printf("home rows, radio rows and choice cards\n");
    NewPage();
    Measure("home row, a title alone", widgets::MakeListRow(s_column, "Call the plumber", nullptr, nullptr, nullptr), 80);
    Measure("home row, a title and a supporting line", widgets::MakeListRow(s_column, "Call the plumber", "To-do · Today, 8:12", nullptr, nullptr), 80);
    Measure("radio row", widgets::MakeRadioRow(s_column, "Medium"), 80);
    Measure("choice card, a name alone", widgets::MakeChoiceCard(s_column, "Idea", false, nullptr));
    Measure("choice card, a name over a detail", widgets::MakeChoiceCard(s_column, "Idea", false, "something to think about"));
    Measure("choice card, a long name over a detail", widgets::MakeChoiceCard(s_column, "Keep the recording after it is transcribed", true, "Only on your Wi-Fi"));
}

// A row of an answer's words wraps and is never cut (DESIGN → Components → The kit): a radio row's `spacing.list-row-height` is a floor
// with `spacing.xs` above and below, so one line and two both stand 80 px and three and four grow it; a detail row's name and value
// wrap. Every word stays whole — no label is ended with three stops — and the content stays centred on the row's height.
int LinesOf(lv_obj_t *label, ui::Type role)
{
    lv_obj_update_layout(label);
    return lv_obj_get_height(label) / static_cast<int>(ui::LineBox(role));  // each line stands on the role's line box
}

// The first label under `obj`, depth first: a row's words.
lv_obj_t *FirstLabel(lv_obj_t *obj)
{
    if (lv_obj_check_type(obj, &lv_label_class)) {
        return obj;
    }
    for (uint32_t i = 0; i < lv_obj_get_child_count(obj); ++i) {
        if (lv_obj_t *found = FirstLabel(lv_obj_get_child(obj, static_cast<int32_t>(i)))) {
            return found;
        }
    }
    return nullptr;
}

void TestWrappingRows()
{
    std::printf("wrapping radio rows and detail rows\n");
    NewPage();
    struct Case {
        const char *words;
        int lines;
        int height;
    };
    static const Case kCases[] = {
        {"Medium", 1, 80},
        {"Every two weeks, on a Monday", 2, 80},                       // 28 characters: two lines of 18 fit the 80 floor
        {"Every weekday except bank holidays", 3, 116},                 // 4 + 3 x 36 + 4
        {"On the first Monday of next month, in the morning", 4, 152},  // 4 + 4 x 36 + 4
    };
    for (const Case &c : kCases) {
        lv_obj_t *row = widgets::MakeRadioRow(s_column, c.words);
        Measure(std::string("radio row, ") + std::to_string(c.lines) + " line(s)", row, c.height);
        lv_obj_t *label = FirstLabel(row);
        Check(lv_label_get_long_mode(label) == LV_LABEL_LONG_MODE_WRAP, std::string("radio row words wrap, never dotted: ") + c.words);
        Check(LinesOf(label, ui::Type::kLabel) == c.lines, std::string("radio row takes ") + std::to_string(c.lines) + " line(s): " + c.words);
        widgets::SetRadioChosen(row, true);
        Measure(std::string("radio row, ") + std::to_string(c.lines) + " line(s), chosen", row, c.height);
    }
    lv_obj_t *disabled = widgets::MakeRadioRow(s_column, "Every two weeks, on a Monday");
    lv_obj_add_state(disabled, LV_STATE_DISABLED);
    Measure("radio row, two lines, disabled", disabled, 80);

    NewPage();
    const widgets::DetailRow one = widgets::MakeDetailRow(s_column, "Serial number", "FU-0001-2026");
    Measure("detail row, one line each", one.root, 76);  // 8 + 24 + 36 + 8, as it always was
    const widgets::DetailRow value = widgets::MakeDetailRow(s_column, "Firmware", "folloup 0.1.0-23-g4ae91c2-dirty");
    Measure("detail row, a two-line value", value.root, 76 + 36);
    Check(LinesOf(value.value, ui::Type::kLabel) == 2 && lv_label_get_long_mode(value.value) == LV_LABEL_LONG_MODE_WRAP, "detail row: a 31-character value takes two lines, whole");
    const widgets::DetailRow name = widgets::MakeDetailRow(s_column, "Wi-Fi network joined on the last boot", "Home");
    Measure("detail row, a two-line name", name.root, 76 + 24);
    Check(LinesOf(name.label, ui::Type::kCaption) == 2, "detail row: a 37-character name takes two lines, whole");
    const widgets::DetailRow both = widgets::MakeDetailRow(s_column, "Wi-Fi network joined on the last boot", "On the first Monday of next month, in the morning");
    Measure("detail row, a two-line name over a three-line value", both.root, 76 + 24 + 2 * 36);
}

// A list view shows only its own page's rows: its band is cut to the page's last row and the ring's reach under it, so the first row of
// the next page never shows in the room a tall row left (DESIGN → Layout → Paging).
void TestListViewPageCut()
{
    std::printf("a list view of wrapping radio rows\n");
    NewPage();
    constexpr int kReach = dt::kSpacingSm;
    const widgets::ListView view = widgets::MakeListView(s_column, 6, dt::kSpacingListRowHeight);
    static const char *const kRows[] = {"Never", "Every day", "Every two weeks, on a Monday", "Every week", "Every month",
                                        "On the first Monday of next month, in the morning", "Next week", "Only when I am on Wi-Fi"};
    for (const char *words : kRows) {
        widgets::MakeRadioRow(view.root, words);
    }
    widgets::UpdateListView(view);
    lv_obj_update_layout(s_screen);
    // 5 rows of 80, 80, 80, 80, 80 and gaps of 16 are 464; the 152 row would make 632, past the 560 of six nominal rows.
    Check(widgets::ListViewPages(view) == 2, "eight rows, one of them four lines, are two pages");
    const int first_rows[] = {0, 5};
    const int last_rows[] = {4, 7};
    for (int page = 0; page < 2; ++page) {
        lv_obj_t *first = lv_obj_get_child(view.root, first_rows[page]);
        lv_obj_t *last = lv_obj_get_child(view.root, last_rows[page]);
        if (page == 1) {
            lv_group_focus_obj(first);  // the focus leaving the page turns it
        }
        lv_obj_update_layout(s_screen);
        Check(widgets::ListViewPage(view) == page, "page " + std::to_string(page) + " is shown");
        lv_area_t band, a, z;
        lv_obj_get_coords(view.root, &band);
        lv_obj_get_coords(first, &a);
        lv_obj_get_coords(last, &z);
        const std::string what = "page " + std::to_string(page);
        Check(a.y1 - band.y1 == kReach, what + ": its first row stands the ring's reach below the band's top");
        Check(band.y2 - z.y2 == kReach, what + ": the band ends the ring's reach under its last row, no further");
        for (uint32_t i = 0; i < lv_obj_get_child_count(view.root); ++i) {
            lv_obj_t *row = lv_obj_get_child(view.root, static_cast<int32_t>(i));
            if (static_cast<int>(i) >= first_rows[page] && static_cast<int>(i) <= last_rows[page]) {
                continue;
            }
            lv_area_t r;
            lv_obj_get_coords(row, &r);
            const bool shows = r.y2 >= band.y1 && r.y1 <= band.y2;  // any of its pixels inside the band's clip
            Check(!shows, what + ": row " + std::to_string(i) + " of the other page is outside the band");
        }
    }
}

void TestNetworkRows()
{
    std::printf("Wi-Fi network rows\n");
    NewPage();
    using S = widgets::NetworkState;
    Measure("network row, open and weak", widgets::MakeNetworkRow(s_column, "CafeNet", false, 1, S::kRest), 80);
    Measure("network row, locked and strong", widgets::MakeNetworkRow(s_column, "Office", true, 3, S::kRest), 80);
    Measure("network row, a name cut with three stops", widgets::MakeNetworkRow(s_column, "Pretty Fly for a Wi-Fi (guest, 5 GHz)", true, 1, S::kRest), 80);
    Measure("network row, connected", widgets::MakeNetworkRow(s_column, "HomeNet", true, 3, S::kConnected), 80);
    Measure("network row, connecting", widgets::MakeNetworkRow(s_column, "Office", true, 2, S::kConnecting), 80);
    // The signal glyph stands alone, centred on the row's height at its right end, with no word under it (DESIGN → Exceptions).
    for (int signal = 0; signal <= 3; ++signal) {
        lv_obj_t *row = widgets::MakeNetworkRow(s_column, "Office", true, signal, S::kRest);
        lv_obj_update_layout(s_screen);
        lv_obj_t *glyph = lv_obj_get_child(row, lv_obj_get_child_count(row) - 1);  // the canvas holding the bars: the row's last child
        lv_area_t r, g;
        lv_obj_get_coords(row, &r);
        lv_obj_get_coords(glyph, &g);
        const std::string what = "signal " + std::to_string(signal);
        Check(std::abs((g.y1 + g.y2) - (r.y1 + r.y2)) <= 1, what + ": the glyph is centred on the row's height");
        Check(r.x2 - g.x2 == dt::kSpacingMd + dt::kSpacingStrokeDefault || r.x2 - g.x2 >= dt::kSpacingMd, what + ": at the row's right end, spacing.md in");
        bool word_under = false;
        for (uint32_t i = 0; i < lv_obj_get_child_count(glyph); ++i) {
            word_under = word_under || lv_obj_check_type(lv_obj_get_child(glyph, static_cast<int32_t>(i)), &lv_label_class);
        }
        Check(!word_under && lv_obj_get_child_count(row) == 3, what + ": no word under the glyph — the row holds the name, the facts and the glyph");
    }
    // In a list view's band, as the Wi-Fi screen holds them.
    const widgets::ListView view = widgets::MakeListView(s_column, 4, dt::kSpacingListRowHeight);
    for (int i = 0; i < 6; ++i) {
        widgets::MakeNetworkRow(view.root, i % 2 ? "Guest" : "Neighbours 5G", i % 3 == 0, 1 + i % 3, S::kRest);
    }
    widgets::UpdateListView(view);
    lv_obj_update_layout(s_screen);
    for (uint32_t i = 0; i < 6; ++i) {
        Measure("list view row " + std::to_string(i) + " (network row)", lv_obj_get_child(view.root, static_cast<int32_t>(i)), 80);
    }
}

void TestListViewValueRows()
{
    std::printf("a list view of value rows\n");
    NewPage();
    const widgets::ListView view = widgets::MakeListView(s_column, 4, dt::kSpacingTarget);
    for (int i = 1; i <= 6; ++i) {
        widgets::MakeValueRow(view.root, ("Follow-up " + std::to_string(i)).c_str(), i % 3 == 0 ? "Done" : "Open");
    }
    widgets::UpdateListView(view);
    lv_obj_update_layout(s_screen);
    for (uint32_t i = 0; i < 6; i += 5) {
        Measure("list view row " + std::to_string(i) + " (value row)", lv_obj_get_child(view.root, static_cast<int32_t>(i)), 64);
    }
}

// The select sheet's options: one line, two, and a third that grows the row; the chosen one with its check, and the others.
void TestSelectSheet()
{
    std::printf("the select sheet's options\n");
    NewPage();
    static const char *const kOptions[] = {
        "Never",                                                                        // one line
        "In an hour",
        "On the first Monday of next month",                                            // two lines (20 characters to a line)
        "On the first Monday of every month, in the morning",                           // three lines: the row grows
        "Tomorrow",
    };
    widgets::Dropdown *dropdown = widgets::MakeDropdown(s_column, "Remind me", kOptions, 5, 4);
    lv_obj_update_layout(s_screen);
    widgets::OpenSelectSheet(dropdown);
    lv_obj_update_layout(s_screen);
    Check(widgets::SelectSheetOpen(), "the select sheet opens");
    const uint32_t stops = lv_group_get_obj_count(s_group);
    Check(stops == 6, "the sheet's stops are its five rows and Cancel, and nothing behind it");
    const char *names[] = {"one line", "one line, short", "two lines", "three lines", "one line, the chosen one"};
    int heights[5] = {};
    for (uint32_t i = 0; i < 5 && i < stops; ++i) {
        lv_obj_t *row = lv_group_get_obj_by_index(s_group, i);
        heights[i] = lv_obj_get_height(row);
        Measure(std::string("sheet option, ") + names[i], row);
    }
    Check(heights[0] == 64 && heights[1] == 64 && heights[4] == 64, "a one-line option's row is the 64 px floor");
    Check(heights[2] == 2 * ui::LineBox(ui::Type::kBody) + 2 * dt::kSpacingSm, "a two-line option's row is 2 x 40 and 8 above and below");
    Check(heights[3] == 3 * ui::LineBox(ui::Type::kBody) + 2 * dt::kSpacingSm, "a three-line option grows its row to 3 x 40 and 8 above and below");
    widgets::CloseSelectSheet();
    Check(!widgets::SelectSheetOpen(), "the select sheet closes");
}

}  // namespace

// The sheet with the longest option the kit has — a three-line one — among short ones: the sheet keeps its foot on the glass, and
// every page shows only whole rows above the foot, however many of the long rows it holds.
void TestSelectSheetLongOption()
{
    std::printf("the select sheet with a three-line option\n");
    NewPage();
    static const char *const kOptions[] = {"Never", "In an hour", "On the first Monday of every month, in the morning", "Tomorrow",
                                           "On the first Monday of every month, in the morning, early", "Next week", "In 2 days"};
    widgets::Dropdown *dropdown = widgets::MakeDropdown(s_column, "Remind me", kOptions, 7, 2);
    lv_obj_update_layout(s_screen);
    widgets::OpenSelectSheet(dropdown);
    lv_obj_update_layout(s_screen);
    const uint32_t stops = lv_group_get_obj_count(s_group);
    lv_obj_t *cancel = lv_group_get_obj_by_index(s_group, stops - 1);
    lv_area_t foot = {};
    lv_obj_get_coords(cancel, &foot);
    Check(foot.y2 + dt::kSpacingFocusOffset + dt::kSpacingStrokeFocus < 800, "the sheet's Cancel and its ring stand on the glass");
    for (uint32_t focused = 0; focused + 1 < stops; ++focused) {
        router::Focus(lv_group_get_obj_by_index(s_group, focused));
        lv_obj_update_layout(s_screen);
        lv_obj_t *band = lv_obj_get_parent(lv_group_get_obj_by_index(s_group, 0));  // the list view's rows' parent
        lv_area_t cut;
        lv_obj_get_coords(band, &cut);
        int shown = 0;
        bool whole = true;
        for (uint32_t i = 0; i + 1 < stops; ++i) {
            lv_area_t a;
            lv_obj_get_coords(lv_group_get_obj_by_index(s_group, i), &a);
            if (a.y2 < cut.y1 || a.y1 > cut.y2) {
                continue;  // on another page
            }
            ++shown;
            whole = whole && a.y1 >= cut.y1 && a.y2 <= cut.y2;
        }
        std::printf("  focus on row %u: %d rows on its page, the band y %d..%d, the foot from y %d, whole rows: %s\n", focused, shown,
                    int(cut.y1), int(cut.y2), int(foot.y1), whole ? "yes" : "NO");
        Check(shown > 0 && whole && cut.y2 < foot.y1, "with focus on row " + std::to_string(focused) + " its page shows whole rows above the foot");
    }
    widgets::CloseSelectSheet();
}

int main()
{
    lv_init();
    lv_tick_set_cb(Ticks);
    lv_display_t *display = lv_display_create(480, 800);
    static uint8_t buffer[480 * 800];
    lv_display_set_color_format(display, LV_COLOR_FORMAT_L8);
    lv_display_set_buffers(display, buffer, nullptr, sizeof buffer, LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(display, Flush);
    lv_display_set_theme(display, nullptr);  // as the firmware: no LVGL theme is compiled in
    s_group = lv_group_create();
    lv_group_set_default(s_group);
    if (ui::Start() != ESP_OK) {
        std::printf("ui::Start failed\n");
        return 1;
    }
    TestSettingRows();
    TestValueRows();
    TestHomeRows();
    TestWrappingRows();
    TestListViewPageCut();
    TestNetworkRows();
    TestListViewValueRows();
    TestSelectSheet();
    TestSelectSheetLongOption();
    std::printf("row centring on LVGL: %d checks, %d failed\n", s_checks, s_failures);
    return s_failures == 0 ? 0 : 1;
}
