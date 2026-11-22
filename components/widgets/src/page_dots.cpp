#include "widgets/page_dots.h"

#include "design_tokens.h"
#include "internal/base.h"

namespace widgets {
namespace dt = design_tokens;

lv_obj_t *MakePageDots(lv_obj_t *parent, int count, int current)
{
    lv_obj_t *dots = detail::Bare(parent);
    lv_obj_set_size(dots, LV_SIZE_CONTENT, dt::kSpacingPageDot);
    lv_obj_set_flex_flow(dots, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(dots, dt::kSpacingSm, 0);
    count = count > kPageDotsMax ? kPageDotsMax : count;
    for (int i = 0; i < count; ++i) {
        lv_obj_t *dot = detail::Pill(dots);
        lv_obj_set_size(dot, dt::kSpacingPageDot, dt::kSpacingPageDot);
        lv_obj_set_style_bg_color(dot, ui::Level(dt::kColorBackground), 0);  // hollow: the page behind, a hairline edge
        detail::Outline(dot, dt::kSpacingStrokeHairline);
        lv_obj_set_style_bg_color(dot, ui::Level(dt::kColorSurfaceAction), LV_STATE_CHECKED);
        lv_obj_set_style_width(dot, dt::kSpacingPagePill, LV_STATE_CHECKED);
    }
    SetPageDot(dots, current);
    return dots;
}

void SetPageDot(lv_obj_t *dots, int current)
{
    const uint32_t n = lv_obj_get_child_count(dots);
    for (uint32_t i = 0; i < n; ++i) {
        lv_obj_set_state(lv_obj_get_child(dots, static_cast<int32_t>(i)), LV_STATE_CHECKED,
                         static_cast<int>(i) == current);
    }
}

}  // namespace widgets
