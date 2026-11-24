#include "widgets/status_block.h"

#include "design_tokens.h"
#include "internal/base.h"
#include "internal/glue.h"
#include "internal/marks.h"

namespace widgets {
namespace dt = design_tokens;

lv_obj_t *MakeStatusBlock(lv_obj_t *parent, const char *heading, const char *description, StatusMark mark)
{
    const lv_image_dsc_t *art = mark == StatusMark::kCheck     ? &widgets_mark_status_check
                                : mark == StatusMark::kClock   ? &widgets_mark_status_clock
                                : mark == StatusMark::kLinkOff ? &widgets_mark_status_link_off
                                                               : nullptr;
    lv_obj_t *block = detail::Bare(parent);  // the mark's slot is `status-mark-slot`: one x for the words
    lv_obj_set_size(block, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(block, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(block, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(block, dt::kSpacingLg, 0);
    if (art == nullptr) {
        lv_obj_t *empty = detail::Bare(block);  // the slot stays empty: the words stand where a mark would put them
        lv_obj_set_size(empty, dt::kSpacingStatusMarkSlot, 1);
    } else {
        lv_obj_t *image = lv_image_create(block);
        lv_image_set_src(image, art);
        lv_obj_set_width(image, dt::kSpacingStatusMarkSlot);  // the art flush left in its slot
        lv_image_set_inner_align(image, LV_IMAGE_ALIGN_TOP_LEFT);
        lv_obj_set_style_image_recolor(image, ui::Level(dt::kColorTextPrimary), 0);
        lv_obj_set_style_image_recolor_opa(image, LV_OPA_COVER, 0);
    }
    lv_obj_t *words = detail::Bare(block);
    lv_obj_set_flex_flow(words, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_grow(words, 1);
    lv_obj_set_size(words, 1, LV_SIZE_CONTENT);  // the grow takes the room the mark leaves; the words wrap inside it
    lv_obj_set_style_pad_row(words, dt::kSpacingPairMd, 0);  // a title over a wrapped description, between their own boxes
    lv_obj_t *title = detail::Words(words, ui::Type::kLabel, heading);
    lv_obj_t *lines = detail::Words(words, ui::Type::kBody, description, true);
    detail::Glue(title, lines);
    return block;
}

}  // namespace widgets
