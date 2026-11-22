// The face every button variant shares, for a button with words (button.cpp) and one with a glyph alone (icon_button.cpp):
// an edge reserved at rest and drawn only by the outline variant, `rounded.lg`, the variant's fill and words, and the
// disabled look (DESIGN → Components → The kit, States). Private to the widgets component.

#pragma once

#include "lvgl.h"
#include "widgets/button.h"

namespace widgets::detail {

// Styles a bare LVGL button as `variant`: its size, padding and children are the caller's.
void StyleButtonFace(lv_obj_t *button, ButtonVariant variant);

}  // namespace widgets::detail
