// Host test of slide-off and the escape chain's first link (components/router/touch.cpp, router.cpp; the range slider, the
// text view) on LVGL's real pointer, keypad, groups and scrolls, with the real widgets, on the computer with no board, each read
// going through the router's read hooks as lvgl_display's reads hand them over: a lift off a button past its hit area and
// `spacing.md` presses nothing, a lift inside the slop presses; a lift off the range slider past the slop leaves the value the
// press found, a lift on it lands one; a swipe on a list view still turns its page and presses no row; and holding OK while a
// control is open for editing puts back what it had when OK opened it — the slider's value, the text view's page — and closes
// it, before the chain closes an overlay or goes back (DESIGN → Focus and input; APP_ARCHITECTURE §7).
// run.sh builds LVGL's sources from managed_components once, into build/host/lvgl, and the sources named below, against stand-ins
// for the ESP-IDF headers (tests/host/stubs).
// host-sources: components/ui/ui.cpp components/ui/line_box.cpp components/ui/fonts/bitcount/*.c components/widgets/marks/marks.c components/widgets/src/*.cpp components/widgets/src/internal/*.cpp components/router/router.cpp components/router/overlays.cpp components/router/focus.cpp components/router/touch.cpp components/router/routes.cpp components/router/screen.cpp components/router/status.cpp
// Build and run: bash tests/host/run.sh

#include <algorithm>
#include <cstdio>
#include <deque>
#include <memory>
#include <string>

#include "design_tokens.h"
#include "lvgl.h"
#include "lvgl_display.h"
#include "router/router.h"
#include "ui.h"
#include "widgets/widgets.h"

namespace dt = design_tokens;

// --- The stand-ins for lvgl_display's side -----------------------------------------------------------------------------------

namespace {
lv_indev_t *s_keys = nullptr;
lv_indev_t *s_touch = nullptr;
lv_group_t *s_bound = nullptr;
void (*s_back)() = nullptr;
uint32_t s_ms = 0;
}  // namespace

namespace lvgl_display {
lv_group_t *Group() { return s_bound; }
void SetGroup(lv_group_t *group)
{
    s_bound = group;
    lv_indev_set_group(s_keys, group);
    lv_group_set_default(group);
}
void ClearGroup()
{
    lv_group_set_editing(s_bound, false);
    lv_group_remove_all_objs(s_bound);
}
void SetBackHandler(void (*back)()) { s_back = back; }
void (*s_key_hook)(lv_indev_t *, lv_indev_data_t *) = nullptr;
void (*s_touch_hook)(lv_indev_t *, lv_indev_data_t *) = nullptr;
void SetKeyReadHook(void (*hook)(lv_indev_t *, lv_indev_data_t *)) { s_key_hook = hook; }
void SetTouchReadHook(void (*hook)(lv_indev_t *, lv_indev_data_t *)) { s_touch_hook = hook; }
void SetPartialCounting(PartialCounting) {}
void NoteTyping() {}
void RequestFull() {}
// The at-end cue a widget asks for, counted: a step or a turn past an end asks it, one that moves never.
int s_at_ends = 0;
void PlayCue(Cue cue) { s_at_ends += cue == Cue::kAtEnd; }
}  // namespace lvgl_display

