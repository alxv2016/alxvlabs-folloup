// Host test of a list view's pages of facts on LVGL's real objects, groups and input, on the computer with no board: a page with no
// row that takes the focus is one stop of the walk with the ring round its rows, in page order beside the real rows; DOWN and UP
// reach every page and the walk wraps with no key trap; leaving a stop turns the page as leaving a row does; OK on a stop does
// nothing; a page that has a row that takes focus has no stop of its own; and the page in words ("1 of 2") shows under a list view
// only when its screen asks, in one place on every page (DESIGN → Components → The kit, Layout → Paging; STICKY_DESIGN_TOKENS §9).
// run.sh builds LVGL's sources from managed_components (a firmware build fetches them) into build/host/lvgl, once, and the
// sources named below, against stand-ins for the ESP-IDF headers (tests/host/stubs).
// host-sources: components/ui/ui.cpp components/ui/line_box.cpp components/ui/fonts/bitcount/*.c components/widgets/marks/marks.c components/widgets/src/*.cpp components/widgets/src/internal/*.cpp components/router/router.cpp components/router/overlays.cpp components/router/focus.cpp components/router/touch.cpp components/router/routes.cpp components/router/screen.cpp components/router/status.cpp
// Build and run: bash tests/host/run.sh

#include <cstdio>
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
void SetGroup(lv_group_t *group)
{
    if (group == nullptr) {
        return;
    }
    s_group = group;
    for (lv_indev_t *indev = lv_indev_get_next(nullptr); indev != nullptr; indev = lv_indev_get_next(indev)) {
        lv_indev_set_group(indev, group);
    }
    lv_group_set_default(group);
}
void SetBackHandler(void (*back)()) { s_back = back; }
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

constexpr uint32_t kReadMs = 33;
bool s_ok_down;

