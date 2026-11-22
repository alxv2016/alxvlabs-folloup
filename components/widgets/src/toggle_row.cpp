#include "widgets/toggle_row.h"

#include "design_tokens.h"
#include "internal/base.h"
#include "internal/setting_row.h"
#include "widgets/toggle.h"

namespace widgets {
namespace dt = design_tokens;

namespace {
void ShowState(lv_obj_t *toggle, lv_obj_t *state)
{
    lv_label_set_text(state, lv_obj_has_state(toggle, LV_STATE_CHECKED) ? "On" : "Off");
}

// A tap on the row, off the toggle, flips the toggle: the whole row toggles.
void OnRowClicked(lv_event_t *e)
{
    lv_obj_t *toggle = static_cast<lv_obj_t *>(lv_event_get_user_data(e));
    if (lv_obj_has_state(toggle, LV_STATE_DISABLED)) {
        return;
    }
    lv_obj_set_state(toggle, LV_STATE_CHECKED, !lv_obj_has_state(toggle, LV_STATE_CHECKED));
    lv_obj_send_event(toggle, LV_EVENT_VALUE_CHANGED, nullptr);
}

void OnToggled(lv_event_t *e)
{
    ShowState(lv_event_get_target_obj(e), static_cast<lv_obj_t *>(lv_event_get_user_data(e)));
}
}  // namespace

ToggleRow MakeToggleRow(lv_obj_t *parent, const char *name, bool on)
{
    const detail::SettingRow parts = detail::MakeSettingRow(parent, name, true);
    ToggleRow row{};
    row.root = parts.root;
    row.label = parts.label;
    row.reason = parts.reason;
    row.state = parts.state;
    row.toggle = MakeToggle(row.root);
    lv_obj_set_state(row.toggle, LV_STATE_CHECKED, on);
    ShowState(row.toggle, row.state);
    lv_obj_add_event_cb(row.root, OnRowClicked, LV_EVENT_CLICKED, row.toggle);
    lv_obj_add_event_cb(row.toggle, OnToggled, LV_EVENT_VALUE_CHANGED, row.state);
    return row;
}

void SetToggleRowEnabled(const ToggleRow &row, bool enabled, const char *reason)
{
    lv_obj_set_state(row.toggle, LV_STATE_DISABLED, !enabled);
    detail::ShowSettingRowEnabled({row.root, lv_obj_get_parent(row.label), row.label, row.reason, row.state}, enabled, reason);
}

}  // namespace widgets
