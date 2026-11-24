#include "widgets/list_row.h"

#include "design_tokens.h"
#include "internal/base.h"
#include "lvgl_display.h"

namespace widgets {
namespace dt = design_tokens;

namespace {
lv_style_t s_row, s_chosen;
bool s_styles_built = false;

// The row's two styles, built from ui's levels and fonts on the first row, after ui::Start().
void BuildStyles()
{
    if (s_styles_built) {
        return;
    }
    s_styles_built = true;
    // list-row: 80 px, spacing.sm above and below, spacing.md either side, rounded.lg, no rule below — the ring and the
    // 16 px between rows separate them (STICKY_DESIGN_TOKENS §4).
    lv_style_init(&s_row);
    lv_style_set_height(&s_row, dt::kSpacingListRowHeight);
    lv_style_set_width(&s_row, lv_pct(100));
    lv_style_set_pad_ver(&s_row, dt::kSpacingSm);
    lv_style_set_pad_hor(&s_row, dt::kSpacingMd);
    lv_style_set_bg_color(&s_row, ui::Level(dt::kColorBackground));
    lv_style_set_bg_opa(&s_row, LV_OPA_COVER);
    lv_style_set_text_color(&s_row, ui::Level(dt::kColorTextPrimary));
    lv_style_set_text_font(&s_row, ui::Font(ui::Type::kLabel));
    lv_style_set_border_width(&s_row, 0);
    lv_style_set_radius(&s_row, dt::kRoundedLg);

    lv_style_init(&s_chosen);
    lv_style_set_bg_color(&s_chosen, ui::Level(dt::kColorSurfaceChosen));
    lv_style_set_text_color(&s_chosen, ui::Level(dt::kColorTextOnFill));
}
}  // namespace

lv_obj_t *MakeListRow(lv_obj_t *parent, const char *label, const char *supporting, lv_event_cb_t on_click, void *user_data)
{
    BuildStyles();
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_add_style(row, &s_row, 0);
    lv_obj_add_style(row, &s_chosen, LV_STATE_CHECKED);
    lv_obj_set_scrollable(row, false);  // a component never scrolls; a page that must, says so itself
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(row, 0, 0);
    lv_obj_t *title = lv_label_create(row);
    lv_label_set_text(title, label);
    lv_label_set_long_mode(title, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_width(title, lv_pct(100));
    // One line on its line box: the rows keep their pitch and the detail its distance (STICKY_DESIGN_TOKENS §9).
    ui::FitLineBox(title, ui::Type::kLabel);
    if (supporting != nullptr) {
        lv_obj_t *sub = lv_label_create(row);
        lv_label_set_text(sub, supporting);
        lv_obj_set_style_text_font(sub, ui::Font(ui::Type::kCaption), 0);
        lv_obj_set_style_text_color(sub, ui::Level(dt::kColorTextSecondary), 0);
        lv_obj_set_style_text_color(sub, ui::Level(dt::kColorTextOnFill), LV_STATE_CHECKED);
        lv_label_set_long_mode(sub, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_set_width(sub, lv_pct(100));
        // One line on its line box, ending in LVGL's dots when too long (STICKY_DESIGN_TOKENS §9).
        ui::FitLineBox(sub, ui::Type::kCaption);
    }
    ui::RingOutside(row);
    if (on_click != nullptr) {
        ui::ShowWhenFocused(row);
        lv_obj_set_clickable(row, true);
        lv_obj_add_event_cb(row, on_click, LV_EVENT_CLICKED, user_data);
        lv_group_add_obj(lvgl_display::Group(), row);
    }
    return row;
}

void SetListRowChosen(lv_obj_t *row, bool chosen)
{
    lv_obj_set_state(row, LV_STATE_CHECKED, chosen);
}

}  // namespace widgets
