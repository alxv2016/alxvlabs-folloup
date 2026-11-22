#include "widgets/dialog.h"

#include "design_tokens.h"
#include "internal/base.h"
#include "widgets/button.h"

namespace widgets {
namespace dt = design_tokens;

Dialog MakeDialog(lv_obj_t *parent, const char *title, const char *message, const char *action, const char *cancel)
{
    // Two half buttons in the box's inside: the content, less the outline and the padding either side.
    constexpr int32_t kInside = dt::kSpacingContent - 2 * dt::kSpacingStrokeStrong - 2 * dt::kSpacingLg;
    constexpr int32_t kHalf = (kInside - dt::kSpacingMd) / 2;
    Dialog dialog{};
    dialog.frame = detail::Bare(parent);
    lv_obj_set_size(dialog.frame, lv_pct(100), LV_SIZE_CONTENT);
    detail::LetRingOut(dialog.frame);  // also lets the offset out of the frame: LVGL clips a child to its parent
    dialog.box = detail::Bare(dialog.frame);
    lv_obj_set_size(dialog.box, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(dialog.box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(dialog.box, dt::kSpacingLg, 0);
    lv_obj_set_style_pad_row(dialog.box, dt::kSpacingMd, 0);
    lv_obj_set_style_radius(dialog.box, dt::kRoundedMd, 0);
    lv_obj_set_style_bg_opa(dialog.box, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(dialog.box, ui::Level(dt::kColorBackground), 0);
    detail::Outline(dialog.box, dt::kSpacingStrokeStrong);
    ui::AddOffset(dialog.box, ui::Elevation::kFloating, ui::OffsetSide::kBottomRight);

    detail::Words(dialog.box, ui::Type::kLabel, title);
    detail::Words(dialog.box, ui::Type::kBody, message);
    lv_obj_t *buttons = detail::Bare(dialog.box);
    lv_obj_set_size(buttons, lv_pct(100), dt::kSpacingTarget);
    lv_obj_set_flex_flow(buttons, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(buttons, dt::kSpacingMd, 0);
    detail::LetRingOut(buttons);
    dialog.action = MakeButton(buttons, action, ButtonVariant::kAction, kHalf);
    dialog.cancel = MakeButton(buttons, cancel, ButtonVariant::kOutline, kHalf);
    return dialog;
}

}  // namespace widgets
