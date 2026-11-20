// DESIGN.md's look in LVGL: Inter at each type role, and one style per component, built from the token header
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

}  // namespace ui
