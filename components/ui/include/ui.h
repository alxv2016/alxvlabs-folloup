// DESIGN.md's look in LVGL: Bitcount Single at each type role, and one style per component, built from the token header
// (design_tokens.h). No LVGL theme is compiled in, so nothing carries a pressed tint, a transition or a shadow the glass
// cannot show; focus is the ring, chosen is the inversion (DESIGN → Focus and input). Call under lvgl_display::Lock().

#pragma once

#include <cstdint>

#include "esp_err.h"
#include "line_box.h"
#include "lvgl.h"

namespace ui {

// Loads the fonts and builds the styles; once, before the first screen.
esp_err_t Start();

// --- Fonts and styles from DESIGN.md -------------------------------------------------------------------------------

enum class Type : uint8_t { kHero, kDisplay, kHeadline, kBody, kLabel, kCaption, kCaptionLoose, kCaptionStrong };
const lv_font_t *Font(Type role);
int16_t LineBox(Type role);

// Bitcount Single at one bit a pixel, compiled by fonts/bitcount/make_fonts.sh at weights 600 and 700: 20 to 80 px in
// steps of 10 with ASCII and · ° × – — ’ • …, and 100, 120 and 160 px with figures only; nullptr for any other
// size or weight. The roles take theirs through Font (STICKY_DESIGN_TOKENS §3).
const lv_font_t *Bitcount(int16_t size_px, int16_t weight);

// A colour token's level as LVGL's colour: 0 → #000000 … 3 → #FFFFFF.
lv_color_t Level(uint8_t level);
// A fill's colour from its level: levels 1 and 2 as their dither codes, so a grey surface survives partial refreshes
// as a pattern (lvgl_display::kDitherDark, kDitherLight); text and lines take Level (STICKY_DESIGN_TOKENS §4). Levels 4 and 5,
// the sparse dithers (design_tokens::kColorTextDisabled and kColorTextDisabledPage), are the one text colour drawn as a
// pattern: a disabled control's words take Fill too, the one-bit font drawing each stroke pixel as the code.
lv_color_t Fill(uint8_t level);

// The components, built from DESIGN.md's tokens. Each returns the object to place; a control is added to lvgl_display::Group().
lv_obj_t *NewScreen();                                         // the glass, no scrolling
lv_obj_t *NewStatusBar(lv_obj_t *parent, const char *title);   // status-bar: caption, 44 px, a hairline below
lv_obj_t *NewLabel(lv_obj_t *parent, Type role, const char *text, bool secondary = false);
// FitLineBox (line_box.h) with the role's line height token as the line box (STICKY_DESIGN_TOKENS §3, §9).
void FitLineBox(lv_obj_t *label, Type role);
lv_obj_t *NewRow(lv_obj_t *parent, const char *label, const char *supporting, lv_event_cb_t on_click,
                 void *user_data);                             // list-row: 80 px, rounded.lg, its ring outside, no rule
lv_obj_t *NewButton(lv_obj_t *parent, const char *label, bool action, lv_event_cb_t on_click,
                    void *user_data);                          // button-action or button-outline, 64 px
lv_obj_t *NewDivider(lv_obj_t *parent);
void SetChosen(lv_obj_t *control, bool chosen);                // list-row-chosen: inverted, never focus
// The one focus ring, for every control: outside the object, spacing.focus-offset clear of its edge and following its
// corners, black — for a control with spacing.md of room round it, whose containers let it out
// (DESIGN → Components → States; STICKY_DESIGN_TOKENS §9). The buttons' focus is LV_STATE_FOCUS_KEY.
// `inset` is for an object drawn that far inside the box its ring belongs to: the ring stands that much further out.
void RingOutside(lv_obj_t *obj, int32_t inset = 0);
// Lets the rings of an object's children out of it: LVGL clips a child to its parent, and a ring reaches
// spacing.focus-offset + spacing.stroke.edit past its control. Call it on every container between a control and the first
// ancestor with that much room (STICKY_DESIGN_TOKENS §9).
void LetRingOut(lv_obj_t *obj);
// Brings `show` (the object itself when null) and its ring into view when `obj` takes the focus, at once: the pages round
// it scroll, with no animation — LVGL's own scroll on focus animates, which the glass would draw frame by frame. Nothing
// happens on a screen that does not scroll. NewRow (with a tap) and NewButton do it already; call it for any other control
// in the focus group, and with `show` for a control inside a view that turns pages itself, so the page brings that view
// into view and never the control by a pixel (STICKY_DESIGN_TOKENS §9).
void ShowWhenFocused(lv_obj_t *obj, lv_obj_t *show = nullptr);
// How high a surface stands off the page: each level has its own dither for its offset (design_tokens.h `kElevation1` to `3`),
// one table in ui.cpp (DESIGN → Elevation & Depth). Flat draws none.
enum class Elevation : uint8_t { kFlat, kRaised, kAnchored, kFloating };
// Which edge of the surface's box shows its offset: a floating box stands clear of the page on every side, so its offset falls
// down and to the right; a sheet hangs off the glass's bottom and sides, so only its top edge is exposed and its offset rises
// from it.
enum class OffsetSide : uint8_t { kBottomRight, kTop };
// Gives `box` an offset: its own shape, `spacing.overlay-offset` away on `side`, in the dither of `level`, ringed on its outer
// side by a white `spacing.stroke.hairline` keyline so it keeps an edge over a dithered ground — drawn before the box, a hard
// offset, never a blur or solid black. It follows the box's corner radius. The offset stands outside the box, so the box's
// parents let it out (`LetRingOut` does, for its reach is larger).
void AddOffset(lv_obj_t *box, Elevation level, OffsetSide side);
// The ring of an object open for editing (LV_STATE_EDITED, a range slider after OK): spacing.stroke.edit wide, still
// spacing.focus-offset outside it — the weight says UP and DOWN now change its value. Call after RingOutside.
void RingEditing(lv_obj_t *obj, int32_t inset = 0);  // `inset` as RingOutside's

}  // namespace ui
