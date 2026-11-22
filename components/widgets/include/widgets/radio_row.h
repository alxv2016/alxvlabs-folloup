// A radio row — the deck's radio list item: its words, then a `spacing.mark` ring on the light dither; chosen shows a
// `spacing.mark-dot` dot in the ring and nothing else (DESIGN → Components → The kit).

#pragma once

#include "lvgl.h"

namespace widgets {

// A row `spacing.list-row-height` high, full width, `rounded.lg`, its words `spacing.card-inset` in, in the focus group
// with its ring outside it. The caller listens for LV_EVENT_CLICKED and chooses one row of its set with SetRadioChosen.
// Disabled (LV_STATE_DISABLED) it is the light dither with no edge, its words `text-disabled` and the mark's edge gone.
lv_obj_t *MakeRadioRow(lv_obj_t *parent, const char *text);
void SetRadioChosen(lv_obj_t *row, bool chosen);

}  // namespace widgets
