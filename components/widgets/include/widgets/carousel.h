// A carousel: onboarding, one card at a time (DESIGN → Components → The kit). One whole-width card fills the content column's height —
// white inside a `spacing.stroke.default` outline, `rounded.lg`, its title in `display` and its words in `body`, `spacing.card-inset`
// in — so nothing shifts from card to card; under it the page in words ("2 of 4", `caption` `text-secondary`) with the page dots
// `spacing.md` after them, an indicator and never a control; in the footer three fixed slots, Back (outline, absent on the first card),
// Skip (outline) and Next (action, reading "Done" on the last card), 133 px wide and `spacing.md` apart. The card is no focus stop.
//
// A turn redraws the card's words, the indicator and the footer's Back and Next in place — one partial refresh, nothing animates. A
// swipe left or right turns one card, decided on the lift (the swipe the pages share, widgets/internal/swipe.h); past either end
// `sound.at-end` sounds and nothing is drawn. Back and Next turn by the buttons; Skip, and Done on the last card, call `leave`. Holding
// OK calls `leave` too, at any depth — the root included — through the screen's own escape (router::AttachEscape), so MakeCarousel
// is called from the screen's OnCreate.

#pragma once

#include "lvgl.h"

namespace widgets {

struct CarouselCard {
    const char *title;  // up to about two lines of `display`
    const char *body;   // words in `body`, 21 characters a line
};

// What Skip and Done do: leave the screen. Runs under the lock, from the button's click.
using CarouselLeave = void (*)(void *user);

struct Carousel {
    lv_obj_t *card;   // the card, filling its column; sends nothing
    lv_obj_t *back;   // hidden on the first card
    lv_obj_t *skip;
    lv_obj_t *next;   // "Done" on the last card
};

// Builds the card and its indicator in `content` (the screen's content column) and the footer on `screen`, after `content`. `cards`
// are kept by pointer, so they outlive the screen; `count` is at least two and at most `kPageDotsMax`. Focus starts on Next, with no ring
// (the first press shows it). The footer's buttons join the screen's group in the order Back, Skip, Next.
Carousel MakeCarousel(lv_obj_t *screen, lv_obj_t *content, const CarouselCard *cards, int count, CarouselLeave leave, void *user);
// The card showing, 0-based.
int CarouselIndex(const Carousel &carousel);
// Shows card `index`, as a swipe or a button does: one redraw; focus stays on its stop, handed to Next when Back goes. No sound. Call it
// once the screen is on the glass: the page dots' widths are laid out with the first frame, and a state changed before it is not.
void ShowCarouselCard(const Carousel &carousel, int index);

}  // namespace widgets
