// The checkbox control: the toggle's look as a square — `spacing.check-size` across, `rounded.md`; off is the light dither
// inside a `spacing.stroke.default` outline, on is black with a white check (DESIGN → Components → The kit).

#pragma once

#include "lvgl.h"

namespace widgets {

// A control `spacing.check-size` square in the focus group, its ring outside it and following its corners; OK or a tap
// flips it, and it takes touches across `spacing.target`. It sends LV_EVENT_VALUE_CHANGED as the toggle does; the caller
// sets LV_STATE_CHECKED to start. Disable it with lv_obj_add_state(…, LV_STATE_DISABLED): the light dither with no edge, a
// check `text-disabled`. Its parents let the ring out (`LetRingOut`). Its state needs no words: the check is the state.
lv_obj_t *MakeCheckbox(lv_obj_t *parent);

}  // namespace widgets
