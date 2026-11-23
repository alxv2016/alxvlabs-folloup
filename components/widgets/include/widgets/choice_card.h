// A choice card: a card of words on the light dither with a ring mark (one choice of a set) or a box (any number);
// chosen is the mark alone — its dot or its check (DESIGN → Components → The kit).

#pragma once

#include "lvgl.h"

namespace widgets {

// A card at least `spacing.mark` and two `spacing.md` high, full width, its words — a name in bold over an optional
// supporting line — wrapping; `rounded.lg`, its words `spacing.card-inset` in; in the focus group with its ring outside it. Its
// parents let the ring out and keep `spacing.md` between cards.
// A group is the cards of one choice that share a parent, as a radio button row's is. OK on a card, or a tap on the lift,
// chooses it: the others of its group are cleared, then LV_EVENT_VALUE_CHANGED is sent on it. Choosing the chosen one sends
// nothing; moving the focus never chooses. A card of any number (`many`) is a group of one: it flips and sends the event each
// time. SetChoiceChosen sets a card by the program, clearing the group's others when it chooses one, and sends nothing. The
// caller listens for LV_EVENT_VALUE_CHANGED and reads ChoiceChosen. The card's user data stays the caller's: the group is kept in
// the card's own listener. Disabled (LV_STATE_DISABLED) it is the light dither with no edge, its words `text-disabled`, the mark a
// white disc or box with no edge and a chosen dot or check `text-disabled-page` on it.
lv_obj_t *MakeChoiceCard(lv_obj_t *parent, const char *text, bool many, const char *supporting = nullptr);
void SetChoiceChosen(lv_obj_t *card, bool chosen);
bool ChoiceChosen(lv_obj_t *card);

namespace detail {
// A choice's rules, shared by the radio row and the choice card: `MakeChoice` on the row or card makes it choose on OK or a
// tap (LV_EVENT_CLICKED) and says so (LV_EVENT_VALUE_CHANGED), as the choice card's and the radio row's header say; `SetChoice`
// sets it by the program, clearing its group when chosen, and sends nothing.
void MakeChoice(lv_obj_t *obj, bool many);
void SetChoice(lv_obj_t *obj, bool chosen);
// The mark: a `spacing.mark` ring (or a box when `many`) with a `spacing.stroke.strong` edge, whose `spacing.mark-dot`
// dot (or check) shows when its parent is chosen.
lv_obj_t *MakeMark(lv_obj_t *parent, bool many);
}  // namespace detail

}  // namespace widgets
