// A checkbox row: a setting's name, then the checkbox at the content's right edge — the toggle row's anatomy with no state
// word, since the check is the state; OK on the box or a tap anywhere on the row flips it (DESIGN → Components → The kit).

#pragma once

#include "lvgl.h"

namespace widgets {

struct CheckboxRow {
    lv_obj_t *root;
    lv_obj_t *label;
    lv_obj_t *reason;  // a `caption` under the name while the row is disabled, hidden otherwise
    lv_obj_t *box;     // the checkbox, the row's focus stop; a tap on the row forwards its flip here
};

// A row at least `spacing.target` high, full width; the box is the focus stop, its ring round the box alone, and sends
// LV_EVENT_VALUE_CHANGED. `checked` sets the start. A name too long for its room — 20 characters of `label` — wraps, never
// truncates.
CheckboxRow MakeCheckboxRow(lv_obj_t *parent, const char *name, bool checked);
// Shows the row disabled — the box the light dither with no edge, the name `text-disabled-page`, the buttons skip it, the
// reason in words under the name — or enabled again, the reason hidden.
void SetCheckboxRowEnabled(const CheckboxRow &row, bool enabled, const char *reason = nullptr);

}  // namespace widgets