void ReadKeypad(lv_indev_t *, lv_indev_data_t *data)
{
    data->key = LV_KEY_ENTER;
    data->state = s_ok_down ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

void Tick()
{
    lv_tick_inc(kReadMs);
    lv_timer_handler();
}

void PressOk()
{
    s_ok_down = true;
    Tick();
    s_ok_down = false;
    Tick();
}

// --- The screens -------------------------------------------------------------------------------------------------------

struct Screen {
    lv_obj_t *root = nullptr;
    lv_obj_t *column = nullptr;
    lv_obj_t *back = nullptr;
    widgets::ListView list{};
    std::vector<lv_obj_t *> rows;
};
Screen s_screen;
int s_pages_told;  // LV_EVENT_VALUE_CHANGED from the list: its page changed
void OnPage(lv_event_t *) { ++s_pages_told; }

void NewScreen()
{
    if (s_screen.root != nullptr) {
        lvgl_display::ClearGroup();
        lv_obj_t *blank = lv_obj_create(nullptr);
        lv_screen_load(blank);
        lv_obj_delete(s_screen.root);
    }
    s_screen = {};
    s_screen.root = ui::NewScreen();
    lv_screen_load(s_screen.root);
    s_screen.column = lv_obj_create(s_screen.root);
    lv_obj_remove_style_all(s_screen.column);
    lv_obj_set_width(s_screen.column, lv_pct(100));
    lv_obj_set_flex_grow(s_screen.column, 1);
    lv_obj_set_flex_flow(s_screen.column, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_hor(s_screen.column, dt::kSpacingEdge, 0);
    lv_obj_set_style_pad_ver(s_screen.column, dt::kSpacingMd, 0);
    lv_obj_set_style_pad_row(s_screen.column, dt::kSpacingMd, 0);
    lv_obj_set_scrollable(s_screen.column, false);
    ui::LetRingOut(s_screen.column);
    s_pages_told = 0;
}

// The footer's Back, added after the list so it follows it in the walk.
void EndScreen()
{
    lv_obj_t *footer = lv_obj_create(s_screen.root);
    lv_obj_remove_style_all(footer);
    lv_obj_set_size(footer, lv_pct(100), dt::kSpacingFooter);
    lv_obj_set_scrollable(footer, false);
    s_screen.back = ui::NewButton(footer, "Back", false, nullptr, nullptr);
    lv_obj_add_event_cb(s_screen.list.root, OnPage, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_update_layout(s_screen.root);
    Tick();
}

// An About screen of nine facts, one with a two-line value: five facts on the first page (76, 112, 76, 76, 76 and the gaps: 480),
// four on the second (352).
void AboutScreen(widgets::PageWords words)
{
    NewScreen();
    s_screen.list = widgets::MakeListView(s_screen.column, 6, 76, widgets::IndicatorCentre::kView, words);
    const char *const kFacts[][2] = {{"Product", "FolloUp"},          {"Firmware", "0.1.0-rc2+g3f9c1ab-dirty"},
                                     {"Built", "2026-11-23 14:02"},    {"Board", "reTerminal Sticky V2.0"},
                                     {"Serial number", "FU-0001-2026"}, {"Wi-Fi address", "7C:DF:A1:12:34:56"},
                                     {"ESP-IDF", "v6.1.1"},            {"Display", "SSD2677, 480 x 800"},
                                     {"Storage", "MicroSD, 29.7 GB free"}};
    for (const auto &fact : kFacts) {
        s_screen.rows.push_back(widgets::MakeDetailRow(s_screen.list.root, fact[0], fact[1]).root);
    }
    widgets::UpdateListView(s_screen.list);
    EndScreen();
}

// A mixed list: page 1 six value rows, page 2 five facts, page 3 a fact over two value rows.
void StorageScreen()
{
    NewScreen();
    s_screen.list = widgets::MakeListView(s_screen.column, 6, dt::kSpacingTarget, widgets::IndicatorCentre::kView, widgets::PageWords::kUnder);
    for (const char *name : {"Save to", "Keep for", "Delete when full", "Compress", "Language", "Transcribe"}) {
        s_screen.rows.push_back(widgets::MakeValueRow(s_screen.list.root, name, "On").root);
    }
    for (const char *name : {"Card", "Capacity", "Free", "Recordings", "Oldest"}) {
        s_screen.rows.push_back(widgets::MakeDetailRow(s_screen.list.root, name, "x").root);
    }
    s_screen.rows.push_back(widgets::MakeDetailRow(s_screen.list.root, "Format", "exFAT").root);
    for (const char *name : {"Check the card", "Eject the card"}) {
        s_screen.rows.push_back(widgets::MakeValueRow(s_screen.list.root, name, "").root);
    }
    widgets::UpdateListView(s_screen.list);
    EndScreen();
}

lv_obj_t *Focused()
{
    return lv_group_get_focused(s_group);
}

// What UP and DOWN do to the walk: DOWN is the next control, UP the one before (LV_KEY_NEXT and LV_KEY_PREV, as the buttons are
// mapped), the group wrapping at its ends.
void Down()
{
    lv_group_focus_next(s_group);
    Tick();
}
void Up()
{
    lv_group_focus_prev(s_group);
    Tick();
}

lv_area_t Coords(lv_obj_t *obj)
{
    lv_obj_update_layout(obj);
    lv_area_t a;
    lv_obj_get_coords(obj, &a);
    return a;
}
int Wide(const lv_area_t &a) { return lv_area_get_width(&a); }
int High(const lv_area_t &a) { return lv_area_get_height(&a); }

lv_obj_t *At(int index)
{
    return lv_group_get_obj_by_index(s_group, static_cast<uint32_t>(index));
}

// The stop's box is the page's rows: the first row's left and top, the rows' width, to the last row's bottom.
bool HugsRows(lv_obj_t *stop, lv_obj_t *first, lv_obj_t *last)
{
    const lv_area_t s = Coords(stop), a = Coords(first), b = Coords(last);
    return s.x1 == a.x1 && s.y1 == a.y1 && Wide(s) == Wide(a) && s.y2 == b.y2;
}

std::string Words()
{
    return s_screen.list.words != nullptr ? lv_label_get_text(s_screen.list.words) : "";
}

// --- The facts page ----------------------------------------------------------------------------------------------------

void TestFactsPageIsOneStopPerPage()
{
    AboutScreen(widgets::PageWords::kUnder);
    Check(widgets::ListViewPages(s_screen.list) == 2, "nine facts, one two lines, are two pages: five and four");
    // The group holds the two stops and Back: the facts are in none.
    Check(lv_group_get_obj_count(s_group) == 3, "a page with no row that takes focus is one stop: two stops and Back");
    lv_obj_t *page1 = At(0), *page2 = At(1);
    Check(At(2) == s_screen.back, "Back follows the stops in the walk");
    for (lv_obj_t *row : s_screen.rows) {
        Check(lv_obj_get_group(row) == nullptr, "a detail row is in no focus group");
    }
    Check(lv_obj_get_parent(page1) == s_screen.list.frame, "a stop is in the list's frame, not its band: the rows' indexes are the pages'");
    Check(!lv_obj_has_flag(page1, LV_OBJ_FLAG_CLICKABLE), "a stop takes no touch");
    Check(!lv_obj_has_flag(page1, LV_OBJ_FLAG_SCROLL_WITH_ARROW), "a stop's arrow scroll is off");
    Check(!lv_obj_has_flag(s_screen.list.root, LV_OBJ_FLAG_SCROLL_WITH_ARROW), "the band's arrow scroll is off: no key scrolls it");

    // Page 1 shows: its stop is the box of its five facts, the other has none.
    Check(widgets::ListViewPage(s_screen.list) == 0, "the first page shows");
    Check(HugsRows(page1, s_screen.rows[0], s_screen.rows[4]), "page 1's stop is the box of its facts, first row's top to last row's bottom");
    lv_area_t box = Coords(page1);
    Check(Wide(box) == dt::kSpacingContent && High(box) == 480, "page 1's box is 432 x 480");
    Check(Wide(Coords(page2)) == 0 && High(Coords(page2)) == 0, "page 2's stop has no box while page 1 shows: none of it can be in view");
    Check(lv_obj_get_style_radius(page1, LV_PART_MAIN) == dt::kRoundedLg, "the stop's corners are rounded.lg, as a row's");

    // The ring is the ordinary one: the same outline as a button's, 4 px wide and 4 px out.
    lv_group_focus_obj(page1);
    lv_obj_add_state(page1, LV_STATE_FOCUS_KEY);
    lv_obj_add_state(s_screen.back, LV_STATE_FOCUS_KEY);
    Check(lv_obj_get_style_outline_width(page1, LV_PART_MAIN) == dt::kSpacingStrokeFocus, "the ring is spacing.stroke.focus wide");
    Check(lv_obj_get_style_outline_width(page1, LV_PART_MAIN) == lv_obj_get_style_outline_width(s_screen.back, LV_PART_MAIN),
          "as wide as a control's ring");
    Check(lv_obj_get_style_outline_pad(page1, LV_PART_MAIN) == lv_obj_get_style_outline_pad(s_screen.back, LV_PART_MAIN),
          "as far out as a control's ring");
    Check(lv_color_to_u32(lv_obj_get_style_outline_color(page1, LV_PART_MAIN)) ==
              lv_color_to_u32(lv_obj_get_style_outline_color(s_screen.back, LV_PART_MAIN)),
          "the same colour as a control's ring");
    lv_obj_remove_state(s_screen.back, LV_STATE_FOCUS_KEY);
    lv_obj_remove_state(page1, LV_STATE_FOCUS_KEY);
}

void TestFactsWalk()
{
    AboutScreen(widgets::PageWords::kUnder);
    lv_obj_t *page1 = At(0), *page2 = At(1), *back = s_screen.back;
    lv_group_focus_obj(page1);
    Tick();
    Check(Focused() == page1 && widgets::ListViewPage(s_screen.list) == 0, "focus on page 1's stop, page 1 showing");

    s_pages_told = 0;
    Down();
    Check(Focused() == page2, "DOWN from page 1's stop reaches page 2's stop");
    Check(widgets::ListViewPage(s_screen.list) == 1, "and turns to page 2");
    Check(s_pages_told == 1, "the turn is reported once");
    Check(HugsRows(page2, s_screen.rows[5], s_screen.rows[8]), "page 2's ring hugs its four facts, not the room left below them");
    Check(High(Coords(page2)) == 352, "page 2's box is 432 x 352");
    Check(High(Coords(page1)) == 0, "page 1's stop has no box now");
    Check(lv_obj_has_state(page2, LV_STATE_FOCUSED), "page 2's stop is the focused one");

    Down();
    Check(Focused() == back && widgets::ListViewPage(s_screen.list) == 1, "DOWN from the last stop reaches Back and leaves page 2 on the glass");
    s_pages_told = 0;
    Down();
    Check(Focused() == page1 && widgets::ListViewPage(s_screen.list) == 0, "DOWN from Back wraps to page 1's stop, a page turn");
    Check(s_pages_told == 1, "the wrap's turn is reported");

    Up();
    Check(Focused() == back && widgets::ListViewPage(s_screen.list) == 0, "UP from page 1's stop wraps to Back");
    Up();
    Check(Focused() == page2 && widgets::ListViewPage(s_screen.list) == 1, "UP from Back reaches page 2's stop and turns to it");
    Up();
    Check(Focused() == page1 && widgets::ListViewPage(s_screen.list) == 0, "UP from page 2's stop turns back to page 1");
    Check(HugsRows(page1, s_screen.rows[0], s_screen.rows[4]), "page 1's ring hugs its five facts again");

    // No key trap: DOWN alone visits every control and every page, and comes round.
    std::vector<lv_obj_t *> seen;
    bool pages[2] = {false, false};
    for (int i = 0; i < 3; ++i) {
        Down();
        seen.push_back(Focused());
        pages[widgets::ListViewPage(s_screen.list)] = true;
    }
    Check(seen[0] == page2 && seen[1] == back && seen[2] == page1, "DOWN three times visits page 2, Back and page 1 and is where it began");
    Check(pages[0] && pages[1], "the walk showed both pages");
}

void TestOkOnlyTicks()
{
    AboutScreen(widgets::PageWords::kUnder);
    lv_obj_t *page1 = At(0);
    lv_group_focus_obj(page1);
    Tick();
    int clicks = 0;
    lv_obj_add_event_cb(page1, [](lv_event_t *e) { ++*static_cast<int *>(lv_event_get_user_data(e)); }, LV_EVENT_VALUE_CHANGED, &clicks);
    s_pages_told = 0;
    PressOk();
    PressOk();
    Check(Focused() == page1, "OK on a facts page leaves the focus where it is");
    Check(widgets::ListViewPage(s_screen.list) == 0 && s_pages_told == 0, "OK on a facts page turns nothing");
    Check(!lv_group_get_editing(s_group), "OK opens no mode: the group is not editing");
    Check(clicks == 0, "OK sends no change");
}

// LVGL scrolls a scrollable object a quarter of its height on UP and DOWN when it takes the key and keeps
// LV_OBJ_FLAG_SCROLL_WITH_ARROW. The key is sent where that would happen — the band itself, and the stop — at the first page and
// at the second: nothing moves, and with the flag put back on the band the same keys do move it (the check is not vacuous).
void TestNoArrowScroll()
{
    AboutScreen(widgets::PageWords::kUnder);
    lv_obj_t *band = s_screen.list.root;
    auto Send = [](lv_obj_t *to, uint32_t key) { lv_obj_send_event(to, LV_EVENT_KEY, &key); };
    Check(lv_obj_is_scrollable(band), "the band is scrollable: its rows are taller than its view, so a key could scroll it");
    const int32_t at_first = lv_obj_get_scroll_y(band);
    Send(band, LV_KEY_DOWN);
    Send(band, LV_KEY_DOWN);
    Check(lv_obj_get_scroll_y(band) == at_first, "DOWN sent to the band does not scroll it");
    Down();
    Down();  // to Back, leaving page 2 showing
    Up();
    const int32_t at_second = lv_obj_get_scroll_y(band);
    Check(widgets::ListViewPage(s_screen.list) == 1 && at_second > at_first, "page 2 stands further down the band");
    Send(band, LV_KEY_UP);
    Send(band, LV_KEY_UP);
    Check(lv_obj_get_scroll_y(band) == at_second, "UP sent to the band does not scroll it");
    Send(At(1), LV_KEY_DOWN);
    Send(At(1), LV_KEY_UP);
    Check(lv_obj_get_scroll_y(band) == at_second && widgets::ListViewPage(s_screen.list) == 1, "keys sent to the stop leave the band and the page alone");

    // The same keys with the flag put back: LVGL scrolls it. This is what the check above guards against.
    lv_obj_add_flag(band, LV_OBJ_FLAG_SCROLL_WITH_ARROW);
    Send(band, LV_KEY_UP);
    Check(lv_obj_get_scroll_y(band) != at_second, "with LV_OBJ_FLAG_SCROLL_WITH_ARROW back on, UP does scroll the band");
    lv_obj_remove_flag(band, LV_OBJ_FLAG_SCROLL_WITH_ARROW);
}

// UpdateListView again, after the footer is in the group: the stops are made anew in the same places and Back stays last.
void TestUpdateAgain()
{
    AboutScreen(widgets::PageWords::kUnder);  // two stops, then Back
    for (const char *name : {"Extra one", "Extra two", "Extra three", "Extra four"}) {
        widgets::MakeDetailRow(s_screen.list.root, name, "x");
    }
    widgets::UpdateListView(s_screen.list);  // thirteen facts: three pages
    Check(widgets::ListViewPages(s_screen.list) == 3, "thirteen facts are three pages");
    Check(lv_group_get_obj_count(s_group) == 4, "three stops and Back: the old stops are gone, not left behind");
    Check(At(3) == s_screen.back, "Back is still last: the stops came in before it, not after");
    for (int i = 0; i < 3; ++i) {
        Check(lv_obj_get_parent(At(i)) == s_screen.list.frame, "the first three are the list's stops, in page order");
    }
    lv_group_focus_obj(At(0));
    Tick();
    Down();
    Down();
    Check(widgets::ListViewPage(s_screen.list) == 2 && Focused() == At(2), "DOWN twice reaches the third page's stop");
    Down();
    Check(Focused() == s_screen.back, "and the next is Back");
    widgets::UpdateListView(s_screen.list);  // again, with nothing changed
    Check(lv_group_get_obj_count(s_group) == 4 && At(3) == s_screen.back, "a second call with nothing changed keeps the same walk");

    // A mixed list keeps its order by its rows.
    StorageScreen();
    widgets::UpdateListView(s_screen.list);
    Check(lv_group_get_obj_count(s_group) == 10 && lv_obj_get_parent(At(6)) == s_screen.list.frame && At(7) == s_screen.rows[12] && At(9) == s_screen.back,
          "a mixed list's stop is still between page 1's rows and page 3's after a second call");
}

// Deleting a stop that has the focus hands the focus to another stop of the same list; that must not walk the stops being deleted.
// No footer: the list's stops are the whole group.
void TestUpdateWithStopFocused()
{
    NewScreen();
    s_screen.list = widgets::MakeListView(s_screen.column, 6, 76, widgets::IndicatorCentre::kView, widgets::PageWords::kUnder);
    for (int i = 0; i < 9; ++i) {
        widgets::MakeDetailRow(s_screen.list.root, "Fact", "x");
    }
    widgets::UpdateListView(s_screen.list);
    lv_obj_update_layout(s_screen.root);
    Check(lv_group_get_obj_count(s_group) == 2, "two stops and nothing else in the group");
    lv_group_focus_obj(At(1));
    Tick();
    Check(widgets::ListViewPage(s_screen.list) == 1 && Focused() == At(1), "the second stop is focused, page 2 showing");
    widgets::UpdateListView(s_screen.list);
    Check(lv_group_get_obj_count(s_group) == 2, "after the stops are made anew there are still two");
    Check(widgets::ListViewPage(s_screen.list) == 0, "the list is back on its first page");
    Check(lv_obj_get_parent(At(0)) == s_screen.list.frame && lv_obj_get_parent(At(1)) == s_screen.list.frame, "both are the new stops");
    Check(High(Coords(At(0))) == 536 && High(Coords(At(1))) == 0, "the first stop has page 1's box, the second none");
    Down();
    Check(widgets::ListViewPage(s_screen.list) == 1 && High(Coords(At(1))) == 260, "DOWN walks the new stops");
}

void TestPageWords()
{
    AboutScreen(widgets::PageWords::kUnder);
    Check(s_screen.list.words != nullptr && !lv_obj_has_flag(s_screen.list.words, LV_OBJ_FLAG_HIDDEN), "asked for, two pages show their words");
    Check(Words() == "1 of 2", "page 1 says '1 of 2'");
    const lv_area_t first = Coords(s_screen.list.words);
    const lv_area_t frame = Coords(s_screen.list.frame);
    Check(first.x1 == frame.x1, "the words stand at the rows' left edge");
    Check(first.y1 == frame.y1 + 6 * 76 + 5 * dt::kSpacingMd + dt::kSpacingMd,
          "spacing.md under the view's whole height (the page's six rows), not under the page's rows");
    Check(High(frame) == 6 * 76 + 5 * dt::kSpacingMd + dt::kSpacingMd + ui::LineBox(ui::Type::kCaption), "the frame holds the view, the gap and the words' line");
    Down();
    Check(Words() == "2 of 2", "page 2 says '2 of 2'");
    const lv_area_t second = Coords(s_screen.list.words);
    Check(second.x1 == first.x1 && second.y1 == first.y1 && Wide(second) == Wide(first) && High(second) == High(first),
          "the words stand in one place on every page: a turn redraws the same box");

    AboutScreen(widgets::PageWords::kNone);
    Check(s_screen.list.words == nullptr, "not asked for, a list view says no page in words");
    Check(High(Coords(s_screen.list.frame)) == 6 * 76 + 5 * dt::kSpacingMd, "and the frame is the view's height, as it was");

    // One page needs none.
    NewScreen();
    s_screen.list = widgets::MakeListView(s_screen.column, 6, 76, widgets::IndicatorCentre::kView, widgets::PageWords::kUnder);
    for (const char *name : {"Product", "Board", "ESP-IDF"}) {
        widgets::MakeDetailRow(s_screen.list.root, name, "x");
    }
    widgets::UpdateListView(s_screen.list);
    EndScreen();
    Check(widgets::ListViewPages(s_screen.list) == 1 && lv_obj_has_flag(s_screen.list.words, LV_OBJ_FLAG_HIDDEN),
          "a list of one page hides its words even when asked");
    Check(High(Coords(s_screen.list.frame)) == 6 * 76 + 5 * dt::kSpacingMd, "and keeps the view's height");
    Check(lv_group_get_obj_count(s_group) == 2, "a facts page alone is still one stop: its stop and Back");
}

// --- A mixed list ------------------------------------------------------------------------------------------------------

void TestMixedList()
{
    StorageScreen();
    Check(widgets::ListViewPages(s_screen.list) == 3, "six value rows, five facts, a fact and two value rows are three pages");
    // The walk: six rows, page 2's stop, the two rows of page 3, Back. Page 3 is mixed: it has no stop of its own.
    Check(lv_group_get_obj_count(s_group) == 10, "six rows, one stop, two rows and Back: ten controls");
    lv_obj_t *stop = At(6);
    Check(lv_obj_get_parent(stop) == s_screen.list.frame, "the one stop is in the group between the rows of page 1 and of page 3");
    for (int i = 0; i < 6; ++i) {
        Check(At(i) == s_screen.rows[static_cast<size_t>(i)], "page 1's rows come first, in order");
    }
    Check(At(7) == s_screen.rows[12] && At(8) == s_screen.rows[13], "page 3's value rows follow the stop, in order");
    Check(At(9) == s_screen.back, "Back is last");
    Check(lv_obj_get_group(s_screen.rows[11]) == nullptr, "page 3's fact is in no group: it shows with the page");

    lv_group_focus_obj(s_screen.rows[5]);
    Tick();
    Check(widgets::ListViewPage(s_screen.list) == 0 && Words() == "1 of 3", "the last row of page 1 is on page 1");
    Down();
    Check(Focused() == stop && widgets::ListViewPage(s_screen.list) == 1, "DOWN from page 1's last row reaches page 2's stop and turns to page 2");
    Check(Words() == "2 of 3", "page 2 says '2 of 3'");
    Check(HugsRows(stop, s_screen.rows[6], s_screen.rows[10]), "the ring hugs page 2's five facts");
    Check(High(Coords(stop)) == 5 * 76 + 4 * dt::kSpacingMd, "five facts and the gaps between them: 444");
    Down();
    Check(Focused() == s_screen.rows[12] && widgets::ListViewPage(s_screen.list) == 2, "DOWN from the stop reaches page 3's first value row, and page 3 shows");
    Check(High(Coords(stop)) == 0, "the stop has no box while page 3 shows");
    // The fact above the row shows with the page.
    const lv_area_t band = Coords(s_screen.list.root);
    const lv_area_t fact = Coords(s_screen.rows[11]);
    Check(fact.y1 >= band.y1 && fact.y2 <= band.y2, "page 3's fact is inside the band, shown with the page");
    Down();
    Check(Focused() == s_screen.rows[13] && widgets::ListViewPage(s_screen.list) == 2, "DOWN reaches page 3's last row");
    Down();
    Check(Focused() == s_screen.back, "DOWN reaches Back");
    Down();
    Check(Focused() == s_screen.rows[0] && widgets::ListViewPage(s_screen.list) == 0, "DOWN from Back wraps to the first row and page 1");

    // Every control, every page, both ways.
    bool pages[3] = {false, false, false};
    int visited = 0;
    for (int i = 0; i < 10; ++i) {
        Down();
        ++visited;
        pages[widgets::ListViewPage(s_screen.list)] = true;
    }
    Check(visited == 10 && Focused() == s_screen.rows[0], "ten presses of DOWN come round to where they began");
    Check(pages[0] && pages[1] && pages[2], "the keys reached every page");
    Up();
    Check(Focused() == s_screen.back, "UP from the first row wraps to Back");
    Up();
    Up();
    Up();
    Check(Focused() == s_screen.rows[12] || Focused() == s_screen.rows[13] || Focused() == stop, "UP walks back through page 3");
    Check(Focused() == stop && widgets::ListViewPage(s_screen.list) == 1, "Back, row 14, row 13 and then the stop: three UPs back reach the stop and page 2");
    Up();
    Check(Focused() == s_screen.rows[5] && widgets::ListViewPage(s_screen.list) == 0, "UP from page 2's stop reaches the last row of page 1 and turns to it");
}

// A list whose rows are all in the group has no stops: the walk is the rows' and Back's, as it always was.
void TestRowsOnly()
{
    NewScreen();
    s_screen.list = widgets::MakeListView(s_screen.column, 4, dt::kSpacingTarget);
    for (int i = 0; i < 10; ++i) {
        widgets::MakeValueRow(s_screen.list.root, "Row", "On");
    }
    widgets::UpdateListView(s_screen.list);
    EndScreen();
    Check(lv_group_get_obj_count(s_group) == 11, "ten rows and Back: no stop is added to a list of rows");
    Check(s_screen.list.words == nullptr, "and no words unless asked");
    Check(widgets::ListViewPages(s_screen.list) == 3, "four rows a page make three pages of ten");
}

}  // namespace

int main()
{
    lv_init();
    static uint8_t buf[480 * 40 * 4];
    lv_display_t *display = lv_display_create(480, 800);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_L8);
    lv_display_set_theme(display, nullptr);  // as the firmware: no LVGL theme is compiled in
    s_group = lv_group_create();
    lv_group_set_default(s_group);
    if (ui::Start() != ESP_OK) {
        std::printf("ui::Start failed\n");
        return 1;
    }
    lv_display_set_buffers(display, buf, nullptr, sizeof buf, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, Flush);
    lv_indev_t *keypad = lv_indev_create();
    lv_indev_set_type(keypad, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(keypad, ReadKeypad);
    lv_indev_set_group(keypad, s_group);
    Tick();

    TestFactsPageIsOneStopPerPage();
    TestFactsWalk();
    TestOkOnlyTicks();
    TestNoArrowScroll();
    TestUpdateAgain();
    TestUpdateWithStopFocused();
    TestPageWords();
    TestMixedList();
    TestRowsOnly();

    std::printf("facts pages on LVGL: %d checks, %d failed\n", s_checks, s_failures);
    return s_failures == 0 ? 0 : 1;
}
