// The radio control: the toggle's look as a disc — `spacing.check-size` across, `rounded.full`; off is the light dither
// inside a `spacing.stroke.default` outline, on is black with a white dot of `spacing.check-dot` (DESIGN → Components →
// The kit).

#pragma once

#include "lvgl.h"

namespace widgets {

// A control `spacing.check-size` round in the focus group, its ring outside it; OK or a tap chooses it, and it takes touches
// across `spacing.target`. A radio never unchooses itself: a press on a chosen one does nothing. Choosing sends
// LV_EVENT_VALUE_CHANGED; the caller sets LV_STATE_CHECKED to start, and clears the others of its group (a radio button row
// does, see radio_button_row.h). Disable it with lv_obj_add_state(…, LV_STATE_DISABLED): the light dither with no edge, a
// chosen dot `text-disabled`. Its parents let the ring out (`LetRingOut`).
lv_obj_t *MakeRadio(lv_obj_t *parent);

}  // namespace widgets
