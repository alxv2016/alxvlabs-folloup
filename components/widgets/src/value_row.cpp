#include "widgets/value_row.h"

#include "design_tokens.h"
#include "internal/base.h"

namespace widgets {
namespace dt = design_tokens;

ValueRow MakeValueRow(lv_obj_t *parent, const char *name, const char *value)
{
    ValueRow row{};
    row.root = lv_button_create(parent);
    lv_obj_remove_style_all(row.root);  // touch-exception: no pressed state (DESIGN → Motion → Press)
    lv_obj_set_scrollable(row.root, false);
    lv_obj_set_size(row.root, lv_pct(100), dt::kSpacingTarget);
    lv_obj_set_flex_flow(row.root, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row.root, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    // The words are spacing.md in from the row's edge: the padding is that less the edge the row reserves below.
    lv_obj_set_style_pad_hor(row.root, dt::kSpacingMd - dt::kSpacingStrokeDefault, 0);
    lv_obj_set_style_pad_column(row.root, dt::kSpacingMd, 0);
    lv_obj_set_style_radius(row.root, dt::kRoundedLg, 0);  // a row has no fill: its corners shape its ring
    // The edge a row reserves is never drawn: a disabled row has none either, and its words do not move when it is disabled.
    lv_obj_set_style_border_width(row.root, dt::kSpacingStrokeDefault, 0);
    lv_obj_set_style_border_opa(row.root, LV_OPA_TRANSP, 0);
    detail::DitherWhenDisabled(row.root);
    row.label = detail::Line(row.root, ui::Type::kLabel, name, false);
    row.value = detail::Line(row.root, ui::Type::kLabel, value, true);
    // The value is text-secondary on white and `text-disabled` on the disabled row's dither (DESIGN → Components → States):
    // the row passes its state to its label, whose own colour would otherwise outrank the row's.
    lv_obj_add_flag(row.root, LV_OBJ_FLAG_STATE_TRICKLE);
    lv_obj_set_style_text_color(row.value, detail::DisabledOnFace(), LV_STATE_DISABLED);
    lv_obj_set_flex_grow(row.value, 1);
    lv_obj_set_style_text_align(row.value, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_width(row.value, 1);  // the grow takes the room left; the value shortens with LVGL's dots, never the name
    detail::Focusable(row.root);
    return row;
}

void SetValueRow(const ValueRow &row, const char *value)
{
    lv_label_set_text(row.value, value);
}

}  // namespace widgets
