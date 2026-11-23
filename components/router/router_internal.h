// What the router's own files ask of each other; nothing outside components/router includes this.

#pragma once

#include "lvgl.h"
#include "router/screen.h"

namespace router::detail {

// router.cpp: the screen on top of the stack, nullptr before the root.
Screen *TopScreen();

// overlays.cpp: holding OK's first link — the top overlay closes as its Cancel does; false with none open.
bool EscapeTopOverlay();

// focus.cpp: the buttons' read hook, the no-ring press.
void KeyReadHook(lv_indev_t *indev, lv_indev_data_t *data);

// touch.cpp: the touch panel's read hook, and which input was given last — a button press (true) or a touch.
void TouchReadHook(lv_indev_t *indev, lv_indev_data_t *data);
void NoteKeyInput(bool keys);
bool KeysWereLast();

}  // namespace router::detail
