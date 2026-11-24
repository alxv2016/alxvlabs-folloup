#include "widgets/section_heading.h"

#include "design_tokens.h"
#include "internal/base.h"

namespace widgets {
namespace dt = design_tokens;

lv_obj_t *MakeSectionHeading(lv_obj_t *parent, const char *text, HeadingX x)
{
    const bool first = lv_obj_get_child_count(parent) == 0;
    lv_obj_t *heading = ui::NewLabel(parent, ui::Type::kCaption, text, true);
    lv_obj_set_style_pad_left(heading, x == HeadingX::kRowWords ? dt::kSpacingMd : 0, 0);
    lv_obj_set_style_pad_right(heading, dt::kSpacingMd, 0);
    // `section` over the heading: the parent's own gap above it is part of it.
    const int32_t gap = lv_obj_get_style_pad_row(parent, LV_PART_MAIN);
    lv_obj_set_style_pad_top(heading, first ? dt::kSpacingLg : dt::kSpacingSection - gap, 0);
    return heading;
}

}  // namespace widgets
