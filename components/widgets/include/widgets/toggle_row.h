// A toggle row: a setting's name, its state in words, then the toggle at the content's right edge; OK on the toggle or a
// tap anywhere on the row flips it (DESIGN → Components → The kit).

#pragma once

#include "lvgl.h"

namespace widgets {

struct ToggleRow {
    lv_obj_t *root;
    lv_obj_t *label;
    lv_obj_t *state;   // "On" or "Off"
    lv_obj_t *reason;  // a `caption` under the name while the row is disabled, hidden otherwise
    lv_obj_t *toggle;  // the switch, the row's focus stop; a tap on the row forwards its flip here
};

// A row at least `spacing.target` high, full width; the toggle is the focus stop, its ring round the toggle alone, and
// sends LV_EVENT_VALUE_CHANGED. `on` sets the start. A name too long for its room wraps, never truncates.
ToggleRow MakeToggleRow(lv_obj_t *parent, const char *name, bool on);
// Shows the row disabled — the toggle's track the light dither with a white thumb and no edge, the name and the state word
// `text-disabled-page`, the buttons skip it, the reason in words under the name — or enabled again, the reason hidden.
void SetToggleRowEnabled(const ToggleRow &row, bool enabled, const char *reason = nullptr);

}  // namespace widgets