namespace {

int s_failures = 0;
int s_checks = 0;

void Check(bool ok, const char *what)
{
    ++s_checks;
    if (!ok) {
        ++s_failures;
        std::printf("FAIL  %s\n", what);
    }
}

void Flush(lv_display_t *display, const lv_area_t *, uint8_t *)
{
    lv_display_flush_ready(display);
}

uint32_t Ticks() { return s_ms; }

void Wait(uint32_t ms)
{
    s_ms += ms;
    lv_timer_handler();
}

// --- The inputs, through the router's hooks ----------------------------------------------------------------------------------

struct KeyRead {
    uint32_t key;
    bool pressed;
};
std::deque<KeyRead> s_key_reads;
uint32_t s_last_key = LV_KEY_ENTER;

void ReadKeys(lv_indev_t *indev, lv_indev_data_t *data)
{
    bool pressed = false;
    if (!s_key_reads.empty()) {
        s_last_key = s_key_reads.front().key;
        pressed = s_key_reads.front().pressed;
        s_key_reads.pop_front();
    }
    data->key = s_last_key;
    data->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    lvgl_display::s_key_hook(indev, data);
}

void Press(uint32_t key)
{
    s_key_reads.push_back({key, true});
    lv_indev_read(s_keys);
    s_key_reads.push_back({key, false});
    lv_indev_read(s_keys);
}

lv_point_t s_point = {0, 0};
bool s_finger = false;

void ReadTouch(lv_indev_t *indev, lv_indev_data_t *data)
{
    data->point = s_point;
    data->state = s_finger ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    lvgl_display::s_touch_hook(indev, data);
}

// One read of the finger at (x, y), 10 ms after the last, as the input watcher's polls are; LVGL's timers run after it.
void Touch(int32_t x, int32_t y, bool down)
{
    s_point = {x, y};
    s_finger = down;
    s_ms += 10;
    lv_indev_read(s_touch);
    lv_timer_handler();
}

// The finger goes down at `from`, moves to `to` in steps of `each` px, and lifts there.
void Slide(lv_point_t from, lv_point_t to, int32_t each)
{
    Touch(from.x, from.y, true);
    lv_point_t at = from;
    while (at.x != to.x || at.y != to.y) {
        at.x += to.x > at.x ? std::min(each, to.x - at.x) : -std::min(each, at.x - to.x);
        at.y += to.y > at.y ? std::min(each, to.y - at.y) : -std::min(each, at.y - to.y);
        Touch(at.x, at.y, true);
    }
    Touch(at.x, at.y, false);
    for (int i = 0; i < 6; ++i) {
        Touch(at.x, at.y, false);
    }
}

lv_area_t Coords(lv_obj_t *obj)
{
    lv_obj_update_layout(lv_obj_get_screen(obj));
    lv_area_t a;
    lv_obj_get_coords(obj, &a);
    return a;
}

// --- The page: a button, a slider, a text view and a list view ----------------------------------------------------------------

struct Page {
    lv_obj_t *root = nullptr;
    lv_obj_t *button = nullptr;
    widgets::RangeSlider slider{};
    widgets::TextView text{};
    widgets::ListView list{};
    lv_obj_t *rows[8] = {};
};
Page s_page;

int s_clicks = 0;
int s_lost = 0;
int s_row_clicks = 0;

void OnClicked(lv_event_t *) { ++s_clicks; }
void OnLost(lv_event_t *) { ++s_lost; }
void OnRowClicked(lv_event_t *) { ++s_row_clicks; }

class TestPage final : public router::Screen {
public:
    void OnCreate(lv_obj_t *root, lv_group_t *) override
    {
        ui::StyleScreen(root);
        s_page = Page{};
        s_page.root = root;
        lv_obj_t *column = lv_obj_create(root);
        lv_obj_remove_style_all(column);
        lv_obj_set_width(column, lv_pct(100));
        lv_obj_set_flex_grow(column, 1);
        lv_obj_set_flex_flow(column, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_hor(column, dt::kSpacingEdge, 0);
        lv_obj_set_style_pad_row(column, dt::kSpacingMd, 0);
        lv_obj_set_scrollable(column, false);
        s_page.button = widgets::MakeButton(column, "Save", widgets::ButtonVariant::kAction, dt::kSpacingButtonWidth);
        lv_obj_add_event_cb(s_page.button, OnClicked, LV_EVENT_CLICKED, nullptr);
        lv_obj_add_event_cb(s_page.button, OnLost, LV_EVENT_PRESS_LOST, nullptr);
        s_page.slider = widgets::MakeRangeSlider(column, 0, 100, 10, 50, "%");
        s_page.text = widgets::MakeTextView(column, 3,
                                            "One two three four five six seven eight nine ten eleven twelve thirteen fourteen "
                                            "fifteen sixteen seventeen eighteen nineteen twenty twenty-one twenty-two "
                                            "twenty-three twenty-four twenty-five twenty-six twenty-seven twenty-eight.");
        s_page.list = widgets::MakeListView(column, 2, dt::kSpacingTarget);
        for (int i = 0; i < 8; ++i) {
            s_page.rows[i] = widgets::MakeValueRow(s_page.list.root, "Follow-up", "Open").root;
            lv_obj_add_event_cb(s_page.rows[i], OnRowClicked, LV_EVENT_CLICKED, nullptr);
        }
        widgets::UpdateListView(s_page.list);
    }
};

// --- The tests --------------------------------------------------------------------------------------------------------------

void TestButtonSlideOff()
{
    const lv_area_t b = Coords(s_page.button);
    const int32_t cx = (b.x1 + b.x2) / 2;
    const int32_t cy = (b.y1 + b.y2) / 2;
    s_clicks = 0;
    Slide({cx, cy}, {cx, cy}, 1);
    Check(s_clicks == 1, "a tap on a button presses it");

    s_clicks = 0;
    s_lost = 0;
    Slide({cx, cy}, {b.x2 + dt::kSpacingMd - 2, cy}, 4);  // lifted just outside the button, inside the slop
    Check(s_clicks == 1 && s_lost == 0, "a lift inside the slop presses it");

    s_clicks = 0;
    Slide({cx, cy}, {cx, b.y2 + 3 * dt::kSpacingMd}, 4);  // well off it
    Check(s_clicks == 0, "a lift off the button, past the slop, presses nothing");
    Check(s_lost == 1, "and the button hears that its press was lost");
    Slide({cx, cy}, {cx, cy}, 1);
    Check(s_clicks == 1, "the next tap presses again: the contact was forgotten, not the button");
}

void TestSliderSlideOff()
{
    const lv_area_t t = Coords(s_page.slider.root);
    const int32_t cy = (t.y1 + t.y2) / 2;
    widgets::SetRangeSliderValue(s_page.slider, 50);
    // A lift on the track lands the value under the finger: a tenth of the way along.
    const int32_t tenth = t.x1 + (t.x2 - t.x1) / 10 + dt::kSpacingXs;
    Slide({tenth, cy}, {tenth, cy}, 1);
    Check(widgets::RangeSliderValue(s_page.slider) == 10, "a tap on the track lands its value");
    // Pressed at 10, slid sideways off the end and well past the slop: the value the press found.
    Slide({tenth, cy}, {t.x2, cy}, 8);
    Check(widgets::RangeSliderValue(s_page.slider) == 100, "a lift at the track's end lands the end");
    widgets::SetRangeSliderValue(s_page.slider, 30);
    Slide({tenth, cy}, {t.x1 + 20, t.y2 + 4 * dt::kSpacingMd}, 4);
    Check(widgets::RangeSliderValue(s_page.slider) == 30, "a lift off the slider past the slop leaves the value it had");
}

void TestListViewSwipeStays()
{
    const lv_area_t r = Coords(s_page.rows[0]);
    const int32_t x = (r.x1 + r.x2) / 2;
    s_row_clicks = 0;
    Slide({x, r.y2 - 4}, {x, r.y2 - 4 - 10 - dt::kTouchSwipeDistance - 20}, 4);  // past LVGL's scroll limit and the swipe distance
    Check(widgets::ListViewPage(s_page.list) == 1, "a swipe up on the list view still turns its page");
    Check(s_row_clicks == 0, "and presses no row");
}

// Opens `obj` for editing with OK, as the buttons do: the ring on it first, then OK.
void OpenEdit(lv_obj_t *obj)
{
    router::Ring(obj);
    Press(LV_KEY_ENTER);
}

void TestEditCancel()
{
    widgets::SetRangeSliderValue(s_page.slider, 40);
    OpenEdit(s_page.slider.root);
    Check(lv_group_get_editing(s_bound), "OK opens the slider for editing");
    Press(LV_KEY_UP);
    Press(LV_KEY_UP);
    Check(widgets::RangeSliderValue(s_page.slider) == 60, "UP raises it while it is open");
    Check(router::Escape(), "holding OK is taken");
    Check(widgets::RangeSliderValue(s_page.slider) == 40, "holding OK puts back the value it had when OK opened it");
    Check(!lv_group_get_editing(s_bound), "and closes it");
    Check(lv_group_get_focused(s_bound) == s_page.slider.root, "focus stays on the slider");

    OpenEdit(s_page.slider.root);
    Press(LV_KEY_UP);
    Press(LV_KEY_ENTER);
    Check(widgets::RangeSliderValue(s_page.slider) == 50 && !lv_group_get_editing(s_bound), "OK closes it and keeps the value");

    const int pages = widgets::TextViewPages(s_page.text);
    Check(pages >= 2, "the text view has pages to turn");
    OpenEdit(s_page.text.root);
    Press(LV_KEY_DOWN);
    Check(widgets::TextViewPage(s_page.text) == 1, "DOWN turns the text view's page while it is open");
    router::Escape();
    Check(widgets::TextViewPage(s_page.text) == 0 && !lv_group_get_editing(s_bound),
          "holding OK turns back to the page it opened on, and closes it");
}

// At an end, a step or a turn that changes nothing asks the at-end cue, after the press's own tick; one that moves never
// does (STICKY_DESIGN_TOKENS §5).
void TestAtEnd()
{
    widgets::SetRangeSliderValue(s_page.slider, 90);
    OpenEdit(s_page.slider.root);
    lvgl_display::s_at_ends = 0;
    Press(LV_KEY_UP);
    Check(widgets::RangeSliderValue(s_page.slider) == 100 && lvgl_display::s_at_ends == 0, "a step to the end is a step, not the end");
    Press(LV_KEY_UP);
    Press(LV_KEY_UP);
    Check(widgets::RangeSliderValue(s_page.slider) == 100 && lvgl_display::s_at_ends == 2, "each step past the top asks the at-end sound");
    Press(LV_KEY_DOWN);
    Check(widgets::RangeSliderValue(s_page.slider) == 90 && lvgl_display::s_at_ends == 2, "a step back down asks nothing");
    Press(LV_KEY_ENTER);
    widgets::SetRangeSliderValue(s_page.slider, 0);
    OpenEdit(s_page.slider.root);
    Press(LV_KEY_DOWN);
    Check(widgets::RangeSliderValue(s_page.slider) == 0 && lvgl_display::s_at_ends == 3, "and a step under the bottom asks it too");
    Press(LV_KEY_ENTER);

    const int pages = widgets::TextViewPages(s_page.text);
    OpenEdit(s_page.text.root);
    lvgl_display::s_at_ends = 0;
    Press(LV_KEY_UP);
    Check(widgets::TextViewPage(s_page.text) == 0 && lvgl_display::s_at_ends == 1, "UP on the text view's first page asks the at-end sound");
    for (int i = 0; i < pages - 1; ++i) {
        Press(LV_KEY_DOWN);
    }
    Check(widgets::TextViewPage(s_page.text) == pages - 1 && lvgl_display::s_at_ends == 1, "the turns to the last page ask nothing");
    Press(LV_KEY_DOWN);
    Check(widgets::TextViewPage(s_page.text) == pages - 1 && lvgl_display::s_at_ends == 2, "DOWN on its last page asks it");
    Press(LV_KEY_ENTER);

    // The list view, by touch: swipes down turn back to its first page, asking nothing; one more has nowhere to turn.
    lvgl_display::s_at_ends = 0;
    for (int guard = 0; guard < 8 && widgets::ListViewPage(s_page.list) > 0; ++guard) {
        const lv_area_t r = Coords(s_page.rows[2 * widgets::ListViewPage(s_page.list)]);  // the page's first row, in view
        const int32_t x = (r.x1 + r.x2) / 2;
        Slide({x, r.y1 + 4}, {x, r.y1 + 4 + 10 + dt::kTouchSwipeDistance + 20}, 4);
    }
    Check(widgets::ListViewPage(s_page.list) == 0 && lvgl_display::s_at_ends == 0, "swipes down that turn back ask nothing");
    const lv_area_t r = Coords(s_page.rows[0]);
    const int32_t x = (r.x1 + r.x2) / 2;
    Slide({x, r.y1 + 4}, {x, r.y1 + 4 + 10 + dt::kTouchSwipeDistance + 20}, 4);
    Check(widgets::ListViewPage(s_page.list) == 0 && lvgl_display::s_at_ends == 1,
          "a swipe down on the list's first page asks the at-end sound");
}

// The chain's order: an open edit first, then the top overlay, then back one screen.
void TestChainOrder()
{
    router::Push(std::make_unique<TestPage>());
    Check(router::Depth() == 2, "a page over home");
    lv_obj_t *page_slider = s_page.slider.root;
    router::Overlay overlay;
    router::OpenOverlay(&overlay);
    const widgets::RangeSlider in_overlay = widgets::MakeRangeSlider(s_page.root, 0, 100, 10, 20, "%");
    OpenEdit(in_overlay.root);
    Press(LV_KEY_UP);
    Check(router::Escape() && widgets::RangeSliderValue(in_overlay) == 20 && router::OverlayOpen() && router::Depth() == 2,
          "first holding OK cancels the edit, and the overlay stays");
    Check(router::Escape() && !router::OverlayOpen() && router::Depth() == 2, "then it closes the overlay");
    lv_obj_delete(in_overlay.root);
    Check(lv_group_get_focused(s_bound) != nullptr && s_bound != nullptr && page_slider != nullptr, "the page has the buttons again");
    Check(router::Escape() && router::Depth() == 1, "then it goes back");
    Wait(10);
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
    lv_display_set_theme(display, nullptr);
    s_keys = lv_indev_create();
    lv_indev_set_type(s_keys, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(s_keys, ReadKeys);
    lv_indev_set_mode(s_keys, LV_INDEV_MODE_EVENT);
    s_touch = lv_indev_create();
    lv_indev_set_type(s_touch, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(s_touch, ReadTouch);
    lv_indev_set_mode(s_touch, LV_INDEV_MODE_EVENT);
    if (ui::Start() != ESP_OK) {
        std::printf("ui::Start failed\n");
        return 1;
    }
    router::Init();
    router::SetRoot(std::make_unique<TestPage>());
    lv_obj_update_layout(s_page.root);
    TestButtonSlideOff();
    TestSliderSlideOff();
    TestListViewSwipeStays();
    TestEditCancel();
    TestAtEnd();
    TestChainOrder();
    std::printf("slide-off and edit cancel on LVGL: %d checks, %d failed\n", s_checks, s_failures);
    return s_failures == 0 ? 0 : 1;
}
