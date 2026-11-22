#include "widgets/checkbox.h"

#include "design_tokens.h"
#include "internal/base.h"
#include "internal/marks.h"

namespace widgets {
namespace dt = design_tokens;

lv_obj_t *MakeCheckbox(lv_obj_t *parent)
{
    constexpr lv_style_selector_t kOnDisabled = static_cast<lv_style_selector_t>(LV_STATE_DISABLED) | LV_STATE_CHECKED;
    lv_obj_t *box = detail::Bare(parent);
    lv_obj_add_flag(box, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(box, LV_OBJ_FLAG_CHECKABLE);  // OK or a tap flips LV_STATE_CHECKED and sends LV_EVENT_VALUE_CHANGED
    lv_obj_set_size(box, dt::kSpacingCheckSize, dt::kSpacingCheckSize);
    lv_obj_set_style_radius(box, dt::kRoundedMd, 0);
    // Off: the light dither inside a 3 px outline, the one edge that makes an off box findable (a dither alone is 1.3 : 1 on
    // white). On: black, the edge and the fill one shape. Disabled: the dither either way, no edge, the size kept.
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(box, ui::Fill(dt::kColorBand), 0);
    lv_obj_set_style_bg_color(box, ui::Level(dt::kColorSurfaceOn), LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(box, ui::Fill(dt::kColorBand), LV_STATE_DISABLED);
    lv_obj_set_style_bg_color(box, ui::Fill(dt::kColorBand), kOnDisabled);
    detail::Outline(box, dt::kSpacingStrokeDefault);
    lv_obj_set_style_border_opa(box, LV_OPA_TRANSP, LV_STATE_DISABLED);

    // The check: BYTE-90's at one bit, white on the black; shown only while checked. It takes the box's states.
    lv_obj_t *check = detail::NewMark(box, &widgets_mark_glyph_check, dt::kColorThumbOn, dt::kColorThumbOn);
    lv_obj_set_style_image_opa(check, LV_OPA_TRANSP, 0);
    lv_obj_set_style_image_opa(check, LV_OPA_COVER, LV_STATE_CHECKED);
    lv_obj_center(check);
    lv_obj_add_flag(box, LV_OBJ_FLAG_STATE_TRICKLE);

    // Touches across spacing.target: the 44 px box padded 10 px a side.
    lv_obj_set_ext_click_area(box, (dt::kSpacingTarget - dt::kSpacingCheckSize) / 2);
    // The ring is outside the box, 4 px clear of it, following its corners.
    ui::RingOutside(box);
    detail::JoinGroup(box);
    return box;
}

}  // namespace widgets
