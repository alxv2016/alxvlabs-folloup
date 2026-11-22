// A chip: a rounded box as wide as its words, on the light dither; chosen is the deck's black edge, the fill unchanged
// (DESIGN → Components → The kit).

#pragma once

#include "lvgl.h"

namespace widgets {

// A box `spacing.target` high, `rounded.lg` — the deck's chip is its base radius, not a pill — in the focus group with its
// ring outside it. A `spacing.stroke.strong` edge is reserved at rest, transparent, so choosing a chip moves none of its
// neighbours. The caller owns LV_STATE_CHECKED and listens for LV_EVENT_CLICKED.
lv_obj_t *MakeChip(lv_obj_t *parent, const char *text);

}  // namespace widgets
