#include "widgets/chip.h"

#include "design_tokens.h"
#include "internal/base.h"

namespace widgets {
namespace dt = design_tokens;

lv_obj_t *MakeChip(lv_obj_t *parent, const char *text)
{
    lv_obj_t *chip = lv_button_create(parent);
    lv_obj_remove_style_all(chip);  // touch-exception: no pressed state (DESIGN → Motion → Press)
    lv_obj_set_scrollable(chip, false);
    lv_obj_set_size(chip, LV_SIZE_CONTENT, dt::kSpacingTarget);
    lv_obj_set_style_radius(chip, dt::kRoundedLg, 0);
    lv_obj_set_style_pad_hor(chip, dt::kSpacingLg, 0);
    lv_obj_set_style_bg_opa(chip, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(chip, ui::Fill(dt::kColorBand), 0);  // the deck's chip is a filled box, black words on it
    lv_obj_set_style_text_font(chip, ui::Font(ui::Type::kLabel), 0);
    lv_obj_set_style_text_color(chip, ui::Level(dt::kColorTextPrimary), 0);
    // Chosen is the edge: reserved at rest and transparent, drawn black when chosen.
    lv_obj_set_style_border_width(chip, dt::kSpacingStrokeStrong, 0);
    lv_obj_set_style_border_color(chip, ui::Level(dt::kColorOutline), 0);
    lv_obj_set_style_border_opa(chip, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(chip, LV_OPA_COVER, LV_STATE_CHECKED);
    // Disabled: the light dither it already has, with no edge even when chosen, and its words `text-disabled`.
    detail::DitherWhenDisabled(chip);
    lv_obj_set_style_border_opa(chip, LV_OPA_TRANSP, static_cast<lv_style_selector_t>(LV_STATE_DISABLED) | LV_STATE_CHECKED);
    lv_obj_t *label = lv_label_create(chip);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    detail::Focusable(chip);
    return chip;
}

}  // namespace widgets
