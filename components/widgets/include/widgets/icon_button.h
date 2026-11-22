// The icon button: a square of `spacing.target` with a glyph and no word, in the button's four variants — an exception with
// a closed set (DESIGN → Exceptions → The icon button): Back, Close, Play, Pause and Stop — and, inside a text field, Close as
// the clear and the eye's two faces (text_input.h). Sound on and off is the toggle icon button's own pair
// (toggle_icon_button.h).

#pragma once

#include "lvgl.h"
#include "widgets/button.h"
#include "widgets/glyph.h"

namespace widgets {

// A button `spacing.target` square, drawn and hit alike, the glyph of `action` centred and nothing else, in the focus group
// with its ring outside it, following `rounded.lg`. The variants as the button's: action black with a white glyph, outline
// a 3 px edge, tonal the light dither, ghost the glyph alone. No pressed state: the tick and the next frame answer a press
// (DESIGN → Motion → Press). The caller listens for LV_EVENT_CLICKED. Disabled (lv_obj_add_state(…, LV_STATE_DISABLED)) it is
// the light dither with the glyph `text-disabled`, in every variant alike.
//
// There is no screen reader, so a glyph is the button's only name: `action_words` is the action in words — "Play",
// "Go back" — for the footer's hint for OK while the button is focused (IconButtonWords); kept by pointer, so a literal.
lv_obj_t *MakeIconButton(lv_obj_t *parent, IconAction action, ButtonVariant variant, const char *action_words);
// The words MakeIconButton was given, for the hint.
const char *IconButtonWords(lv_obj_t *button);

}  // namespace widgets
