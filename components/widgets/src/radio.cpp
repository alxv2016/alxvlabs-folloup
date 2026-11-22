#include "widgets/radio.h"

#include "design_tokens.h"
#include "internal/base.h"

namespace widgets {
namespace dt = design_tokens;

namespace {
// A press chooses the radio, and does nothing on one already chosen: a radio never unchooses itself.
void OnClicked(lv_event_t *e)
{
    lv_obj_t *radio = lv_event_get_target_obj(e);
    if (lv_obj_has_state(radio, LV_STATE_CHECKED) || lv_obj_has_state(radio, LV_STATE_DISABLED)) {
        return;
    }
    lv_obj_add_state(radio, LV_STATE_CHECKED);
    lv_obj_send_event(radio, LV_EVENT_VALUE_CHANGED, nullptr);
}
}  // namespace

lv_obj_t *MakeRadio(lv_obj_t *parent)
{
    constexpr lv_style_selector_t kOnDisabled = static_cast<lv_style_selector_t>(LV_STATE_DISABLED) | LV_STATE_CHECKED;
    lv_obj_t *radio = detail::Bare(parent);
    lv_obj_add_flag(radio, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(radio, dt::kSpacingCheckSize, dt::kSpacingCheckSize);
    lv_obj_set_style_radius(radio, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(radio, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(radio, ui::Fill(dt::kColorBand), 0);
    lv_obj_set_style_bg_color(radio, ui::Level(dt::kColorSurfaceOn), LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(radio, ui::Fill(dt::kColorBand), LV_STATE_DISABLED);
    lv_obj_set_style_bg_color(radio, ui::Fill(dt::kColorBand), kOnDisabled);
    detail::Outline(radio, dt::kSpacingStrokeDefault);
    lv_obj_set_style_border_opa(radio, LV_OPA_TRANSP, LV_STATE_DISABLED);

    // The dot: white on the black, shown only while chosen — the toggle's thumb, 16 px so it reads as chosen, not as a ring.
    lv_obj_t *dot = detail::Bare(radio);
    lv_obj_set_size(dot, dt::kSpacingCheckDot, dt::kSpacingCheckDot);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, ui::Level(dt::kColorThumbOn), 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(dot, detail::DisabledOnFace(), kOnDisabled);
    lv_obj_center(dot);
    lv_obj_add_flag(radio, LV_OBJ_FLAG_STATE_TRICKLE);
    lv_obj_add_event_cb(radio, OnClicked, LV_EVENT_CLICKED, nullptr);

    lv_obj_set_ext_click_area(radio, (dt::kSpacingTarget - dt::kSpacingCheckSize) / 2);
    ui::RingOutside(radio);
    detail::JoinGroup(radio);
    return radio;
}

}  // namespace widgets
