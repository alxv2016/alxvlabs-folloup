// DESIGN.md's look in LVGL: Bitcount Single at each type role, and one style per component, built from the token header
// (design_tokens.h). No LVGL theme is compiled in, so nothing carries a pressed tint, a transition or a shadow the glass
// cannot show; focus is the ring, chosen is the inversion (DESIGN → Focus and input). Call under lvgl_display::Lock().

#pragma once

#include <cstdint>

#include "esp_err.h"
#include "lvgl.h"

namespace ui {

// Loads the fonts and builds the styles; once, before the first screen.
esp_err_t Start();

// --- Fonts and styles from DESIGN.md -------------------------------------------------------------------------------

enum class Type : uint8_t { kHero, kDisplay, kHeadline, kBody, kLabel, kCaption };
const lv_font_t *Font(Type role);
int16_t LineBox(Type role);

// Bitcount Single at one bit a pixel, compiled by fonts/bitcount/make_fonts.sh at weights 600 and 700: 20 to 80 px in
// steps of 10 with ASCII and · ° × – — ’ • …, and 100, 120 and 160 px with figures only; nullptr for any other
// size or weight. The roles take theirs through Font (STICKY_DESIGN_TOKENS §3).
const lv_font_t *Bitcount(int16_t size_px, int16_t weight);

// A colour token's level as LVGL's colour: 0 → #000000 … 3 → #FFFFFF.
lv_color_t Level(uint8_t level);
// A fill's colour from its level: levels 1 and 2 as their dither codes, so a grey surface survives partial refreshes
// as a pattern (lvgl_display::kDitherDark, kDitherLight); text and lines take Level (STICKY_DESIGN_TOKENS §4).
lv_color_t Fill(uint8_t level);

// The components, built from DESIGN.md's tokens. Each returns the object to place; a control is added to lvgl_display::Group().
lv_obj_t *NewScreen();                                         // the glass, no scrolling
lv_obj_t *NewStatusBar(lv_obj_t *parent, const char *title);   // status-bar: caption, 44 px, a hairline below
lv_obj_t *NewLabel(lv_obj_t *parent, Type role, const char *text, bool secondary = false);
lv_obj_t *NewRow(lv_obj_t *parent, const char *label, const char *supporting, lv_event_cb_t on_click,
                 void *user_data);                             // list-row, 80 px, a divider below
lv_obj_t *NewButton(lv_obj_t *parent, const char *label, bool action, lv_event_cb_t on_click,
                    void *user_data);                          // button-action or button-outline, 64 px
lv_obj_t *NewDivider(lv_obj_t *parent);
void SetChosen(lv_obj_t *control, bool chosen);                // list-row-chosen: inverted, never focus
void RingOnFocus(lv_obj_t *obj);                               // the focus ring, on a focusable object a feature makes

}  // namespace ui
