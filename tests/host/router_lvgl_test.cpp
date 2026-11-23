// Host test of the router's screen stack and routes (components/router) on LVGL's real objects, groups, keypad and timers, on
// the computer with no board: the lifecycle runs in order — the new screen built, the old one exited, the new one entered, and
// back — and a popped screen is deleted only after the call that popped it, from LVGL's timer; each screen's group is bound to
// the buttons while it is on top; a screen opened by a key press opens with no ring; Pop gives focus back to the control that
// opened the screen, ringed only if it was ringed then; GoTo replaces everything above home; a route refuses an unknown id and a
// param its screen does not take; the stack stops at kMaxDepth; holding OK pops; every root is press-locked; and with no memory
// to defer a popped screen's deletion, its Screen and group go at once and its root after the call, nothing leaked
// (APP_ARCHITECTURE §7).
// run.sh builds LVGL's sources from managed_components once, into build/host/lvgl, and the sources named below, against stand-ins
// for the ESP-IDF headers (tests/host/stubs).
// host-sources: components/router/router.cpp components/router/overlays.cpp components/router/focus.cpp components/router/touch.cpp components/router/routes.cpp components/router/screen.cpp components/router/status.cpp
// Build and run: bash tests/host/run.sh

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <memory>
#include <new>
#include <string>
#include <vector>

#include "lvgl.h"
#include "lvgl_display.h"
#include "router/router.h"

// --- An allocator that fails on request: the router allocates with `new (std::nothrow)`, the test's own objects never do ------

namespace {
bool s_fail_nothrow_new = false;
}  // namespace

void *operator new(std::size_t size, const std::nothrow_t &) noexcept
{
    return s_fail_nothrow_new ? nullptr : std::malloc(size != 0 ? size : 1);
}

// --- The stand-ins for lvgl_display's side: the group bound to the keypad, and the back handler ------------------------------

