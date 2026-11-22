#include "widgets/section_heading.h"

#include "design_tokens.h"
#include "internal/base.h"

namespace widgets {
namespace dt = design_tokens;

lv_obj_t *MakeSectionHeading(lv_obj_t *parent, const char *text)
{
    lv_obj_t *heading = ui::NewLabel(parent, ui::Type::kCaption, text, true);
    lv_obj_set_style_pad_hor(heading, dt::kSpacingMd, 0);
    lv_obj_set_style_pad_top(heading, dt::kSpacingLg, 0);
    return heading;
}

}  // namespace widgets
