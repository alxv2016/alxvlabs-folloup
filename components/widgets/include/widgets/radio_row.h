// A radio row — the deck's radio list item: its words, then a `spacing.mark` ring on the light dither; chosen shows a
// `spacing.mark-dot` dot in the ring and nothing else (DESIGN → Components → The kit).

#pragma once

#include "lvgl.h"

namespace widgets {

// A row at least `spacing.list-row-height` high, full width, `rounded.lg`, its words `spacing.card-inset` in, in the focus group
// with its ring outside it. The words wrap, 18 characters a line, and are never cut: `spacing.xs` above and below, so one line
// and two both stand 80 px high, three 116 and four 152. A radio row has no detail line; an answer that needs one is a choice
// card. A group is the radio rows that share a parent: OK on a row, or a tap on the lift, chooses it, clears the others, then
// sends LV_EVENT_VALUE_CHANGED on it; choosing the chosen row sends nothing, and moving the focus never chooses.
// SetRadioChosen sets a row by the program (clearing the others when it chooses one) and sends nothing.
// Disabled (LV_STATE_DISABLED) it is the light dither with no edge, its words `text-disabled` and the mark's edge gone.
lv_obj_t *MakeRadioRow(lv_obj_t *parent, const char *text);
void SetRadioChosen(lv_obj_t *row, bool chosen);
bool RadioChosen(lv_obj_t *row);

}  // namespace widgets
