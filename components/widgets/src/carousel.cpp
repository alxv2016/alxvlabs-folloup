#include "widgets/carousel.h"

#include <algorithm>
#include <cstdio>

#include "design_tokens.h"
#include "internal/base.h"
#include "lvgl_display.h"
#include "router/router.h"
#include "widgets/button.h"
#include "widgets/page_dots.h"

namespace widgets {
namespace dt = design_tokens;

namespace {
constexpr int32_t kSlot = (dt::kSpacingContent - 2 * dt::kSpacingMd) / 3;  // a footer slot: 133 px, three and two gaps fill the content

struct State {
    const CarouselCard *cards;
    int count;
    int index;
    CarouselLeave leave;
    void *user;
    lv_obj_t *title, *body, *words, *dots;
    lv_obj_t *back, *skip, *next, *next_label;
};

State *Of(lv_obj_t *card)
{
    return static_cast<State *>(lv_obj_get_user_data(card));
}

// Draws card `index`: its words, "n of N", the dot, Back present or not, Next or Done. The ring, if one shows, stays on its stop;
// Back going away hands it to Next.
void Show(lv_obj_t *card, int index)
{
    State *s = Of(card);
    index = std::clamp(index, 0, s->count - 1);
    s->index = index;
    lv_label_set_text(s->title, s->cards[index].title);
    lv_label_set_text(s->body, s->cards[index].body);
    char words[24];
    std::snprintf(words, sizeof words, "%d of %d", index + 1, s->count);
    lv_label_set_text(s->words, words);
    SetPageDot(s->dots, index);
    lv_label_set_text(s->next_label, index == s->count - 1 ? "Done" : "Next");
    router::SetHiddenHandingFocus(s->back, index == 0, s->next);
}

// A swipe turns one card, `dir` +1 for a swipe left; past an end the at-end sound says there is no card, and nothing is drawn.
void OnSwipe(lv_obj_t *card, int32_t dir)
{
    State *s = Of(card);
    const int index = s->index + (dir > 0 ? 1 : -1);
    if (index < 0 || index >= s->count) {
        lvgl_display::PlayCue(lvgl_display::Cue::kAtEnd);
        return;
    }
    Show(card, index);
}

void OnBack(lv_event_t *e)
{
    lv_obj_t *card = static_cast<lv_obj_t *>(lv_event_get_user_data(e));
    Show(card, Of(card)->index - 1);
}

void OnNext(lv_event_t *e)
{
    lv_obj_t *card = static_cast<lv_obj_t *>(lv_event_get_user_data(e));
    State *s = Of(card);
    if (s->index == s->count - 1) {
        if (s->leave != nullptr) {
            s->leave(s->user);
        }
        return;
    }
    Show(card, s->index + 1);
}

void OnSkip(lv_event_t *e)
{
    State *s = Of(static_cast<lv_obj_t *>(lv_event_get_user_data(e)));
    if (s->leave != nullptr) {
        s->leave(s->user);
    }
}

// Holding OK leaves, as Skip does, at any depth: the router asks it when nothing is open and no overlay is up (router::Escape).
void OnEscape(void *user)
{
    State *s = static_cast<State *>(user);
    if (s->leave != nullptr) {
        s->leave(s->user);
    }
}

void OnDelete(lv_event_t *e)
{
    delete Of(lv_event_get_target_obj(e));
}
}  // namespace

Carousel MakeCarousel(lv_obj_t *screen, lv_obj_t *content, const CarouselCard *cards, int count, CarouselLeave leave, void *user)
{
    Carousel carousel{};
    State *s = new State{};
    s->cards = cards;
    s->count = std::clamp(count, 2, kPageDotsMax);
    s->leave = leave;
    s->user = user;

    // The card: the column's width and the rest of its height; white on a `spacing.stroke.default` outline, `rounded.lg`, its words
    // `spacing.card-inset` in from the card's edge. It is no focus stop and takes no tap, only the swipe.
    lv_obj_t *card = detail::Bare(content);
    carousel.card = card;
    lv_obj_set_width(card, lv_pct(100));
    lv_obj_set_flex_grow(card, 1);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_radius(card, dt::kRoundedLg, 0);
    lv_obj_set_style_pad_all(card, dt::kSpacingCardInset - dt::kSpacingStrokeDefault, 0);
    lv_obj_set_style_pad_row(card, dt::kSpacingMd, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(card, ui::Level(dt::kColorBackground), 0);
    detail::Outline(card, dt::kSpacingStrokeDefault);
    s->title = ui::NewLabel(card, ui::Type::kDisplay, "");
    s->body = ui::NewLabel(card, ui::Type::kBody, "");
    // Room to scroll: the swipe claims a drag only on an object that has somewhere to go, and puts it back at once, so nothing moves.
    lv_obj_t *room = detail::Bare(card);
    lv_obj_add_flag(room, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(room, 2 * dt::kSpacingContent, 1);
    lv_obj_set_pos(room, 0, 0);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLL_WITH_ARROW);  // no key ever scrolls the card: its turns are the swipe's and the buttons'
    lv_obj_set_user_data(card, s);
    lv_obj_add_event_cb(card, OnDelete, LV_EVENT_DELETE, nullptr);
    detail::ClaimSwipe(card, OnSwipe, detail::SwipeAxis::kHorizontal);
    router::AttachEscape(OnEscape, s);  // holding OK leaves; heard only while this screen is being built

    // "2 of 4" in `caption` `text-secondary`, the dots `spacing.md` after it; no hit area.
    lv_obj_t *indicator = detail::Bare(content);
    lv_obj_set_size(indicator, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(indicator, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(indicator, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(indicator, dt::kSpacingMd, 0);
    s->words = ui::NewLabel(indicator, ui::Type::kCaption, "", true);
    lv_obj_set_width(s->words, LV_SIZE_CONTENT);
    ui::FitLineBox(s->words, ui::Type::kCaption);
    s->dots = MakePageDots(indicator, s->count, 0);

    // The footer: three fixed slots, so Skip and Next never move from card to card.
    lv_obj_t *footer = detail::Bare(screen);
    lv_obj_set_size(footer, lv_pct(100), dt::kSpacingFooter);
    lv_obj_set_style_pad_hor(footer, dt::kSpacingEdge, 0);
    lv_obj_set_style_pad_column(footer, dt::kSpacingMd, 0);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    detail::LetRingOut(footer);
    lv_obj_t *slot = detail::Bare(footer);
    lv_obj_set_size(slot, kSlot, dt::kSpacingTarget);
    detail::LetRingOut(slot);
    s->back = MakeButton(slot, "Back", ButtonVariant::kOutline, kSlot);
    s->skip = MakeButton(footer, "Skip", ButtonVariant::kOutline, kSlot);
    s->next = MakeButton(footer, "Next", ButtonVariant::kAction, kSlot);
    s->next_label = lv_obj_get_child(s->next, 0);
    lv_obj_add_event_cb(s->back, OnBack, LV_EVENT_CLICKED, card);
    lv_obj_add_event_cb(s->skip, OnSkip, LV_EVENT_CLICKED, card);
    lv_obj_add_event_cb(s->next, OnNext, LV_EVENT_CLICKED, card);
    carousel.back = s->back;
    carousel.skip = s->skip;
    carousel.next = s->next;

    Show(card, 0);
    lv_group_focus_obj(s->next);  // opens on Next, with no ring: the first press shows it
    return carousel;
}

int CarouselIndex(const Carousel &carousel)
{
    return Of(carousel.card)->index;
}

void ShowCarouselCard(const Carousel &carousel, int index)
{
    Show(carousel.card, index);
}

}  // namespace widgets
