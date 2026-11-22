#include "widgets/detail_row.h"

#include "design_tokens.h"
#include "internal/base.h"

namespace widgets {
namespace dt = design_tokens;

DetailRow MakeDetailRow(lv_obj_t *parent, const char *name, const char *value)
{
    DetailRow row{};
    row.root = detail::Bare(parent);
    lv_obj_set_size(row.root, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row.root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_hor(row.root, dt::kSpacingMd, 0);
    lv_obj_set_style_pad_ver(row.root, dt::kSpacingSm, 0);
    lv_obj_set_style_pad_row(row.root, 0, 0);  // the two lines meet: 8 + 24 + 36 + 8 = 76
    row.label = detail::Line(row.root, ui::Type::kCaption, name, true);
    row.value = detail::Line(row.root, ui::Type::kLabel, value, false);
    lv_obj_set_width(row.label, lv_pct(100));
    lv_obj_set_width(row.value, lv_pct(100));
    return row;
}

void SetDetailRow(const DetailRow &row, const char *value)
{
    lv_label_set_text(row.value, value);
}

}  // namespace widgets
