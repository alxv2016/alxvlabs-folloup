#include "widgets/toggle_icon_button.h"

#include "design_tokens.h"
#include "internal/base.h"
#include "internal/marks.h"

namespace widgets {
namespace dt = design_tokens;

lv_obj_t *MakeToggleIconButton(lv_obj_t *parent)
{
    constexpr lv_style_selector_t kOnDisabled = static_cast<lv_style_selector_t>(LV_STATE_DISABLED) | LV_STATE_CHECKED;
    lv_obj_t *tile = lv_button_create(parent);
    lv_obj_remove_style_all(tile);  // touch-exception: no pressed state (DESIGN → Motion → Press)
    lv_obj_set_scrollable(tile, false);
    lv_obj_add_flag(tile, LV_OBJ_FLAG_CHECKABLE);  // OK or a tap flips LV_STATE_CHECKED and sends LV_EVENT_VALUE_CHANGED
    lv_obj_set_size(tile, dt::kSpacingTarget, dt::kSpacingTarget);
    lv_obj_set_style_radius(tile, dt::kRoundedLg, 0);
    // A tonal tile that goes black when on; disabled the light dither either way, with no edge.
    lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(tile, ui::Fill(dt::kColorBand), 0);
    lv_obj_set_style_bg_color(tile, ui::Level(dt::kColorSurfaceOn), LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(tile, ui::Fill(dt::kColorBand), LV_STATE_DISABLED);
    lv_obj_set_style_bg_color(tile, ui::Fill(dt::kColorBand), kOnDisabled);
    // The glyph shows the current state: the speaker with waves when on, with a slash when off. Each is drawn in the tile's
    // ink — black on the dither, white on the black — and only in its state; the tile passes its states down.
    lv_obj_t *on = detail::NewMark(tile, &widgets_mark_glyph_speaker_on, dt::kColorTextPrimary, dt::kColorTextOnFill);
    lv_obj_t *off = detail::NewMark(tile, &widgets_mark_glyph_speaker_off, dt::kColorTextPrimary, dt::kColorTextOnFill);
    lv_obj_set_style_image_opa(on, LV_OPA_TRANSP, 0);
    lv_obj_set_style_image_opa(on, LV_OPA_COVER, LV_STATE_CHECKED);
    lv_obj_set_style_image_opa(off, LV_OPA_COVER, 0);
    lv_obj_set_style_image_opa(off, LV_OPA_TRANSP, LV_STATE_CHECKED);
    lv_obj_center(on);
    lv_obj_center(off);
    lv_obj_add_flag(tile, LV_OBJ_FLAG_STATE_TRICKLE);
    detail::Focusable(tile);
    return tile;
}

}  // namespace widgets
