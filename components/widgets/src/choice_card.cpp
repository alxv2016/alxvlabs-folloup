#include "widgets/choice_card.h"

#include "design_tokens.h"
#include "internal/base.h"
#include "internal/marks.h"

namespace widgets {
namespace dt = design_tokens;

namespace detail {
namespace {
// A choice says what it is by its own listener: the one `OnClicked` is added with a tag as its user data — one-of-a-set or any
// number — so the object's own user data stays the caller's. A group is the choices of the one-of-a-set tag that share a parent.
int s_one_tag;
int s_many_tag;
void OnClicked(lv_event_t *e);

bool IsOne(lv_obj_t *obj)
{
    const uint32_t n = lv_obj_get_event_count(obj);
    for (uint32_t i = 0; i < n; ++i) {
        lv_event_dsc_t *dsc = lv_obj_get_event_dsc(obj, i);
        if (lv_event_dsc_get_cb(dsc) == OnClicked) {
            return lv_event_dsc_get_user_data(dsc) == &s_one_tag;
        }
    }
    return false;
}

void ClearSiblings(lv_obj_t *chosen)
{
    lv_obj_t *group = lv_obj_get_parent(chosen);
    const uint32_t n = lv_obj_get_child_count(group);
    for (uint32_t i = 0; i < n; ++i) {
        lv_obj_t *other = lv_obj_get_child(group, static_cast<int32_t>(i));
        if (other != chosen && IsOne(other)) {
            lv_obj_remove_state(other, LV_STATE_CHECKED);
        }
    }
}

// OK on it, or a tap on its lift: it is chosen, its group's others are cleared, and then the change is sent on it. Choosing
// the chosen one sends nothing (a radio never unchooses itself); a card of any number flips and says so each time. The focus
// never chooses: nothing here listens for it.
void OnClicked(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_current_target_obj(e);
    if (lv_obj_has_state(obj, LV_STATE_DISABLED)) {
        return;
    }
    if (lv_event_get_user_data(e) == &s_one_tag) {
        if (lv_obj_has_state(obj, LV_STATE_CHECKED)) {
            return;
        }
        ClearSiblings(obj);
        lv_obj_add_state(obj, LV_STATE_CHECKED);
    } else {
        lv_obj_set_state(obj, LV_STATE_CHECKED, !lv_obj_has_state(obj, LV_STATE_CHECKED));
    }
    lv_obj_send_event(obj, LV_EVENT_VALUE_CHANGED, nullptr);
}
}  // namespace

void MakeChoice(lv_obj_t *obj, bool many)
{
    // First, so a caller's own listener sees the group settled; the object's user data is left to the caller.
    lv_obj_add_event_cb(obj, OnClicked, LV_EVENT_CLICKED, many ? static_cast<void *>(&s_many_tag) : static_cast<void *>(&s_one_tag));
}

void SetChoice(lv_obj_t *obj, bool chosen)
{
    if (chosen && IsOne(obj)) {
        ClearSiblings(obj);
    }
    lv_obj_set_state(obj, LV_STATE_CHECKED, chosen);
}

lv_obj_t *MakeMark(lv_obj_t *parent, bool many)
{
    constexpr lv_style_selector_t kOnDisabled = static_cast<lv_style_selector_t>(LV_STATE_DISABLED) | LV_STATE_CHECKED;
    lv_obj_t *mark = Bare(parent);
    lv_obj_set_size(mark, dt::kSpacingMark, dt::kSpacingMark);
    lv_obj_set_style_radius(mark, many ? dt::kRoundedMd : LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(mark, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(mark, ui::Level(dt::kColorBackground), 0);
    Outline(mark, dt::kSpacingStrokeStrong);
    // Disabled: no edge of any kind, the white disc or box kept as a knocked-out part, its check or dot `text-disabled-page` on it.
    lv_obj_set_style_border_opa(mark, LV_OPA_TRANSP, LV_STATE_DISABLED);
    if (many) {
        // A checkbox's check: the deck's art at one bit, shown while the card is chosen.
        lv_obj_t *check = lv_image_create(mark);
        lv_image_set_src(check, &widgets_mark_check);
        lv_obj_set_style_image_recolor(check, ui::Level(dt::kColorSurfaceAction), 0);
        lv_obj_set_style_image_recolor_opa(check, LV_OPA_COVER, 0);
        lv_obj_set_style_image_recolor(check, DisabledOnPage(), kOnDisabled);
        lv_obj_set_style_image_opa(check, LV_OPA_TRANSP, 0);
        lv_obj_set_style_image_opa(check, LV_OPA_COVER, LV_STATE_CHECKED);
        lv_obj_remove_flag(check, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_center(check);
    } else {
        lv_obj_t *dot = Bare(mark);
        lv_obj_set_size(dot, dt::kSpacingMarkDot, dt::kSpacingMarkDot);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot, ui::Level(dt::kColorSurfaceAction), 0);
        lv_obj_set_style_bg_color(dot, DisabledOnPage(), kOnDisabled);
        lv_obj_set_style_bg_opa(dot, LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, LV_STATE_CHECKED);
        lv_obj_center(dot);
    }
    lv_obj_add_flag(mark, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_flag(mark, LV_OBJ_FLAG_STATE_TRICKLE);    // passes the chosen state it gets on to its dot or check
    lv_obj_add_flag(parent, LV_OBJ_FLAG_STATE_TRICKLE);  // the row's or card's chosen state reaches the mark
    return mark;
}
}  // namespace detail

lv_obj_t *MakeChoiceCard(lv_obj_t *parent, const char *text, bool many, const char *supporting)
{
    lv_obj_t *card = lv_button_create(parent);
    lv_obj_remove_style_all(card);  // touch-exception: no pressed state (DESIGN → Motion → Press)
    lv_obj_set_scrollable(card, false);
    lv_obj_set_width(card, lv_pct(100));
    lv_obj_set_height(card, LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(card, dt::kSpacingMark + 2 * dt::kSpacingMd, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(card, dt::kSpacingCardInset, 0);
    lv_obj_set_style_pad_ver(card, dt::kSpacingMd, 0);
    lv_obj_set_style_pad_column(card, dt::kSpacingMd, 0);
    lv_obj_set_style_radius(card, dt::kRoundedLg, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(card, ui::Fill(dt::kColorBand), 0);  // the deck's card is a filled surface; black words
    lv_obj_set_style_text_color(card, ui::Level(dt::kColorTextPrimary), 0);
    detail::DitherWhenDisabled(card);  // disabled: the light dither it has, no edge, its words `text-disabled`
    lv_obj_t *words = detail::Bare(card);
    lv_obj_set_flex_flow(words, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_grow(words, 1);
    lv_obj_set_size(words, 1, LV_SIZE_CONTENT);  // the grow takes the room the mark leaves; the words wrap inside it
    if (supporting != nullptr && supporting[0] != '\0') {
        detail::WrapWords(words, text, ui::Type::kCaptionStrong);
        detail::WrapWords(words, supporting, ui::Type::kCaption);
    } else {
        detail::WrapWords(words, text, ui::Type::kCaption);
    }
    detail::MakeMark(card, many);
    detail::MakeChoice(card, many);
    detail::Focusable(card);
    return card;
}

void SetChoiceChosen(lv_obj_t *card, bool chosen)
{
    detail::SetChoice(card, chosen);
}

bool ChoiceChosen(lv_obj_t *card)
{
    return lv_obj_has_state(card, LV_STATE_CHECKED);
}

}  // namespace widgets
