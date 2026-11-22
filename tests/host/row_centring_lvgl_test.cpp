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
// host-sources: components/ui/ui.cpp components/ui/line_box.cpp components/ui/fonts/bitcount/*.c components/widgets/marks/marks.c components/widgets/src/*.cpp components/widgets/src/internal/*.cpp
// Build and run: bash tests/host/run.sh

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "design_tokens.h"
#include "lvgl.h"
#include "lvgl_display.h"
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
void SetBackHandler(void (*back)()) { s_back = back; }
void (*BackHandler())() { return s_back; }
void SetPartialCounting(PartialCounting) {}
void NoteTyping() {}
void RequestFull() {}
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
    Measure("home row, a title alone", ui::NewRow(s_column, "Call the plumber", nullptr, nullptr, nullptr), 80);
    Measure("home row, a title and a supporting line", ui::NewRow(s_column, "Call the plumber", "To-do · Today, 8:12", nullptr, nullptr), 80);
    Measure("radio row", widgets::MakeRadioRow(s_column, "Medium"), 80);
    Measure("choice card, a name alone", widgets::MakeChoiceCard(s_column, "Idea", false, nullptr));
    Measure("choice card, a name over a detail", widgets::MakeChoiceCard(s_column, "Idea", false, "something to think about"));
    Measure("choice card, a long name over a detail", widgets::MakeChoiceCard(s_column, "Keep the recording after it is transcribed", true, "Only on your Wi-Fi"));
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
        "On the first Monday of next month, in the morning",                            // two lines (30 characters to a line)
        "On the first Monday of every month, in the morning, whether or not it is raining",  // three lines: the row grows
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
    Check(heights[2] == 64, "a two-line option fits the 64 px floor: 2 x 24 and 8 above and below");
    Check(heights[3] == 3 * 24 + 2 * dt::kSpacingSm, "a three-line option grows its row to 3 x 24 and 8 above and below");
    widgets::CloseSelectSheet();
    Check(!widgets::SelectSheetOpen(), "the select sheet closes");
}

}  // namespace

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
    TestNetworkRows();
    TestListViewValueRows();
    TestSelectSheet();
    std::printf("row centring on LVGL: %d checks, %d failed\n", s_checks, s_failures);
    return s_failures == 0 ? 0 : 1;
}
