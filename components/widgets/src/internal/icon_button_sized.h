// The icon button's body at any size and corner, for the button inside a text field (DESIGN → Exceptions → The icon button);
// `MakeIconButton` calls it with `spacing.target` and `rounded.lg`. Private to the widgets component.

#pragma once

#include "lvgl.h"
#include "widgets/button.h"
#include "widgets/glyph.h"

namespace widgets::detail {

// A square button `size` a side with `radius` corners, the glyph of `action` centred, in the focus group with its ring outside it
// (the ring's corner is the button's radius plus the gap out to it, so a square button has a square-cornered ring). The face, the
// variants, the disabled look and the words for the hint are the icon button's own.
lv_obj_t *MakeIconButtonSized(lv_obj_t *parent, IconAction action, ButtonVariant variant, int32_t size, int32_t radius,
                              const char *action_words);

// The glyph an action draws.
Glyph GlyphOfAction(IconAction action);

}  // namespace widgets::detail
