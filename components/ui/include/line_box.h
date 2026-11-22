// A label that is whole line boxes tall, its words centred in each. Only LVGL is needed, so the arithmetic is tested on the
// computer against LVGL's real label code (tests/host/fit_line_box_lvgl_test.cpp).

#pragma once

#include <cstdint>

#include "lvgl.h"

namespace ui {

// Makes a label whole line boxes tall, its words centred in each: the line box (a role's line height token or any
// primitive's) less the label's compiled font, split above and below as padding, the odd pixel above. The font alone is
// shorter than the line box and LVGL draws a line from the top of its text area, so a bare label stands closer to the one
// beside it than its role's pitch; a name over a detail line stands on line boxes (STICKY_DESIGN_TOKENS §9
// is the rule, STICKY_DESIGN_TOKENS §3 gives a role's line). Call it after the label's font and long mode are set. A dotted
// label, which LVGL ends in "..." only at a fixed height, is made one line box tall; a wrapped one is n line boxes tall for n lines, given
// `text-line-space` of the line box less the font.
void FitLineBox(lv_obj_t *label, int32_t line_px);

}  // namespace ui
