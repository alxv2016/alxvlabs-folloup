#include "widgets/status_line.h"

#include "design_tokens.h"
#include "internal/base.h"
#include "router/router.h"

namespace widgets {
namespace dt = design_tokens;

lv_obj_t *MakeStatusLine(lv_obj_t *parent)
{
    // The zone: a fixed height, the words' column centred on what is left above the rule's own 2 px.
    lv_obj_t *zone = detail::Bare(parent);
    lv_obj_set_size(zone, lv_pct(100), dt::kSpacingListRowHeight);
    lv_obj_set_style_pad_hor(zone, dt::kSpacingEdge, 0);
    lv_obj_set_style_pad_bottom(zone, dt::kSpacingStrokeHairline, 0);
    lv_obj_set_flex_flow(zone, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(zone, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // Words that wrap on each line box of the role (the label is n line boxes tall for n lines), at most two: it ends in LVGL's
    // dots where a third would start (DESIGN → Reading).
    lv_obj_t *words = ui::NewLabel(zone, ui::Type::kCaption, "");
    ui::FitLineBox(words, ui::Type::kCaption);
    lv_label_set_long_mode(words, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_height(words, LV_SIZE_CONTENT);
    lv_obj_set_style_max_height(words, 2 * ui::LineBox(ui::Type::kCaption), 0);
    lv_obj_remove_flag(words, LV_OBJ_FLAG_CLICKABLE);

    // The lower rule is ui::NewDivider's, in the bottom of the zone, taken out of the flow; hidden until there is news.
    lv_obj_t *rule = ui::NewDivider(zone);
    lv_obj_add_flag(rule, LV_OBJ_FLAG_FLOATING);
    lv_obj_remove_flag(rule, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(rule, dt::kSpacingContent, dt::kSpacingStrokeHairline);
    lv_obj_set_pos(rule, 0, dt::kSpacingListRowHeight - dt::kSpacingStrokeHairline);
    lv_obj_add_flag(rule, LV_OBJ_FLAG_HIDDEN);

    router::AttachStatusLine(words, rule);
    return zone;
}

}  // namespace widgets
