#include "internal/setting_row.h"

#include "design_tokens.h"
#include "internal/base.h"

namespace widgets::detail {
namespace dt = design_tokens;

SettingRow MakeSettingRow(lv_obj_t *parent, const char *name, bool with_state)
{
    SettingRow row{};
    row.root = Bare(parent);
    lv_obj_set_size(row.root, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(row.root, dt::kSpacingTarget, 0);
    lv_obj_set_flex_flow(row.root, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row.root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row.root, dt::kSpacingLg, 0);  // a name keeps its ring's reach from the control
    lv_obj_add_flag(row.root, LV_OBJ_FLAG_CLICKABLE);
    LetRingOut(row.root);

    // The name, and the reason under it while disabled: one column that takes the room the state and control leave.
    row.words = Bare(row.root);
    lv_obj_set_flex_flow(row.words, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_grow(row.words, 1);
    lv_obj_set_size(row.words, 1, LV_SIZE_CONTENT);  // the grow takes the room left; the name wraps inside it
    // The flex track is as tall as its tallest child and the row's min_height only pads below it, so the words column is
    // the one that stands spacing.target tall and centres its lines: the name, the state word and the control then sit
    // centred in the 64 px box, with the reason showing or not.
    lv_obj_set_style_min_height(row.words, dt::kSpacingTarget, 0);
    lv_obj_set_flex_align(row.words, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    row.label = Words(row.words, ui::Type::kLabel, name);
    row.reason = Words(row.words, ui::Type::kCaption, "", true);
    lv_obj_add_flag(row.reason, LV_OBJ_FLAG_HIDDEN);
    if (with_state) {
        row.state = Line(row.root, ui::Type::kBody, "", true);
    }
    return row;
}

void ShowSettingRowEnabled(const SettingRow &row, bool enabled, const char *reason)
{
    // A label's own colour outranks a parent's disabled style, so the row sets each word's colour itself.
    lv_obj_set_style_text_color(row.label, enabled ? ui::Level(dt::kColorTextPrimary) : DisabledOnPage(), 0);
    if (row.state != nullptr) {
        lv_obj_set_style_text_color(row.state, enabled ? ui::Level(dt::kColorTextSecondary) : DisabledOnPage(), 0);
    }
    if (!enabled && reason != nullptr) {
        lv_label_set_text(row.reason, reason);
        lv_obj_remove_flag(row.reason, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_pad_row(row.words, dt::kSpacingXs, 0);  // the name and its reason, only while the reason shows
    } else {
        lv_obj_add_flag(row.reason, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_pad_row(row.words, 0, 0);
    }
}

}  // namespace widgets::detail
