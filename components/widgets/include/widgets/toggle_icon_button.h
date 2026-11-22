// The toggle icon button: sound on and off as a tonal tile that goes black when on, its glyph the current state — the
// speaker with waves when on, with a slash when off — never the action a press takes (DESIGN → Exceptions → The icon button).

#pragma once

#include "lvgl.h"

namespace widgets {

// A tile `spacing.target` square in the focus group, its ring outside it: off the light dither with a black glyph, on black
// with a white glyph. OK or a tap flips it and sends LV_EVENT_VALUE_CHANGED; the caller sets LV_STATE_CHECKED to start. The
// state is also in words beside it, which a toggle icon row does. Disabled (lv_obj_add_state(…, LV_STATE_DISABLED)) it is the
// light dither with the glyph `text-disabled`, on or off, and the two differ by the glyph alone.
lv_obj_t *MakeToggleIconButton(lv_obj_t *parent);

}  // namespace widgets
