#include "widgets/status_block.h"

#include "design_tokens.h"
#include "internal/base.h"
#include "internal/marks.h"

namespace widgets {
namespace dt = design_tokens;

lv_obj_t *MakeStatusBlock(lv_obj_t *parent, const char *heading, const char *description, StatusMark mark)
{
    const lv_image_dsc_t *art = mark == StatusMark::kCheck     ? &widgets_mark_status_check
                                : mark == StatusMark::kClock   ? &widgets_mark_status_clock
                                : mark == StatusMark::kLinkOff ? &widgets_mark_status_link_off
                                                               : nullptr;
    lv_obj_t *block = detail::Bare(parent);
    lv_obj_set_size(block, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(block, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(block, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(block, dt::kSpacingLg, 0);
    if (art == nullptr) {
        lv_obj_set_style_pad_left(block, dt::kSpacingLg, 0);  // the slot stays empty: the words stand one gap in, where a mark would put them
    } else {
        lv_obj_t *image = lv_image_create(block);
        lv_image_set_src(image, art);
        lv_obj_set_style_image_recolor(image, ui::Level(dt::kColorTextPrimary), 0);
        lv_obj_set_style_image_recolor_opa(image, LV_OPA_COVER, 0);
    }
    lv_obj_t *words = detail::Bare(block);
    lv_obj_set_flex_flow(words, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_grow(words, 1);
    lv_obj_set_size(words, 1, LV_SIZE_CONTENT);  // the grow takes the room the mark leaves; the words wrap inside it
    lv_obj_set_style_pad_row(words, dt::kSpacingSm, 0);
    detail::Words(words, ui::Type::kLabel, heading);
    detail::Words(words, ui::Type::kCaption, description, true);
    return block;
}

}  // namespace widgets
