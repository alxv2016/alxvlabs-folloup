// What a toggle row, a toggle icon row, a checkbox row and a radio button row share: a setting's name on the left, an
// optional state word, and — added by the caller — one control at the content's right edge; a reason in words under the
// name while the row is disabled (DESIGN → Components → The kit). Private to the widgets component.

#pragma once

#include "lvgl.h"

namespace widgets::detail {

struct SettingRow {
    lv_obj_t *root;    // full width, `spacing.target` high at least, clickable: the whole row acts
    lv_obj_t *words;   // the column that takes the room the control leaves
    lv_obj_t *label;   // the name, wrapping, never truncated
    lv_obj_t *reason;  // a `caption` under the name while disabled, hidden otherwise
    lv_obj_t *state;   // the state in words beside the control, or nullptr
};

// Builds the row up to the control: the caller adds its control as `root`'s next child, and the row's tap handler.
SettingRow MakeSettingRow(lv_obj_t *parent, const char *name, bool with_state);
// The row's words for a disabled or an enabled row: the name and the state word `text-disabled-page` while disabled (on the
// white page), `text-primary` and `text-secondary` otherwise; the reason, solid `text-secondary` in `caption`, `spacing.xs`
// under the name only while it shows, so the row stays 64 px and the name alone stays centred. The control's own disabled
// state is the caller's.
void ShowSettingRowEnabled(const SettingRow &row, bool enabled, const char *reason);

}  // namespace widgets::detail
