#include "widgets/choice_card.h"

#include "design_tokens.h"
#include "internal/base.h"
#include "internal/marks.h"

namespace widgets {
namespace dt = design_tokens;

namespace detail {
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
    detail::Focusable(card);
    return card;
}

void SetChoiceChosen(lv_obj_t *card, bool chosen)
{
    lv_obj_set_state(card, LV_STATE_CHECKED, chosen);
}

bool ChoiceChosen(lv_obj_t *card)
{
    return lv_obj_has_state(card, LV_STATE_CHECKED);
}

}  // namespace widgets
