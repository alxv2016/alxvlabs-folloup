#include "widgets/radio_row.h"

#include "design_tokens.h"
#include "internal/base.h"
#include "widgets/choice_card.h"

namespace widgets {
namespace dt = design_tokens;

lv_obj_t *MakeRadioRow(lv_obj_t *parent, const char *text)
{
    lv_obj_t *row = lv_button_create(parent);
    lv_obj_remove_style_all(row);  // touch-exception: no pressed state (DESIGN → Motion → Press)
    lv_obj_set_scrollable(row, false);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);  // `spacing.list-row-height` is a floor: the row grows with its words
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(row, dt::kSpacingCardInset, 0);
    lv_obj_set_style_pad_ver(row, dt::kSpacingXs, 0);  // one line and two both fit the 80: 4 + 2 x 36 + 4
    lv_obj_set_style_pad_column(row, dt::kSpacingMd, 0);
    lv_obj_set_style_radius(row, dt::kRoundedLg, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(row, ui::Fill(dt::kColorBand), 0);  // a filled card, black words on it
    lv_obj_set_style_text_font(row, ui::Font(ui::Type::kLabel), 0);
    lv_obj_set_style_text_color(row, ui::Level(dt::kColorTextPrimary), 0);
    detail::DitherWhenDisabled(row);  // disabled: the light dither it has, no edge, its words `text-disabled`
    // The words are a column that takes the room the mark leaves, wraps inside it and is as tall as the row less its padding,
    // centring its lines: LVGL's flex track is its tallest child, so a min height on the row alone would pad below the words.
    lv_obj_t *words = detail::Bare(row);
    lv_obj_set_flex_flow(words, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_grow(words, 1);
    lv_obj_set_size(words, 1, LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(words, dt::kSpacingListRowHeight - 2 * dt::kSpacingXs, 0);
    lv_obj_set_flex_align(words, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    detail::LetRingOut(words);  // the 1 px a glyph hangs left of its cell is outside the column, and a child is clipped to its parent
    detail::WrapWords(words, text, ui::Type::kLabel);
    detail::MakeMark(row, false);
    detail::Focusable(row);
    return row;
}

void SetRadioChosen(lv_obj_t *row, bool chosen)
{
    lv_obj_set_state(row, LV_STATE_CHECKED, chosen);
}

}  // namespace widgets
