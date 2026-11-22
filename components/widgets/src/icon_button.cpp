#include "widgets/icon_button.h"

#include "design_tokens.h"
#include "internal/base.h"
#include "internal/button_face.h"
#include "internal/icon_button_sized.h"
#include "internal/marks.h"

namespace widgets {
namespace dt = design_tokens;

namespace detail {
Glyph GlyphOfAction(IconAction action)
{
    switch (action) {
    case IconAction::kBack:      return Glyph::kBack;
    case IconAction::kClose:     return Glyph::kClose;
    case IconAction::kClear:     return Glyph::kClose;
    case IconAction::kPlay:      return Glyph::kPlay;
    case IconAction::kPause:     return Glyph::kPause;
    case IconAction::kStop:      return Glyph::kStop;
    case IconAction::kEyeOpen:   return Glyph::kEyeOpen;
    case IconAction::kEyeSlashed: return Glyph::kEyeSlashed;
    }
    return Glyph::kNone;
}

lv_obj_t *MakeIconButtonSized(lv_obj_t *parent, IconAction action, ButtonVariant variant, int32_t size, int32_t radius,
                              const char *action_words)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_remove_style_all(button);  // touch-exception: no pressed state (DESIGN → Motion → Press)
    lv_obj_set_scrollable(button, false);
    lv_obj_set_size(button, size, size);
    StyleButtonFace(button, variant);
    lv_obj_set_style_radius(button, radius, 0);
    // The glyph is white on the black action button and black on the others, `text-disabled` when disabled: it takes the
    // states the button passes down.
    const uint8_t ink = variant == ButtonVariant::kAction ? dt::kColorTextOnFill : dt::kColorTextPrimary;
    lv_obj_add_flag(button, LV_OBJ_FLAG_STATE_TRICKLE);
    lv_obj_center(NewMark(button, GlyphImage(GlyphOfAction(action)), ink, ink));
    lv_obj_set_user_data(button, const_cast<char *>(action_words));
    Focusable(button);
    return button;
}
}  // namespace detail

lv_obj_t *MakeIconButton(lv_obj_t *parent, IconAction action, ButtonVariant variant, const char *action_words)
{
    return detail::MakeIconButtonSized(parent, action, variant, dt::kSpacingTarget, dt::kRoundedLg, action_words);
}

const char *IconButtonWords(lv_obj_t *button)
{
    return static_cast<const char *>(lv_obj_get_user_data(button));
}

}  // namespace widgets
