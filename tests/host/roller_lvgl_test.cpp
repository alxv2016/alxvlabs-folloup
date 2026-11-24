// Host test of the roller on LVGL's real objects, groups and input, on the computer with no board: OK opens it and UP and DOWN
// step it — UP to the value above — OK keeps and reports one change, holding OK and the footer's Cancel put the value back and
// report none, a circular roller wraps with no at-end cue and any other stops at its ends with one, the footer shows Cancel and the
// hint while it is open and the footer's parts again after, a tap steps and keeps, and one swipe steps once (STICKY_DESIGN_TOKENS §9).
// Beside it, the Clock screen's set (components/hw_test/clock_rules.h): the date written is the RTC's as read just before the write, and a
// failed set leaves the rollers at what the person chose.
// run.sh builds LVGL's sources from managed_components (a firmware build fetches them) into build/host/lvgl, once, and the
// sources named below, against stand-ins for the ESP-IDF headers (tests/host/stubs).
// host-sources: components/ui/ui.cpp components/ui/line_box.cpp components/ui/fonts/bitcount/*.c components/widgets/marks/marks.c components/widgets/src/*.cpp components/widgets/src/internal/*.cpp components/router/router.cpp components/router/overlays.cpp components/router/focus.cpp components/router/touch.cpp components/router/routes.cpp components/router/screen.cpp components/router/status.cpp
// Build and run: bash tests/host/run.sh

#include <cstdio>
#include <string>

#include "design_tokens.h"
#include "lvgl.h"
#include "lvgl_display.h"
#include "ui.h"
#include "widgets/widgets.h"
#include "../../components/hw_test/clock_rules.h"

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
// The at-end cue a widget asks for, counted: a step into the end of a list that stops asks it, a wrap never.
int s_at_ends = 0;
void PlayCue(Cue cue) { s_at_ends += cue == Cue::kAtEnd; }
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

constexpr uint32_t kReadMs = 33;
int s_x, s_y;
bool s_down;
bool s_ok_down;  // the OK key, for the keypad

void Flush(lv_display_t *display, const lv_area_t *, uint8_t *)
{
    lv_display_flush_ready(display);
}

