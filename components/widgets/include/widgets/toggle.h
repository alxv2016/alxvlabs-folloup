// The toggle: a pill track with a pill thumb, ported from the agent deck's switch (STICKY_DESIGN_TOKENS §9) — off is the light dither with a black
// thumb left, on a black track with a white thumb right (DESIGN → Components → The kit).

#pragma once

#include "lvgl.h"

namespace widgets {

// An LVGL switch `spacing.toggle-width` by `spacing.toggle-height`, in the focus group, its ring outside it; OK or a tap
// flips it, and it takes touches across `spacing.target`. The caller sets LV_STATE_CHECKED and listens for
// LV_EVENT_VALUE_CHANGED. Its state belongs in words beside it. Its parents let the ring out (`LetRingOut`).
lv_obj_t *MakeToggle(lv_obj_t *parent);

}  // namespace widgets
