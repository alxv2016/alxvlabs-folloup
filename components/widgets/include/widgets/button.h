// The button: action, outline, tonal and ghost, a set width or as wide as its words (DESIGN → Components → The kit).

#pragma once

#include <cstdint>

#include "lvgl.h"
#include "widgets/glyph.h"

namespace widgets {

// Action: black, one per screen. Outline: the second action or Cancel. Tonal: the light dither, a second action that
// is not Cancel. Ghost: words alone, a third. A destructive action is the action button: its words carry the danger.
enum class ButtonVariant : uint8_t { kAction, kOutline, kTonal, kGhost };

// A button `spacing.target` high, `width` wide (0: as wide as its words, the compact button), in the focus group with its
// ring outside it; the standard one is `spacing.button-width` wide. A `spacing.stroke.default` edge is reserved on every
// variant so none changes size; only the outline variant draws it at rest. The caller listens for LV_EVENT_CLICKED.
// Disable it with lv_obj_add_state(…, LV_STATE_DISABLED) — before or after, whichever variant: the light dither with no edge,
// its words and glyph `text-disabled`, never focused and with no ring; the reason belongs in words under it.
//
// `glyph` stands before the words: `spacing.icon` of art, its ink then `spacing.md` of white from the first letter, the two
// centred as one group in the words' colour — white on the action button, black on the others. A half button holds 7
// characters beside one and a full-width button 19; a footer third holds none (DESIGN → Components → The kit). The glyphs
// beside words are Record (kMic), Play, Back, Save (kCheck) and Retry (kRefresh). No button has a pressed state.
lv_obj_t *MakeButton(lv_obj_t *parent, const char *label, ButtonVariant variant, int32_t width = 0,
                     Glyph glyph = Glyph::kNone);

}  // namespace widgets
