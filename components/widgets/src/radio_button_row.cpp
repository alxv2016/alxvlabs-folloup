#include "widgets/radio_button_row.h"

#include "design_tokens.h"
#include "internal/base.h"
#include "internal/setting_row.h"
#include "widgets/radio.h"

namespace widgets {

namespace {
// A group is the rows made in one parent, and a row says so by this tag on its root; its radio is its root's last child.
int s_row_tag;

bool IsRow(const lv_obj_t *obj)
{
    return lv_obj_get_user_data(const_cast<lv_obj_t *>(obj)) == &s_row_tag;
}

lv_obj_t *RadioOf(lv_obj_t *root)
{
    return lv_obj_get_child(root, -1);
}

void ClearSiblings(lv_obj_t *root)
{
    lv_obj_t *group = lv_obj_get_parent(root);
    const uint32_t n = lv_obj_get_child_count(group);
    for (uint32_t i = 0; i < n; ++i) {
        lv_obj_t *other = lv_obj_get_child(group, static_cast<int32_t>(i));
        if (other != root && IsRow(other)) {
            lv_obj_remove_state(RadioOf(other), LV_STATE_CHECKED);
        }
    }
}

// The radio was chosen by OK or a tap: its group's others are cleared before the caller's own listener sees the event, since
// this one is added first.
void OnChosen(lv_event_t *e)
{
    ClearSiblings(static_cast<lv_obj_t *>(lv_event_get_user_data(e)));
}

// A tap on the row, off the radio, chooses it: the whole row acts. A chosen row stays chosen.
void OnRowClicked(lv_event_t *e)
{
    lv_obj_t *radio = static_cast<lv_obj_t *>(lv_event_get_user_data(e));
    if (lv_obj_has_state(radio, LV_STATE_DISABLED) || lv_obj_has_state(radio, LV_STATE_CHECKED)) {
        return;
    }
    lv_obj_add_state(radio, LV_STATE_CHECKED);
    lv_obj_send_event(radio, LV_EVENT_VALUE_CHANGED, nullptr);
}
}  // namespace

RadioButtonRow MakeRadioButtonRow(lv_obj_t *parent, const char *name, bool chosen)
{
    const detail::SettingRow parts = detail::MakeSettingRow(parent, name, false);
    RadioButtonRow row{};
    row.root = parts.root;
    row.label = parts.label;
    row.reason = parts.reason;
    row.radio = MakeRadio(row.root);
    lv_obj_set_state(row.radio, LV_STATE_CHECKED, chosen);
    lv_obj_set_user_data(row.root, &s_row_tag);
    lv_obj_add_event_cb(row.radio, OnChosen, LV_EVENT_VALUE_CHANGED, row.root);
    lv_obj_add_event_cb(row.root, OnRowClicked, LV_EVENT_CLICKED, row.radio);
    return row;
}

void ChooseRadioButtonRow(const RadioButtonRow &row)
{
    lv_obj_add_state(row.radio, LV_STATE_CHECKED);
    ClearSiblings(row.root);
}

void SetRadioButtonRowEnabled(const RadioButtonRow &row, bool enabled, const char *reason)
{
    lv_obj_set_state(row.radio, LV_STATE_DISABLED, !enabled);
    detail::ShowSettingRowEnabled({row.root, lv_obj_get_parent(row.label), row.label, row.reason, nullptr}, enabled, reason);
}

}  // namespace widgets
