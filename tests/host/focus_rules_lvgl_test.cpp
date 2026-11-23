// Host test of the focus rules (components/router/focus.cpp, touch.cpp) on LVGL's real keypad, pointer, groups and objects, on
// the computer with no board, each read going through the router's read hooks as lvgl_display's reads hand them over: with no
// ring on the glass the first UP, DOWN or OK shows the ring — on the focused control, or the first in view when that is off the
// glass — and moves nothing and presses nothing; the next acts; a tap presses, shows no ring and drops the one showing, keeping
// the place; a control open for editing takes its keys at once; the ring and the place come back across a Pop and an overlay's
// close, ringed only if they were; and the hand-focus helpers ring as the input that moved focus does (DESIGN → Focus and input;
// APP_ARCHITECTURE §7).
// run.sh builds LVGL's sources from managed_components once, into build/host/lvgl, and the sources named below, against stand-ins
// for the ESP-IDF headers (tests/host/stubs).
// host-sources: components/router/router.cpp components/router/overlays.cpp components/router/focus.cpp components/router/touch.cpp components/router/routes.cpp components/router/screen.cpp
// Build and run: bash tests/host/run.sh

#include <cstdio>
#include <deque>
#include <memory>
#include <string>

#include "lvgl.h"
#include "lvgl_display.h"
#include "router/router.h"

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
void SetBackHandler(void (*back)()) { s_back = back; }
void (*s_key_hook)(lv_indev_t *, lv_indev_data_t *) = nullptr;
void (*s_touch_hook)(lv_indev_t *, lv_indev_data_t *) = nullptr;
void SetKeyReadHook(void (*hook)(lv_indev_t *, lv_indev_data_t *)) { s_key_hook = hook; }
void SetTouchReadHook(void (*hook)(lv_indev_t *, lv_indev_data_t *)) { s_touch_hook = hook; }
void RequestFull() {}
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

// --- The inputs: each read filled as lvgl_display's would be, then handed to the router's hook, as lvgl_display does ------------

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
    if (lvgl_display::s_key_hook != nullptr) {
        lvgl_display::s_key_hook(indev, data);
    }
}

lv_point_t s_point = {0, 0};
bool s_finger = false;

void ReadTouch(lv_indev_t *indev, lv_indev_data_t *data)
{
    data->point = s_point;
    data->state = s_finger ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    if (lvgl_display::s_touch_hook != nullptr) {
        lvgl_display::s_touch_hook(indev, data);
    }
}

void KeyDown(uint32_t key)
{
    s_key_reads.push_back({key, true});
    lv_indev_read(s_keys);
}

void KeyUp(uint32_t key)
{
    s_key_reads.push_back({key, false});
    lv_indev_read(s_keys);
}

void Press(uint32_t key)
{
    KeyDown(key);
    KeyUp(key);
}

// A tap at the middle of `obj`: down and up in place.
void Tap(lv_obj_t *obj)
{
    lv_obj_update_layout(lv_obj_get_screen(obj));
    lv_area_t a;
    lv_obj_get_coords(obj, &a);
    s_point = {(a.x1 + a.x2) / 2, (a.y1 + a.y2) / 2};
    s_finger = true;
    lv_indev_read(s_touch);
    s_finger = false;
    lv_indev_read(s_touch);
}

// --- A screen of three buttons that count their presses; the third pushes another screen ---------------------------------------

struct Page {
    lv_obj_t *root = nullptr;
    lv_group_t *group = nullptr;
    lv_obj_t *buttons[3] = {};
    int clicks[3] = {};
};
Page s_home;
Page s_page;  // the page on top

void OnClicked(lv_event_t *e)
{
    ++*static_cast<int *>(lv_event_get_user_data(e));
}

class ThreeButtons;
std::unique_ptr<router::Screen> MakeThreeButtons();

void OnOpenClicked(lv_event_t *)
{
    router::Push(MakeThreeButtons());
}

class ThreeButtons final : public router::Screen {
public:
    void OnCreate(lv_obj_t *root, lv_group_t *group) override
    {
        s_page = Page{};
        s_page.root = root;
        s_page.group = group;
        for (int i = 0; i < 3; ++i) {
            lv_obj_t *button = lv_button_create(root);
            lv_obj_set_size(button, 200, 80);
            lv_obj_set_pos(button, 20, 20 + 120 * i);
            lv_obj_add_event_cb(button, OnClicked, LV_EVENT_CLICKED, &s_page.clicks[i]);
            lv_group_add_obj(group, button);
            s_page.buttons[i] = button;
        }
        lv_obj_add_event_cb(s_page.buttons[2], OnOpenClicked, LV_EVENT_CLICKED, nullptr);
    }
};

std::unique_ptr<router::Screen> MakeThreeButtons()
{
    return std::make_unique<ThreeButtons>();
}

bool Ringed(lv_obj_t *obj)
{
    return lv_obj_has_state(obj, LV_STATE_FOCUS_KEY);
}

lv_obj_t *Focused()
{
    return lv_group_get_focused(s_bound);
}

bool NoRing()
{
    for (lv_obj_t *b : s_page.buttons) {
        if (Ringed(b)) {
            return false;
        }
    }
    return true;
}

