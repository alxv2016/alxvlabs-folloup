// A choice card: a card of words on the light dither with a ring mark (one choice of a set) or a box (any number);
// chosen is the mark alone — its dot or its check (DESIGN → Components → The kit).

#pragma once

#include "lvgl.h"

namespace widgets {

// A card at least `spacing.mark` and two `spacing.md` high, full width, its words — a name in bold over an optional
// supporting line — wrapping; `rounded.lg`, its words `spacing.card-inset` in; in the focus group with its ring outside it. The
// caller listens for LV_EVENT_CLICKED and sets the choice with SetChoiceChosen. Its parents let the ring out and keep
// `spacing.md` between cards. Disabled (LV_STATE_DISABLED) it is the light dither with no edge, its words `text-disabled`,
// the mark a white disc or box with no edge and a chosen dot or check `text-disabled-page` on it.
lv_obj_t *MakeChoiceCard(lv_obj_t *parent, const char *text, bool many, const char *supporting = nullptr);
void SetChoiceChosen(lv_obj_t *card, bool chosen);
bool ChoiceChosen(lv_obj_t *card);

namespace detail {
// The mark: a `spacing.mark` ring (or a box when `many`) with a `spacing.stroke.strong` edge, whose `spacing.mark-dot`
// dot (or check) shows when its parent is chosen.
lv_obj_t *MakeMark(lv_obj_t *parent, bool many);
}  // namespace detail

}  // namespace widgets
