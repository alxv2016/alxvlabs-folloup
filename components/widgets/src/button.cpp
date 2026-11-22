#include "widgets/button.h"

#include "design_tokens.h"
#include "internal/base.h"
#include "internal/button_face.h"
#include "internal/marks.h"

namespace widgets {
namespace dt = design_tokens;

namespace detail {
void StyleButtonFace(lv_obj_t *button, ButtonVariant variant)
{
    // The edge is reserved at rest on every variant, so the compact buttons are as wide as each other's words.
    lv_obj_set_style_border_width(button, dt::kSpacingStrokeDefault, 0);
    lv_obj_set_style_border_color(button, ui::Level(dt::kColorOutline), 0);
    lv_obj_set_style_border_opa(button, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(button, dt::kRoundedLg, 0);  // the deck's one base radius
    lv_obj_set_style_text_font(button, ui::Font(ui::Type::kLabel), 0);
    lv_obj_set_style_text_color(button, ui::Level(dt::kColorTextPrimary), 0);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(button, ui::Level(dt::kColorBackground), 0);
    switch (variant) {
    case ButtonVariant::kAction:
        lv_obj_set_style_bg_color(button, ui::Level(dt::kColorSurfaceAction), 0);
        lv_obj_set_style_text_color(button, ui::Level(dt::kColorTextOnFill), 0);
        break;
    case ButtonVariant::kOutline:
        lv_obj_set_style_border_opa(button, LV_OPA_COVER, 0);
        break;
    case ButtonVariant::kTonal:
        lv_obj_set_style_bg_color(button, ui::Fill(dt::kColorBand), 0);  // black words only on the light dither
        break;
    case ButtonVariant::kGhost:
        lv_obj_set_style_bg_opa(button, LV_OPA_TRANSP, 0);
        break;
    }
    DitherWhenDisabled(button);
}
}  // namespace detail

lv_obj_t *MakeButton(lv_obj_t *parent, const char *label, ButtonVariant variant, int32_t width, Glyph glyph)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_remove_style_all(button);  // touch-exception: no pressed state (DESIGN → Motion → Press)
    lv_obj_set_scrollable(button, false);
    lv_obj_set_size(button, width > 0 ? width : LV_SIZE_CONTENT, dt::kSpacingTarget);
    detail::StyleButtonFace(button, variant);
    // Label room: spacing.md either side in a button of a set width, which holds its words inside the edge; spacing.lg in a
    // compact one, as wide as its words (DESIGN → Components → The kit).
    lv_obj_set_style_pad_hor(button, width > 0 ? dt::kSpacingMd : dt::kSpacingLg, 0);
    lv_obj_t *text = lv_label_create(button);
    lv_label_set_text(text, label);
    if (const lv_image_dsc_t *mark = detail::GlyphImage(glyph)) {
        // The glyph and the words are one group, centred, spacing.md between the ink and the first letter (the mark is
        // trimmed to its ink). The glyph takes the words' colour, and `text-disabled` when disabled, from the states
        // the button passes down.
        const uint8_t words = variant == ButtonVariant::kAction ? dt::kColorTextOnFill : dt::kColorTextPrimary;
        lv_obj_set_flex_flow(button, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(button, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(button, dt::kSpacingMd, 0);
        lv_obj_add_flag(button, LV_OBJ_FLAG_STATE_TRICKLE);
        lv_obj_move_to_index(detail::NewMark(button, mark, words, words), 0);
    } else {
        lv_obj_center(text);
    }
    detail::Focusable(button);
    return button;
}

}  // namespace widgets
