#include "widgets/checkbox_row.h"

#include "design_tokens.h"
#include "internal/base.h"
#include "internal/setting_row.h"
#include "widgets/checkbox.h"

namespace widgets {

namespace {
// A tap on the row, off the box, flips the box: the whole row acts.
void OnRowClicked(lv_event_t *e)
{
    lv_obj_t *box = static_cast<lv_obj_t *>(lv_event_get_user_data(e));
    if (lv_obj_has_state(box, LV_STATE_DISABLED)) {
        return;
    }
    lv_obj_set_state(box, LV_STATE_CHECKED, !lv_obj_has_state(box, LV_STATE_CHECKED));
    lv_obj_send_event(box, LV_EVENT_VALUE_CHANGED, nullptr);
}
}  // namespace

CheckboxRow MakeCheckboxRow(lv_obj_t *parent, const char *name, bool checked)
{
    const detail::SettingRow parts = detail::MakeSettingRow(parent, name, false);
    CheckboxRow row{};
    row.root = parts.root;
    row.label = parts.label;
    row.reason = parts.reason;
    row.box = MakeCheckbox(row.root);
    lv_obj_set_state(row.box, LV_STATE_CHECKED, checked);
    lv_obj_add_event_cb(row.root, OnRowClicked, LV_EVENT_CLICKED, row.box);
    return row;
}

void SetCheckboxRowEnabled(const CheckboxRow &row, bool enabled, const char *reason)
{
    lv_obj_set_state(row.box, LV_STATE_DISABLED, !enabled);
    detail::ShowSettingRowEnabled({row.root, lv_obj_get_parent(row.label), row.label, row.reason, nullptr}, enabled, reason);
}

}  // namespace widgets
