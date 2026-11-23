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

// status.cpp: a screen's status band — the label its words are set into, and the rule that shows with them. nullptrs: none.
struct StatusBand {
    lv_obj_t *words = nullptr;
    lv_obj_t *rule = nullptr;
};
// Before a screen's OnCreate, and after: what it attached.
void BeginStatusBand();
StatusBand TakeStatusBand();
// The screen on top is about to change: the old news goes, drawn nowhere and heard never, and the band of the screen on top is
// left white for the day its screen is shown again. Call it before DismissOverlays, so news held under an overlay is dropped
// rather than drawn as the overlay closes. Nothing can be said until StatusEntered — not from the new screen's OnCreate, nor
// from the old one's OnExit.
void StatusLeaving();
// The new screen is on top, with this band (or none).
void StatusEntered(const StatusBand &entering);
// overlays.cpp: the last overlay has closed — draw and sound the news held under it.
void StatusOverlaysClosed();

}  // namespace router::detail
