// Two stacked texts stand a gap apart between their own boxes, the font's height with no leading (TYPOGRAPHY §17;
// DESIGN → Layout → Spacing). Private to the widgets component.

#pragma once

#include "lvgl.h"

namespace widgets::detail {

// Takes back the leading a text's line box adds below the upper text and above the lower one, as a negative margin, so the
// column's `pad_row` is the gap between the two texts' own boxes.
inline void Glue(lv_obj_t *top, lv_obj_t *bottom)
{
    lv_obj_set_style_margin_bottom(top, -lv_obj_get_style_pad_bottom(top, LV_PART_MAIN), 0);
    lv_obj_set_style_margin_top(bottom, -lv_obj_get_style_pad_top(bottom, LV_PART_MAIN), 0);
}

}  // namespace widgets::detail
