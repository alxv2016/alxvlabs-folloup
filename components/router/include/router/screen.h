// One full-glass screen, and the lifecycle the router drives it through (APP_ARCHITECTURE §7).
//
// ONEXIT IS WHY THE CLASS EXISTS. A screen starts things — an lv_timer that reads a sensor, a key observer, a benchmark's
// preset — and none of them stops on its own when the glass shows something else. The router calls OnExit whenever the
// screen stops being the top one, so what OnEnter started, OnExit stops, and two screens never hold the same thing.
//
// LIFETIME. The router owns the Screen, the root it builds into and its focus group, and deletes all three after the
// screen is popped, from an lv_async_call, never inside the event that popped it. A screen keeps the pointers it is
// handed and never deletes them.
//
// FOCUS. Every screen has its own lv_group_t, bound to the buttons while the screen is on top, and LVGL's default group while
// OnCreate runs, so lvgl_display::Group() is this group and a control made then joins it. Add the controls in the order UP
// and DOWN walk them. Focus stays where it was while another screen covers this one, so it is on the control that opened
// that screen when it comes back.

#pragma once

#include <memory>

#include "lvgl.h"

namespace router {

class Screen {
public:
    virtual ~Screen() = default;
    Screen(const Screen &) = delete;
    Screen &operator=(const Screen &) = delete;

    // Builds the screen under `root`, an empty LVGL screen object the router made — the screen styles it — adding what the
    // buttons reach to `group`. Called once, before the first OnEnter, under the LVGL lock. It must not start anything:
    // a screen may be built and then dropped.
    virtual void OnCreate(lv_obj_t *root, lv_group_t *group) = 0;
    // The screen is on top: start its timers, its observers.
    virtual void OnEnter() {}
    // It is not on top any more — another screen was pushed over it, or it was popped: stop what OnEnter started. Always
    // after an OnEnter; a screen may see the pair many times.
    virtual void OnExit() {}
    // An overlay covers the screen, or no longer does; the screen is still the top one.
    virtual void OnCovered(bool covered) { (void)covered; }

protected:
    Screen() = default;
};

// A screen from free functions, for a feature written as functions and statics: `create` is OnCreate, `enter`, `exit` and
// `covered` the rest (each may be nullptr), and `user` is handed to each. nullptr when there is no memory, which a Push
// refuses.
using CreateFn = void (*)(lv_obj_t *root, lv_group_t *group, void *user);
using LifecycleFn = void (*)(void *user);
using CoveredFn = void (*)(bool covered, void *user);
std::unique_ptr<Screen> MakeScreen(CreateFn create, LifecycleFn enter = nullptr, LifecycleFn exit = nullptr,
                                   CoveredFn covered = nullptr, void *user = nullptr);

}  // namespace router
