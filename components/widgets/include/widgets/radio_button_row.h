// A radio button row: one choice of a group, a name and the radio at the content's right edge; OK on the radio or a tap
// anywhere on the row chooses it and clears the group's others (DESIGN → Components → The kit). Rows sharing one parent are
// one group. Not the radio row of radio_row.h, a card of words on the light dither for a screen's one question.

#pragma once

#include "lvgl.h"

namespace widgets {

struct RadioButtonRow {
    lv_obj_t *root;
    lv_obj_t *label;
    lv_obj_t *reason;  // a `caption` under the name while the row is disabled, hidden otherwise
    lv_obj_t *radio;   // the radio, the row's focus stop; sends LV_EVENT_VALUE_CHANGED when it is chosen
};

// A row at least `spacing.target` high, full width, in a group of the rows made in the same `parent` (a group's rows stand
// `spacing.target` apart with no gap, and its name in `caption` above it is the screen's). `chosen` sets the start; the
// caller makes at most one of a group chosen. Choosing by OK or a tap clears the others first and then sends the event.
RadioButtonRow MakeRadioButtonRow(lv_obj_t *parent, const char *name, bool chosen);
// Chooses the row and clears its group's others, as OK does but sending no event.
void ChooseRadioButtonRow(const RadioButtonRow &row);
// Shows the row disabled — the radio the light dither with no edge, the name `text-disabled-page`, the reason in words —
// or enabled again.
void SetRadioButtonRowEnabled(const RadioButtonRow &row, bool enabled, const char *reason = nullptr);

}  // namespace widgets
