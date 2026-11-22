#include "line_box.h"

namespace ui {

void FitLineBox(lv_obj_t *label, int32_t line_px)
{
    // The spare room goes half above and half below, so the label is whole line boxes tall and its words stand in the
    // middle of each.
    const int32_t spare = line_px - lv_font_get_line_height(lv_obj_get_style_text_font(label, LV_PART_MAIN));
    if (spare > 0) {
        lv_obj_set_style_pad_top(label, (spare + 1) / 2, 0);
        lv_obj_set_style_pad_bottom(label, spare / 2, 0);
    }
    if (lv_label_get_long_mode(label) == LV_LABEL_LONG_MODE_DOTS) {
        lv_obj_set_height(label, line_px);  // one line, padding included
    }
}

}  // namespace ui
