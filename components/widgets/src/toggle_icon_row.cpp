#include "widgets/toggle_icon_row.h"

#include "design_tokens.h"
#include "internal/base.h"
#include "internal/setting_row.h"
#include "widgets/toggle_icon_button.h"

namespace widgets {

namespace {
void ShowState(lv_obj_t *button, lv_obj_t *state)
{
    lv_label_set_text(state, lv_obj_has_state(button, LV_STATE_CHECKED) ? "On" : "Off");
}

// A tap on the row, off the tile, flips the tile: the whole row toggles.
void OnRowClicked(lv_event_t *e)
{
    lv_obj_t *button = static_cast<lv_obj_t *>(lv_event_get_user_data(e));
    if (lv_obj_has_state(button, LV_STATE_DISABLED)) {
        return;
    }
    lv_obj_set_state(button, LV_STATE_CHECKED, !lv_obj_has_state(button, LV_STATE_CHECKED));
    lv_obj_send_event(button, LV_EVENT_VALUE_CHANGED, nullptr);
}

void OnToggled(lv_event_t *e)
{
    ShowState(lv_event_get_target_obj(e), static_cast<lv_obj_t *>(lv_event_get_user_data(e)));
}
}  // namespace

ToggleIconRow MakeToggleIconRow(lv_obj_t *parent, const char *name, bool on)
{
    const detail::SettingRow parts = detail::MakeSettingRow(parent, name, true);
    ToggleIconRow row{};
    row.root = parts.root;
    row.label = parts.label;
    row.reason = parts.reason;
    row.state = parts.state;
    row.button = MakeToggleIconButton(row.root);
    lv_obj_set_state(row.button, LV_STATE_CHECKED, on);
    ShowState(row.button, row.state);
    lv_obj_add_event_cb(row.root, OnRowClicked, LV_EVENT_CLICKED, row.button);
    lv_obj_add_event_cb(row.button, OnToggled, LV_EVENT_VALUE_CHANGED, row.state);
    return row;
}

void SetToggleIconRowEnabled(const ToggleIconRow &row, bool enabled, const char *reason)
{
    lv_obj_set_state(row.button, LV_STATE_DISABLED, !enabled);
    detail::ShowSettingRowEnabled({row.root, lv_obj_get_parent(row.label), row.label, row.reason, row.state}, enabled, reason);
}

}  // namespace widgets