// --- The tests --------------------------------------------------------------------------------------------------------------

void TestFirstPressRings()
{
    Check(NoRing() && Focused() == s_page.buttons[0], "home opens with no ring, its group naming its first control");
    Press(LV_KEY_NEXT);
    Check(Focused() == s_page.buttons[0] && Ringed(s_page.buttons[0]), "the first DOWN shows the ring where focus is, and moves nothing");
    Press(LV_KEY_NEXT);
    Check(Focused() == s_page.buttons[1] && Ringed(s_page.buttons[1]) && !Ringed(s_page.buttons[0]), "the second DOWN moves it");

    router::ClearRing();
    Press(LV_KEY_PREV);
    Check(Focused() == s_page.buttons[1] && Ringed(s_page.buttons[1]), "with no ring, the first UP shows it and moves nothing");

    router::ClearRing();
    Press(LV_KEY_ENTER);
    Check(Ringed(s_page.buttons[1]) && s_page.clicks[1] == 0, "with no ring, the first OK shows it and presses nothing");
    Press(LV_KEY_ENTER);
    Check(s_page.clicks[1] == 1, "the next OK presses");
}

void TestHeldKeyAfterTheRing()
{
    // A held DOWN whose first press only rang: the repeat's release and fresh press (lvgl_display's KeyRepeat) step from there.
    router::ClearRing();
    lv_obj_t *from = Focused();
    KeyDown(LV_KEY_NEXT);
    KeyDown(LV_KEY_NEXT);  // still held, read again: still nothing
    Check(Focused() == from && Ringed(from), "a held DOWN that rang moves nothing while it is held");
    KeyUp(LV_KEY_NEXT);  // the step: a release, then a press
    KeyDown(LV_KEY_NEXT);
    KeyUp(LV_KEY_NEXT);
    Check(Focused() != from, "its first step moves the ring");
}

void TestOffTheGlass()
{
    // The focused control scrolled off the glass: the first press rings the first control in view instead.
    lv_group_focus_obj(s_page.buttons[2]);
    lv_obj_set_pos(s_page.buttons[2], 20, 2000);
    lv_obj_update_layout(s_page.root);
    router::ClearRing();
    Press(LV_KEY_NEXT);
    Check(Focused() == s_page.buttons[0] && Ringed(s_page.buttons[0]), "a focused control off the glass: the first in view takes the ring");
    lv_obj_set_pos(s_page.buttons[2], 20, 20 + 120 * 2);
    lv_obj_update_layout(s_page.root);
}

void TestTap()
{
    Press(LV_KEY_NEXT);
    Check(Ringed(Focused()), "the ring is showing");
    lv_obj_t *was = Focused();
    const int before = s_page.clicks[1];
    Tap(s_page.buttons[1]);
    Check(s_page.clicks[1] == before + 1, "a tap presses at once");
    Check(Focused() == s_page.buttons[1] && NoRing(), "it takes the place, shows no ring, and drops the one showing");
    (void)was;
    Press(LV_KEY_NEXT);
    Check(Focused() == s_page.buttons[1] && Ringed(s_page.buttons[1]), "the next DOWN shows the ring where the tap left the place");

    // A tap on the background drops the ring and keeps the place.
    s_point = {400, 700};
    s_finger = true;
    lv_indev_read(s_touch);
    s_finger = false;
    lv_indev_read(s_touch);
    Check(Focused() == s_page.buttons[1] && NoRing(), "a tap on nothing drops the ring, and the place stays");
}

void TestEditing()
{
    lv_group_set_editing(s_bound, true);
    router::ClearRing();
    lv_indev_data_t data{};
    data.key = LV_KEY_UP;
    data.state = LV_INDEV_STATE_PRESSED;
    lvgl_display::s_key_hook(s_keys, &data);
    Check(data.state == LV_INDEV_STATE_PRESSED, "a control open for editing takes UP at once");
    data.state = LV_INDEV_STATE_RELEASED;
    lvgl_display::s_key_hook(s_keys, &data);
    lv_group_set_editing(s_bound, false);
}

void TestAcrossPop()
{
    // Opened by OK with the ring: the new screen opens with none, and its first key rings it; Pop brings the ring back.
    lv_group_focus_obj(s_page.buttons[2]);
    lv_obj_add_state(s_page.buttons[2], LV_STATE_FOCUS_KEY);
    Press(LV_KEY_ENTER);
    Check(router::Depth() == 2, "OK on the ringed third button pushed a screen");
    Check(NoRing(), "the pushed screen opens with no ring");
    Press(LV_KEY_NEXT);
    Check(Focused() == s_page.buttons[0] && Ringed(s_page.buttons[0]), "its first key shows the ring and moves nothing");
    router::Pop();
    s_page = s_home;
    Wait(10);
    Check(Focused() == s_home.buttons[2] && Ringed(s_home.buttons[2]), "Pop: the opener, ringed as it was");

    // Opened by a tap: back with no ring, the place on the opener; the first key rings it without moving.
    Tap(s_home.buttons[2]);
    Check(router::Depth() == 2, "a tap on the third button pushed a screen");
    router::Pop();
    s_page = s_home;
    Wait(10);
    Check(Focused() == s_home.buttons[2] && !Ringed(s_home.buttons[2]), "Pop: the opener, with no ring, since a tap opened it");
    Press(LV_KEY_NEXT);
    Check(Focused() == s_home.buttons[2] && Ringed(s_home.buttons[2]), "and the first key rings it there");
}