namespace {
lv_indev_t *s_keys = nullptr;
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
// The router's read hooks, kept so a test can run a key read through the focus rules as LVGL's read would.
void (*s_key_hook)(lv_indev_t *, lv_indev_data_t *) = nullptr;
void (*s_touch_hook)(lv_indev_t *, lv_indev_data_t *) = nullptr;
void SetKeyReadHook(void (*hook)(lv_indev_t *, lv_indev_data_t *)) { s_key_hook = hook; }
void SetTouchReadHook(void (*hook)(lv_indev_t *, lv_indev_data_t *)) { s_touch_hook = hook; }
void RequestFull() {}
void PlayCue(Cue) {}  // the status line asks a sound as its news is drawn; these tests do not listen
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

// --- The keypad: the presses a test queues, read one a call -----------------------------------------------------------------

struct KeyRead {
    uint32_t key;
    bool pressed;
};
std::deque<KeyRead> s_reads;
uint32_t s_last_key = LV_KEY_ENTER;

void ReadKeys(lv_indev_t *, lv_indev_data_t *data)
{
    bool pressed = false;
    if (!s_reads.empty()) {
        s_last_key = s_reads.front().key;
        pressed = s_reads.front().pressed;
        s_reads.pop_front();
    }
    data->key = s_last_key;
    data->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

// A press and release of `key`, through LVGL's keypad processing.
void Press(uint32_t key)
{
    s_reads.push_back({key, true});
    lv_indev_read(s_keys);
    s_reads.push_back({key, false});
    lv_indev_read(s_keys);
}

// --- Screens that say what happens to them --------------------------------------------------------------------------------

std::vector<std::string> s_log;
int s_root_deletes = 0;

bool Logged(const std::vector<std::string> &want)
{
    return s_log == want;
}

void OnRootDelete(lv_event_t *)
{
    ++s_root_deletes;
}

void OnOpenClicked(lv_event_t *e);

class Probe final : public router::Screen {
public:
    explicit Probe(std::string name, int buttons = 2) : name_(std::move(name)), buttons_(buttons) {}
    ~Probe() override { s_log.push_back("~" + name_); }

    void OnCreate(lv_obj_t *root, lv_group_t *group) override
    {
        root_ = root;
        group_ = group;
        lv_obj_add_event_cb(root, OnRootDelete, LV_EVENT_DELETE, nullptr);
        for (int i = 0; i < buttons_; ++i) {
            lv_obj_t *button = lv_button_create(root);
            lv_obj_add_event_cb(button, OnOpenClicked, LV_EVENT_CLICKED, nullptr);
            lv_group_add_obj(lvgl_display::Group(), button);  // as ui and the widgets add their controls
            buttons[i] = button;
        }
        s_log.push_back("create " + name_);
    }
    void OnEnter() override { s_log.push_back("enter " + name_); }
    void OnExit() override { s_log.push_back("exit " + name_); }

    lv_obj_t *root_ = nullptr;
    lv_group_t *group_ = nullptr;
    lv_obj_t *buttons[4] = {};

private:
    std::string name_;
    int buttons_;
};

// The probes on the stack now, by name, so a test can reach a screen's root and buttons.
Probe *s_last_made = nullptr;
std::string s_next_name = "B";

std::unique_ptr<router::Screen> MakeProbe(const std::string &name)
{
    auto probe = std::make_unique<Probe>(name);
    s_last_made = probe.get();
    return probe;
}

// A button's click pushes the next probe, as a home row pushes its route.
void OnOpenClicked(lv_event_t *)
{
    router::Push(MakeProbe(s_next_name));
}

bool Ringed(lv_obj_t *obj)
{
    return lv_obj_has_state(obj, LV_STATE_FOCUSED) && lv_obj_has_state(obj, LV_STATE_FOCUS_KEY);
}

Probe *s_home = nullptr;

// --- The tests --------------------------------------------------------------------------------------------------------------

void TestInitAndRoot()
{
    Check(router::Depth() == 0, "the stack is empty before a root");
    Check(router::Push(MakeProbe("X")) == ESP_ERR_INVALID_STATE, "a push before Init and a root is refused");
    Check(router::SetRoot(MakeProbe("X")) == ESP_ERR_INVALID_STATE, "a root before Init is refused");
    s_log.clear();
    Check(router::Init() == ESP_OK && s_back != nullptr, "Init takes holding OK");
    Check(router::Init() == ESP_ERR_INVALID_STATE, "a second Init is refused");
    Check(router::SetRoot(MakeProbe("home")) == ESP_OK, "home is the root");
    s_home = s_last_made;
    Check(router::SetRoot(MakeProbe("again")) == ESP_ERR_INVALID_STATE, "a second root is refused");
    Check(router::Depth() == 1, "the stack holds home");
    Check(lv_screen_active() == s_home->root_, "home is on the glass");
    Check(s_bound == s_home->group_, "home's group is bound to the buttons");
    Check(lv_obj_is_press_lock(s_home->root_), "home's root is press-locked");
    Check(lv_group_get_wrap(s_home->group_), "its group wraps");
    Wait(10);
    s_log.clear();
}

void TestLifecycleAndAsyncDelete()
{
    Check(router::Push(MakeProbe("A")) == ESP_OK, "a push");
    Probe *a = s_last_made;
    Check(Logged({"create A", "exit home", "enter A"}), "push: the new screen built, the old one exited, the new one entered");
    Check(router::Depth() == 2 && lv_screen_active() == a->root_, "the pushed screen is on top and on the glass");
    Check(s_bound == a->group_ && lv_group_get_default() == a->group_, "its group is bound and LVGL's default");
    Check(lv_obj_is_press_lock(a->root_), "its root is press-locked");
    Check(lv_obj_get_group(a->buttons[0]) == a->group_, "a control made in OnCreate joins the screen's own group");
    s_log.clear();
    const int deletes = s_root_deletes;
    router::Pop();
    Check(Logged({"exit A", "enter home"}), "pop: the top exited, the one under it entered");
    Check(router::Depth() == 1 && lv_screen_active() == s_home->root_, "home is on the glass again");
    Check(s_bound == s_home->group_, "home's group is bound again");
    Check(s_root_deletes == deletes, "the popped screen is not deleted inside the call that popped it");
    Wait(10);
    Check(s_root_deletes == deletes + 1, "it is deleted from LVGL's timer after");
    Check(Logged({"exit A", "enter home", "~A"}), "and the Screen with it");
    s_log.clear();
    router::Pop();
    Check(s_log.empty() && router::Depth() == 1, "a pop at home does nothing");
}

void TestUnringedOpenAndRingBack()
{
    // The ring on home's second button, by a key press; OK opens a screen from it.
    lv_group_focus_obj(s_home->buttons[0]);
    Press(LV_KEY_NEXT);
    Check(lv_group_get_focused(s_home->group_) == s_home->buttons[1] && Ringed(s_home->buttons[1]),
          "DOWN rings home's second button");
    s_next_name = "B";
    Press(LV_KEY_ENTER);
    Probe *b = s_last_made;
    Check(router::Depth() == 2 && lv_screen_active() == b->root_, "OK on it pushed a screen");
    lv_obj_t *named = lv_group_get_focused(b->group_);
    Check(named == b->buttons[0], "the pushed screen's group names its first control");
    Check(named != nullptr && lv_obj_has_state(named, LV_STATE_FOCUSED) && !lv_obj_has_state(named, LV_STATE_FOCUS_KEY),
          "a screen opened by a key press opens with no ring");
    router::Pop();
    Check(lv_group_get_focused(s_home->group_) == s_home->buttons[1], "pop: focus is back on the control that opened it");
    Check(Ringed(s_home->buttons[1]), "ringed, as it was when it opened the screen");
    Wait(10);

    // The opener focused with no ring, as a tap leaves it: it comes back with none.
    lv_obj_remove_state(s_home->buttons[1], LV_STATE_FOCUS_KEY);
    router::Push(MakeProbe("C"));
    router::Pop();
    Check(lv_group_get_focused(s_home->group_) == s_home->buttons[1], "focus is back on the opener");
    Check(!lv_obj_has_state(s_home->buttons[1], LV_STATE_FOCUS_KEY), "with no ring, since it had none");
    Wait(10);
    s_log.clear();
}

void TestHoldOkPops()
{
    router::Push(MakeProbe("D"));
    s_log.clear();
    s_back();
    Check(router::Depth() == 1 && Logged({"exit D", "enter home"}), "holding OK pops the top screen");
    Check(!router::Escape(), "at home Escape does nothing");
    Check(router::Depth() == 1, "and home stays");
    Wait(10);
    s_log.clear();
}

void TestGoToAndPopToRoot()
{
    router::Push(MakeProbe("E"));
    router::Push(MakeProbe("F"));
    Check(router::Depth() == 3, "two screens over home");
    s_log.clear();
    Check(router::GoTo(MakeProbe("G")) == ESP_OK, "GoTo");
    Probe *g = s_last_made;
    Check(router::Depth() == 2 && lv_screen_active() == g->root_ && s_bound == g->group_,
          "GoTo replaces everything above home: home and the new screen");
    Check(Logged({"create G", "exit F", "enter G"}), "only the screen on top is exited: the one under it was when covered");
    Wait(10);
    Check(Logged({"create G", "exit F", "enter G", "~F", "~E"}) || Logged({"create G", "exit F", "enter G", "~E", "~F"}),
          "both screens it replaced are deleted after");
    s_log.clear();
    router::Push(MakeProbe("H"));
    s_log.clear();
    router::PopToRoot();
    Check(router::Depth() == 1 && lv_screen_active() == s_home->root_ && s_bound == s_home->group_, "PopToRoot: home");
    Check(Logged({"exit H", "enter home"}), "only the top screen is exited");
    Wait(10);
    s_log.clear();
    Check(router::GoTo(MakeProbe("I")) == ESP_OK && router::Depth() == 2, "GoTo from home is a push");
    router::PopToRoot();
    Wait(10);
    s_log.clear();
}

void TestDepth()
{
    for (size_t depth = router::Depth(); depth < router::kMaxDepth; ++depth) {
        Check(router::Push(MakeProbe("deep")) == ESP_OK, "a push up to kMaxDepth");
    }
    Check(router::Depth() == router::kMaxDepth, "the stack at its depth");
    s_log.clear();
    Check(router::Push(MakeProbe("past")) == ESP_ERR_INVALID_STATE, "a push past kMaxDepth is refused");
    Check(Logged({"~past"}), "the refused screen is never built, only let go");
    Check(router::Push(nullptr) == ESP_ERR_INVALID_ARG, "a push of nothing is refused");
    router::PopToRoot();
    Wait(10);
    s_log.clear();
}

std::unique_ptr<router::Screen> MakeRouteA()
{
    return MakeProbe("route a");
}

std::unique_ptr<router::Screen> MakeTakesX(const char *param)
{
    if (param == nullptr || std::strcmp(param, "x") != 0) {
        return nullptr;
    }
    return MakeProbe("route x");
}

void TestRoutes()
{
    static const router::Route kRoutes[] = {
        {"t.a", router::NoParam<MakeRouteA>},
        {"t.x", MakeTakesX},
    };
    Check(router::AddRoutes({kRoutes, 2}) == ESP_OK, "a table of routes is added");
    static const router::Route kTaken[] = {{"t.b", router::NoParam<MakeRouteA>}, {"t.a", router::NoParam<MakeRouteA>}};
    Check(router::AddRoutes({kTaken, 2}) == ESP_ERR_INVALID_ARG, "a table with an id already taken is refused");
    Check(router::FindRoute("t.b") == nullptr, "whole: none of its routes is added");
    static const router::Route kTwice[] = {{"t.c", router::NoParam<MakeRouteA>}, {"t.c", router::NoParam<MakeRouteA>}};
    Check(router::AddRoutes({kTwice, 2}) == ESP_ERR_INVALID_ARG, "a table naming one id twice is refused");
    static const router::Route kEmpty[] = {{"t.d", nullptr}};
    Check(router::AddRoutes({kEmpty, 1}) == ESP_ERR_INVALID_ARG, "a route with no factory is refused");
    Check(router::FindRoute("t.x") == &kRoutes[1], "a route is found by its id");

    s_log.clear();
    Check(router::PushRoute("t.nope") == ESP_ERR_NOT_FOUND, "an unknown id is refused");
    Check(router::PushRoute("t.x", "y") == ESP_ERR_INVALID_ARG, "a param the screen does not take is refused");
    Check(router::PushRoute("t.a", "z") == ESP_ERR_INVALID_ARG, "a screen that takes no param refuses one");
    Check(router::Depth() == 1 && lv_screen_active() == s_home->root_ && s_log.empty(), "and nothing on the glass changes");
    Check(router::PushRoute("t.a") == ESP_OK && router::Depth() == 2, "a route pushes its screen");
    Check(router::PushRoute("t.x", "x") == ESP_OK && router::Depth() == 3, "with the param it takes");
    Check(router::GoToRoute("t.a") == ESP_OK && router::Depth() == 2, "GoToRoute replaces what is above home");
    router::PopToRoot();
    Wait(10);
}

// The deferred deletion's own record cannot be allocated: the Screen and the group are deleted at once, the root after the
// call that popped it, and nothing is left behind.
void TestRetireWithNoMemory()
{
    router::Push(MakeProbe("J"));
    const uint32_t groups = lv_group_get_count();
    const int deletes = s_root_deletes;
    s_log.clear();
    s_fail_nothrow_new = true;
    router::Pop();
    s_fail_nothrow_new = false;
    Check(router::Depth() == 1 && lv_screen_active() == s_home->root_ && s_bound == s_home->group_,
          "a pop with no memory to defer the deletion still goes back");
    Check(Logged({"exit J", "enter home", "~J"}), "the popped Screen is deleted at once");
    Check(lv_group_get_count() == groups - 1, "and its group");
    Check(s_root_deletes == deletes, "its root is not deleted inside the call that popped it");
    Wait(10);
    Check(s_root_deletes == deletes + 1, "but from LVGL's timer after it: nothing is leaked");
    s_log.clear();
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
    // The buttons as the firmware's are: a keypad, with LVGL's own reads off — the test reads it when it presses — and a
    // pointer, so a focus the code makes is a finger's and draws no ring, as on the glass.
    s_keys = lv_indev_create();
    lv_indev_set_type(s_keys, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(s_keys, ReadKeys);
    lv_indev_set_mode(s_keys, LV_INDEV_MODE_EVENT);
    lv_indev_t *touch = lv_indev_create();
    lv_indev_set_type(touch, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch, [](lv_indev_t *, lv_indev_data_t *data) { data->state = LV_INDEV_STATE_RELEASED; });
    TestInitAndRoot();
    TestLifecycleAndAsyncDelete();
    TestUnringedOpenAndRingBack();
    TestHoldOkPops();
    TestGoToAndPopToRoot();
    TestDepth();
    TestRoutes();
    TestRetireWithNoMemory();
    std::printf("router on LVGL: %d checks, %d failed\n", s_checks, s_failures);
    return s_failures == 0 ? 0 : 1;
}