void ReadPointer(lv_indev_t *, lv_indev_data_t *data)
{
    data->point.x = s_x;
    data->point.y = s_y;
    data->state = s_down ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

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

// --- The screen: two rollers and a footer with Back and the open bar -----------------------------------------------------

struct Screen {
    lv_obj_t *root;
    lv_obj_t *footer;
    lv_obj_t *back;
    lv_obj_t *bar;
    lv_obj_t *cancel;
    widgets::Roller hour;   // 0 to 23, circular, at 8
    widgets::Roller count;  // 1 to 12, stops, at 1
};

int s_changes;  // LV_EVENT_VALUE_CHANGED from either roller
void OnChanged(lv_event_t *) { ++s_changes; }

Screen s_screen;

void NewScreen()
{
    if (s_screen.root != nullptr) {
        lvgl_display::ClearGroup();
        lv_obj_t *blank = lv_obj_create(nullptr);
        lv_screen_load(blank);
        lv_obj_delete(s_screen.root);
    }
    Screen &s = s_screen;
    s.root = ui::NewScreen();
    lv_screen_load(s.root);
    lv_obj_t *column = lv_obj_create(s.root);
    lv_obj_remove_style_all(column);
    lv_obj_set_width(column, lv_pct(100));
    lv_obj_set_flex_grow(column, 1);
    lv_obj_set_flex_flow(column, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(column, dt::kSpacingEdge, 0);
    lv_obj_set_style_pad_column(column, dt::kSpacingLg, 0);
    lv_obj_set_scrollable(column, false);
    ui::LetRingOut(column);
    s.hour = widgets::MakeNumberRoller(column, "Hour", 0, 23, 1, 2, true, 8);
    s.count = widgets::MakeNumberRoller(column, "Count", 1, 12, 1, 1, false, 1);
    s.footer = lv_obj_create(s.root);
    lv_obj_remove_style_all(s.footer);
    lv_obj_set_size(s.footer, lv_pct(100), dt::kSpacingFooter);
    lv_obj_set_flex_flow(s.footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_scrollable(s.footer, false);
    s.back = widgets::MakeButton(s.footer, "Back", widgets::ButtonVariant::kOutline);
    s.bar = widgets::MakeRollerFooter(s.footer);
    s.cancel = lv_obj_get_child(s.bar, 0);
    widgets::SetRollerFooter(s.hour, s.bar);
    widgets::SetRollerFooter(s.count, s.bar);
    lv_obj_add_event_cb(s.hour.root, OnChanged, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_event_cb(s.count.root, OnChanged, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_update_layout(s.root);
    s_changes = 0;
    lvgl_display::s_at_ends = 0;
    s_down = s_ok_down = false;
    Tick();
}

// The three rows' words, top to bottom.
std::string Rows(const widgets::Roller &r)
{
    std::string out;
    for (int i = 1; i <= 3; ++i) {
        lv_obj_t *label = lv_obj_get_child(r.root, i);  // child 0 is the open band
        out += std::string(i > 1 ? "," : "") + lv_label_get_text(label);
    }
    return out;
}

bool Shown(lv_obj_t *obj)
{
    return !lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

// OK from the buttons: the keypad's press and release, the group editing as the shared toggle sets it.
void PressOk()
{
    s_ok_down = true;
    Tick();
    s_ok_down = false;
    Tick();
}

void Up() { lv_group_send_data(lvgl_display::Group(), LV_KEY_UP); }
void Down() { lv_group_send_data(lvgl_display::Group(), LV_KEY_DOWN); }

// Holding OK: the router's escape chain's first link, as router::Escape does it.
void HoldOk()
{
    lv_group_t *group = lvgl_display::Group();
    if (lv_group_get_editing(group)) {
        lv_obj_send_event(lv_group_get_focused(group), LV_EVENT_CANCEL, nullptr);
        lv_group_set_editing(group, false);
    }
    Tick();
}

lv_indev_t *s_keypad = nullptr;

void Focus(const widgets::Roller &r)
{
    lv_group_focus_obj(r.root);
    Tick();
}

void TestFace()
{
    NewScreen();
    lv_area_t a;
    lv_obj_get_coords(s_screen.hour.root, &a);
    Check(lv_area_get_width(&a) == 84 && lv_area_get_height(&a) == 192, "an hour roller is 84 x 192: two digits of 18 px and spacing.lg either side, three rows of 64");
    lv_obj_get_coords(s_screen.count.root, &a);
    Check(lv_area_get_width(&a) == 84, "a count of 1 to 12 is as wide: its widest value is two digits too");
    Check(Rows(s_screen.hour) == "07,08,09", "the hour 8 stands between 07 and 09");
    Check(Rows(s_screen.count) == ",1,2", "a count at its first value has a blank row above");
    Check(!Shown(s_screen.bar) && Shown(s_screen.back), "closed: the footer shows Back and not the bar");
    Check(!lv_obj_has_flag(lv_obj_get_child(s_screen.hour.root, 0), LV_OBJ_FLAG_CLICKABLE), "the open band takes no touch");
    // Cancel is a touch stop only: in no group, so UP and DOWN never reach it.
    Check(lv_obj_get_group(s_screen.cancel) == nullptr, "the footer's Cancel is in no focus group");
    Check(widgets::RollerValue(s_screen.hour) == 8 && widgets::RollerSelected(s_screen.hour) == 8, "the value and the index are 8");
}

void TestKeys()
{
    NewScreen();
    Focus(s_screen.hour);
    PressOk();
    Check(lv_group_get_editing(lvgl_display::Group()) && widgets::RollerOpen(s_screen.hour), "OK opens it for editing");
    Check(lv_obj_has_state(s_screen.hour.root, LV_STATE_EDITED), "and the group's editing state is on it");
    Check(Shown(lv_obj_get_child(s_screen.hour.root, 0)), "open: the middle row's inverted band shows");
    Check(Shown(s_screen.bar) && !Shown(s_screen.back), "open: the footer shows Cancel and the hint in Back's place");
    Up();
    Check(Rows(s_screen.hour) == "06,07,08", "UP steps to the value above: 8 becomes 7");
    Check(lv_obj_get_scroll_y(s_screen.hour.root) == 0, "UP does not also scroll the rows (LVGL's arrow scroll is off)");
    Down();
    Down();
    Check(lv_obj_get_scroll_y(s_screen.hour.root) == 0 && lv_obj_get_scroll_x(s_screen.hour.root) == 0, "nor does DOWN");
    Check(Rows(s_screen.hour) == "08,09,10" && widgets::RollerValue(s_screen.hour) == 9, "DOWN steps to the value below, twice: 9");
    Check(s_changes == 0, "no change is reported while it steps");
    PressOk();
    Check(!widgets::RollerOpen(s_screen.hour) && !lv_group_get_editing(lvgl_display::Group()), "OK keeps and closes");
    Check(s_changes == 1, "keeping reports one change");
    Check(Shown(s_screen.back) && !Shown(s_screen.bar), "closed: the footer shows Back again");
    Check(!Shown(lv_obj_get_child(s_screen.hour.root, 0)), "closed: the band is gone");
    Check(widgets::RollerValue(s_screen.hour) == 9, "the kept value is 9");

    // Open and keep with nothing changed: no event.
    PressOk();
    PressOk();
    Check(s_changes == 1, "keeping the value it opened on reports nothing");
}

// A press that takes the focus away — a tap on another control — closes the roller keeping the value, as the range slider's edit does
// (its value is live and only a cancel puts it back): one change reported, the footer's parts back.
void TestFocusAway()
{
    NewScreen();
    Focus(s_screen.hour);
    PressOk();
    Down();
    Down();
    lv_obj_update_layout(s_screen.root);
    lv_area_t a;
    lv_obj_get_coords(s_screen.count.root, &a);
    s_x = (a.x1 + a.x2) / 2;
    s_y = a.y1 + 100;
    s_down = true;
    Tick();
    s_down = false;
    Tick();
    Tick();
    Check(widgets::RollerOpen(s_screen.count), "a tap on another roller opens that one");
    Check(!widgets::RollerOpen(s_screen.hour) && widgets::RollerValue(s_screen.hour) == 10, "and the first closes, keeping the value it showed");
    Check(s_changes == 1, "reporting the one change");
    HoldOk();
    // The footer's parts: one that was hidden before the roller opened stays hidden after it closes.
    lv_obj_add_flag(s_screen.back, LV_OBJ_FLAG_HIDDEN);
    Focus(s_screen.hour);
    PressOk();
    Check(Shown(s_screen.bar) && !Shown(s_screen.back), "open: the bar shows");
    PressOk();
    Check(!Shown(s_screen.bar) && !Shown(s_screen.back), "closed: a part hidden before it opened is still hidden");
    lv_obj_remove_flag(s_screen.back, LV_OBJ_FLAG_HIDDEN);
}

void TestCancel()
{
    NewScreen();
    Focus(s_screen.hour);
    PressOk();
    Down();
    Down();
    Down();
    Check(widgets::RollerValue(s_screen.hour) == 11, "three steps: 11");
    HoldOk();
    Check(widgets::RollerValue(s_screen.hour) == 8, "holding OK puts back the value OK opened it on");
    Check(!widgets::RollerOpen(s_screen.hour) && !lv_group_get_editing(lvgl_display::Group()), "and closes it");
    Check(s_changes == 0, "a cancel reports no change");
    Check(Shown(s_screen.back) && !Shown(s_screen.bar), "the footer's parts are back");
    Check(Rows(s_screen.hour) == "07,08,09", "and the three rows show the old value");
    // Cancel keeps the roller focused: the next OK opens it again.
    PressOk();
    Check(widgets::RollerOpen(s_screen.hour), "the roller still has the focus: OK opens it again");
    Up();
    // The footer's Cancel, by touch.
    lv_area_t a;
    lv_obj_get_coords(s_screen.cancel, &a);
    lv_obj_update_layout(s_screen.root);
    lv_obj_get_coords(s_screen.cancel, &a);
    s_x = (a.x1 + a.x2) / 2;
    s_y = (a.y1 + a.y2) / 2;
    s_down = true;
    Tick();
    s_down = false;
    Tick();
    Tick();
    Check(widgets::RollerValue(s_screen.hour) == 8 && !widgets::RollerOpen(s_screen.hour), "a tap on the footer's Cancel puts the value back and closes");
    Check(s_changes == 0 && Shown(s_screen.back), "reporting nothing, with the footer's parts back");
    Check(lv_group_get_focused(lvgl_display::Group()) == s_screen.hour.root, "and focus stays on the roller");
}

void TestEnds()
{
    NewScreen();
    // A circular roller wraps with no at-end cue.
    Focus(s_screen.hour);
    PressOk();
    for (int i = 0; i < 8; ++i) {
        Up();
    }
    Check(Rows(s_screen.hour) == "23,00,01", "UP from 08 eight times reaches 00: 23 above it, 01 below");
    Check(lvgl_display::s_at_ends == 0, "a circular roller wraps with no at-end cue");
    Up();
    Check(widgets::RollerValue(s_screen.hour) == 23, "UP at 00 goes round to 23");
    PressOk();
    Check(s_changes == 1, "a wrapped value is one change");
    // A roller that stops sounds the cue for each step into an end, and changes nothing.
    Focus(s_screen.count);
    PressOk();
    Up();
    Check(widgets::RollerValue(s_screen.count) == 1 && lvgl_display::s_at_ends == 1, "UP at the first value stops and sounds at-end");
    for (int i = 0; i < 11; ++i) {
        Down();
    }
    Check(Rows(s_screen.count) == "11,12," && lvgl_display::s_at_ends == 1, "twelve is the last value, with a blank row below, and no cue yet");
    Down();
    Check(widgets::RollerValue(s_screen.count) == 12 && lvgl_display::s_at_ends == 2, "DOWN at the last value stops and sounds at-end");
    Down();
    Check(lvgl_display::s_at_ends == 3, "each step into the end sounds again, as a held key's repeats do");
    PressOk();
    Check(s_changes == 2, "one change for each roller kept");
}

void TestTouch()
{
    NewScreen();
    lv_area_t a;
    lv_obj_get_coords(s_screen.hour.root, &a);
    const int x = (a.x1 + a.x2) / 2;
    auto Tap = [&](int row) {
        s_x = x;
        s_y = a.y1 + row * 64 + 32;
        s_down = true;
        Tick();
        s_down = false;
        Tick();
        Tick();
    };
    Tap(0);
    Check(widgets::RollerOpen(s_screen.hour), "a tap on a closed roller opens it");
    Check(widgets::RollerValue(s_screen.hour) == 8, "and steps nothing");
    Check(Shown(s_screen.bar), "the footer shows Cancel");
    Tap(2);
    Check(widgets::RollerValue(s_screen.hour) == 9, "open: a tap on the row below steps down one");
    Tap(0);
    Tap(0);
    Check(widgets::RollerValue(s_screen.hour) == 7, "a tap on the row above steps up one, twice");
    Check(s_changes == 0 && widgets::RollerOpen(s_screen.hour), "stepping by touch reports nothing and leaves it open");
    Tap(1);
    Check(!widgets::RollerOpen(s_screen.hour) && s_changes == 1, "a tap on the middle row keeps: one change, closed");
    Check(widgets::RollerValue(s_screen.hour) == 7, "the kept value is 7");
    Check(Shown(s_screen.back), "the footer's Back is back");
}

// One swipe: the finger from `from_y` to `to_y` in `each` px a read, then lifted.
void Swipe(const widgets::Roller &r, int from_y, int to_y, int each)
{
    lv_area_t a;
    lv_obj_get_coords(r.root, &a);
    s_x = (a.x1 + a.x2) / 2;
    s_y = from_y;
    s_down = true;
    Tick();
    const int step = to_y < from_y ? -each : each;
    for (int y = from_y + step; step < 0 ? y >= to_y : y <= to_y; y += step) {
        s_y = y;
        Tick();
    }
    s_down = false;
    for (int i = 0; i < 6; ++i) {
        Tick();
    }
}

void TestSwipe()
{
    NewScreen();
    lv_area_t a;
    lv_obj_get_coords(s_screen.hour.root, &a);
    const int mid = (a.y1 + a.y2) / 2;
    // Closed: a swipe steps nothing.
    Swipe(s_screen.hour, mid + 40, mid - 60, 4);
    Check(widgets::RollerValue(s_screen.hour) == 8, "a swipe on a closed roller steps nothing");
    Focus(s_screen.hour);
    PressOk();
    Swipe(s_screen.hour, mid + 40, mid - 60, 4);  // up: the list moves up, the value below comes to the middle
    Check(widgets::RollerValue(s_screen.hour) == 9, "open: one swipe up steps once, to the value below");
    Swipe(s_screen.hour, mid - 60, mid + 40, 4);
    Check(widgets::RollerValue(s_screen.hour) == 8, "one swipe down steps once, to the value above");
    Swipe(s_screen.hour, mid + 60, mid - 60, 4);  // a long drag is still one step
    Check(widgets::RollerValue(s_screen.hour) == 9, "a long swipe is still one step");
    Check(s_changes == 0 && widgets::RollerOpen(s_screen.hour), "a swipe reports nothing and leaves it open");
    Check(lv_obj_get_scroll_y(s_screen.hour.root) == 0, "nothing follows the finger: the roller is not scrolled");
    HoldOk();
    Check(widgets::RollerValue(s_screen.hour) == 8, "holding OK puts back 8 after the swipes");
}

// The Clock screen's set: which moment is written, what is shown after it, and that a failed set leaves the chosen values.
void TestClockSet()
{
    namespace cr = hw_test::clock_rules;
    NewScreen();
    // The person chooses 15 and, by the wrap, 03 on the two rollers.
    Focus(s_screen.hour);
    PressOk();
    for (int i = 0; i < 7; ++i) {
        Down();
    }
    PressOk();
    Check(widgets::RollerValue(s_screen.hour) == 15, "the hour roller was kept at 15");

    std::tm read_before{};
    read_before.tm_year = 126;
    read_before.tm_mon = 10;
    read_before.tm_mday = 22;
    read_before.tm_wday = 0;
    std::tm today = read_before;  // the RTC has rolled over to the next day by the time Set is pressed
    today.tm_mday = 23;
    today.tm_wday = 1;
    std::tm written{};
    int writes = 0;
    auto read_today = [&](std::tm *out) { *out = today; return true; };
    auto write_ok = [&](const std::tm &when) { written = when; ++writes; return true; };

    cr::SetOutcome ok = cr::Set(read_today, write_ok, &read_before, 15, 3);
    Check(ok.ok && writes == 1, "a set writes once");
    Check(written.tm_mday == 23 && written.tm_wday == 1 && written.tm_mon == 10, "the date written is the RTC's now, read just before the write: today, not the day the screen opened");
    Check(written.tm_hour == 15 && written.tm_min == 3 && written.tm_sec == 0, "the hour and minute chosen, seconds at 0");
    Check(ok.shown.tm_mday == 23 && ok.dated, "the date is kept for the next set");

    // The clock will not read before the write: the last date read is kept; with none, 2026-01-01.
    auto read_none = [&](std::tm *) { return false; };
    cr::Set(read_none, write_ok, &read_before, 15, 3);
    Check(written.tm_mday == 22, "an unreadable clock falls back to the date last read");
    cr::Set(read_none, write_ok, nullptr, 15, 3);
    Check(written.tm_year == 126 && written.tm_mon == 0 && written.tm_mday == 1 && written.tm_wday == 4, "and with none, to Thursday 2026-01-01");

    // The write worked and the read-back does not: the screen shows the moment it wrote, never "not set" beside a success.
    int reads = 0;
    auto read_once = [&](std::tm *out) { if (reads++ == 0) { *out = today; return true; } return false; };
    cr::SetOutcome blind = cr::Set(read_once, write_ok, &read_before, 15, 3);
    Check(blind.ok && blind.shown.tm_hour == 15 && blind.shown.tm_min == 3, "a read-back that fails shows the time that was set");
    char words[40];
    cr::NowWords(words, sizeof words, blind.shown);
    Check(std::string(words) == "Now 15:03", "as \"Now 15:03\"");
    cr::SetWords(words, sizeof words, blind.shown);
    Check(std::string(words) == "Clock set to 15:03", "and \"Clock set to 15:03\" on the status line");

    // A failed write: no success, and the rollers keep what the person chose, not the clock's.
    auto write_fails = [&](const std::tm &) { return false; };
    cr::SetOutcome failed = cr::Set(read_today, write_fails, &read_before, widgets::RollerValue(s_screen.hour), 3);
    Check(!failed.ok, "a failed write is not a set");
    Check(widgets::RollerValue(s_screen.hour) == 15 && widgets::RollerSelected(s_screen.hour) == 15, "after a failed set the hour roller still reads 15, not the clock's 08");
    Check(!widgets::RollerOpen(s_screen.hour) && s_changes == 1, "and nothing else about it moved");
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
    lv_indev_t *pointer = lv_indev_create();
    lv_indev_set_type(pointer, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(pointer, ReadPointer);
    s_keypad = lv_indev_create();
    lv_indev_set_type(s_keypad, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(s_keypad, ReadKeypad);
    lv_indev_set_group(s_keypad, s_group);
    Tick();

    TestFace();
    TestKeys();
    TestFocusAway();
    TestCancel();
    TestEnds();
    TestTouch();
    TestSwipe();
    TestClockSet();

    std::printf("roller on LVGL: %d checks, %d failed\n", s_checks, s_failures);
    return s_failures == 0 ? 0 : 1;
}