lv_obj_t *s_overlay_button = nullptr;

void TestAcrossOverlay()
{
    router::Overlay overlay;
    Check(Ringed(Focused()), "the ring is on the page");
    lv_obj_t *opener = Focused();
    lv_group_t *group = router::OpenOverlay(&overlay);
    s_overlay_button = lv_button_create(s_page.root);
    lv_obj_set_size(s_overlay_button, 200, 80);
    lv_obj_set_pos(s_overlay_button, 240, 20);
    lv_group_add_obj(group, s_overlay_button);
    Check(!Ringed(opener) && !Ringed(s_overlay_button), "an overlay opened from code: nothing ringed under it or in it");
    Press(LV_KEY_ENTER);
    Check(Ringed(s_overlay_button), "the first OK in the overlay rings its control");
    router::CloseOverlay(&overlay);
    lv_obj_delete(s_overlay_button);
    Check(Focused() == opener && Ringed(opener), "closing it: the opener, ringed as it was");

    // Opened over a tapped control: back with no ring, the first key rings it.
    Tap(s_page.buttons[0]);
    lv_obj_t *tapped = Focused();
    router::OpenOverlay(&overlay);
    router::CloseOverlay(&overlay);
    Check(Focused() == tapped && !Ringed(tapped), "an overlay opened over a tap closes back to the place, no ring");
}

void TestHelpers()
{
    // Focus: the ring as the input acting says; with none acting, as the last input given.
    Press(LV_KEY_NEXT);  // the buttons are the last input
    router::Focus(s_page.buttons[1]);
    Check(Focused() == s_page.buttons[1] && Ringed(s_page.buttons[1]), "Focus after a key press rings");
    Tap(s_page.buttons[0]);  // a finger is the last input
    router::Focus(s_page.buttons[1]);
    Check(Focused() == s_page.buttons[1] && !Ringed(s_page.buttons[1]), "Focus after a tap moves the place, no ring");
    router::Ring(s_page.buttons[0]);
    Check(Focused() == s_page.buttons[0] && Ringed(s_page.buttons[0]), "Ring rings whatever the input");
    router::ClearRing();
    Check(Focused() == s_page.buttons[0] && !Ringed(s_page.buttons[0]), "ClearRing takes the ring off and keeps the place");

    // Hiding the focused control hands focus on, ringed only if it was.
    router::Ring(s_page.buttons[0]);
    router::SetHiddenHandingFocus(s_page.buttons[0], true, s_page.buttons[1]);
    Check(Focused() == s_page.buttons[1] && Ringed(s_page.buttons[1]), "a ringed control hidden hands focus and ring to the next");
    router::SetHiddenHandingFocus(s_page.buttons[0], false, nullptr);
    Check(!lv_obj_has_flag(s_page.buttons[0], LV_OBJ_FLAG_HIDDEN) && Focused() == s_page.buttons[1], "showing it moves nothing");
    router::ClearRing();
    router::SetHiddenHandingFocus(s_page.buttons[1], true, s_page.buttons[0]);
    Check(Focused() == s_page.buttons[0] && !Ringed(s_page.buttons[0]), "an unringed one hands on the place with no ring");
    router::SetHiddenHandingFocus(s_page.buttons[1], false, nullptr);

    // Hold, and a deletion that hands focus on.
    lv_obj_t *extra = lv_button_create(s_page.root);
    lv_group_add_obj(s_page.group, extra);
    lv_obj_t *held = nullptr;
    router::Hold(&held, extra);
    Check(held == extra, "Hold keeps the object");
    router::Ring(extra);
    router::DeleteHandingFocus(extra, s_page.buttons[1]);
    Check(held == nullptr, "and lets go of it the moment it is deleted");
    Check(Focused() == s_page.buttons[1] && Ringed(s_page.buttons[1]), "deleting the ringed control hands the ring to the next");
    lv_obj_t *other = lv_button_create(s_page.root);
    router::Hold(&held, other);
    router::Hold(&held, nullptr);
    lv_obj_delete(other);
    Check(held == nullptr, "a slot let go of is not written when its old object goes");
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
    router::Init();
    Check(lvgl_display::s_key_hook != nullptr && lvgl_display::s_touch_hook != nullptr, "Init installs both read hooks");
    router::SetRoot(MakeThreeButtons());
    s_home = s_page;
    TestFirstPressRings();
    TestHeldKeyAfterTheRing();
    TestOffTheGlass();
    TestTap();
    TestEditing();
    TestAcrossPop();
    TestAcrossOverlay();
    TestHelpers();
    std::printf("focus rules on LVGL: %d checks, %d failed\n", s_checks, s_failures);
    return s_failures == 0 ? 0 : 1;
}
