// DESIGN.md's components in LVGL: Bitcount Single at each type role, compiled at one bit a pixel, and one style per
// component, from the token header. No theme is compiled in, so nothing carries a pressed tint, a transition or a
// shadow the glass cannot show; focus is the ring, chosen is the inversion (DESIGN → Focus and input).

#include <cstddef>

#include "design_tokens.h"
#include "lvgl_display.h"
#include "ui.h"
#include "esp_check.h"
#include "esp_log.h"
#include "lvgl.h"

// Bitcount Single at one bit a pixel (fonts/bitcount/make_fonts.sh): C objects, declared outside any namespace.
extern "C" {
LV_FONT_DECLARE(bitcount_600_20) LV_FONT_DECLARE(bitcount_600_30) LV_FONT_DECLARE(bitcount_600_40)
LV_FONT_DECLARE(bitcount_600_50) LV_FONT_DECLARE(bitcount_600_60) LV_FONT_DECLARE(bitcount_600_70)
LV_FONT_DECLARE(bitcount_600_80) LV_FONT_DECLARE(bitcount_600_100) LV_FONT_DECLARE(bitcount_600_120)
LV_FONT_DECLARE(bitcount_600_160)
LV_FONT_DECLARE(bitcount_700_20) LV_FONT_DECLARE(bitcount_700_30) LV_FONT_DECLARE(bitcount_700_40)
LV_FONT_DECLARE(bitcount_700_50) LV_FONT_DECLARE(bitcount_700_60) LV_FONT_DECLARE(bitcount_700_70)
LV_FONT_DECLARE(bitcount_700_80) LV_FONT_DECLARE(bitcount_700_100) LV_FONT_DECLARE(bitcount_700_120)
LV_FONT_DECLARE(bitcount_700_160)
}

namespace ui {
namespace {

namespace dt = design_tokens;
constexpr const char *kTag = "ui";

struct Role {
    dt::TypeRole tokens;
    const lv_font_t *font;
};
Role s_roles[] = {
    {dt::kTypographyHero, nullptr},  {dt::kTypographyDisplay, nullptr}, {dt::kTypographyHeadline, nullptr},
    {dt::kTypographyBody, nullptr},  {dt::kTypographyLabel, nullptr},   {dt::kTypographyCaption, nullptr},
};

lv_style_t s_screen, s_status_bar, s_row, s_button_outline, s_button_action, s_divider, s_focus, s_focus_on_fill,
    s_chosen;

struct BitcountFont {
    int16_t size_px;
    int16_t weight;
    const lv_font_t *font;
};
constexpr BitcountFont kBitcount[] = {
    {20, 600, &bitcount_600_20},   {30, 600, &bitcount_600_30},   {40, 600, &bitcount_600_40},
    {50, 600, &bitcount_600_50},   {60, 600, &bitcount_600_60},   {70, 600, &bitcount_600_70},
    {80, 600, &bitcount_600_80},   {100, 600, &bitcount_600_100}, {120, 600, &bitcount_600_120},
    {160, 600, &bitcount_600_160}, {20, 700, &bitcount_700_20},   {30, 700, &bitcount_700_30},
    {40, 700, &bitcount_700_40},   {50, 700, &bitcount_700_50},   {60, 700, &bitcount_700_60},
    {70, 700, &bitcount_700_70},   {80, 700, &bitcount_700_80},   {100, 700, &bitcount_700_100},
    {120, 700, &bitcount_700_120}, {160, 700, &bitcount_700_160},
};

// The space LVGL adds between a role's lines, so a block of text steps by the role's line box: the line height token
// less the font's own height (STICKY_DESIGN_TOKENS §3).
int32_t LineSpace(Type role)
{
    const int32_t space = LineBox(role) - lv_font_get_line_height(Font(role));
    return space > 0 ? space : 0;
}

void NoScroll(lv_obj_t *obj)
{
    lv_obj_set_scrollable(obj, false);  // a component never scrolls; a page that must, says so itself
}

void BuildStyles()
{
    lv_style_init(&s_screen);
    lv_style_set_bg_color(&s_screen, Level(dt::kColorBackground));
    lv_style_set_bg_opa(&s_screen, LV_OPA_COVER);
    lv_style_set_text_color(&s_screen, Level(dt::kColorTextPrimary));
    lv_style_set_text_font(&s_screen, Font(Type::kBody));
    lv_style_set_text_line_space(&s_screen, LineSpace(Type::kBody));
    lv_style_set_pad_all(&s_screen, 0);
    lv_style_set_border_width(&s_screen, 0);

    lv_style_init(&s_status_bar);
    lv_style_set_height(&s_status_bar, dt::kSpacingStatusBar);
    lv_style_set_width(&s_status_bar, lv_pct(100));
    lv_style_set_pad_hor(&s_status_bar, dt::kSpacingEdge);
    lv_style_set_bg_color(&s_status_bar, Level(dt::kColorBackground));
    lv_style_set_bg_opa(&s_status_bar, LV_OPA_COVER);
    lv_style_set_text_font(&s_status_bar, Font(Type::kCaption));
    lv_style_set_border_side(&s_status_bar, LV_BORDER_SIDE_BOTTOM);
    lv_style_set_border_width(&s_status_bar, dt::kSpacingStrokeHairline);
    lv_style_set_border_color(&s_status_bar, Level(dt::kColorDivider));

    // list-row: 80 px, spacing.sm above and below, spacing.md either side, a divider below (STICKY_DESIGN_TOKENS §4).
    lv_style_init(&s_row);
    lv_style_set_height(&s_row, dt::kSpacingListRowHeight);
    lv_style_set_width(&s_row, lv_pct(100));
    lv_style_set_pad_ver(&s_row, dt::kSpacingSm);
    lv_style_set_pad_hor(&s_row, dt::kSpacingMd);
    lv_style_set_bg_color(&s_row, Level(dt::kColorBackground));
    lv_style_set_bg_opa(&s_row, LV_OPA_COVER);
    lv_style_set_text_color(&s_row, Level(dt::kColorTextPrimary));
    lv_style_set_text_font(&s_row, Font(Type::kLabel));
    lv_style_set_border_side(&s_row, LV_BORDER_SIDE_BOTTOM);
    lv_style_set_border_width(&s_row, dt::kSpacingStrokeHairline);
    lv_style_set_border_color(&s_row, Level(dt::kColorDivider));
    lv_style_set_radius(&s_row, dt::kRoundedNone);

    // button-outline and button-action: 64 px, rounded.md, spacing.md inside.
    lv_style_init(&s_button_outline);
    lv_style_set_height(&s_button_outline, dt::kSpacingTarget);
    lv_style_set_pad_hor(&s_button_outline, dt::kSpacingMd);
    lv_style_set_radius(&s_button_outline, dt::kRoundedMd);
    lv_style_set_bg_color(&s_button_outline, Level(dt::kColorBackground));
    lv_style_set_bg_opa(&s_button_outline, LV_OPA_COVER);
    lv_style_set_border_color(&s_button_outline, Level(dt::kColorOutline));
    lv_style_set_border_width(&s_button_outline, dt::kSpacingStrokeDefault);
    lv_style_set_text_color(&s_button_outline, Level(dt::kColorTextPrimary));
    lv_style_set_text_font(&s_button_outline, Font(Type::kLabel));

    lv_style_init(&s_button_action);
    lv_style_set_height(&s_button_action, dt::kSpacingTarget);
    lv_style_set_pad_hor(&s_button_action, dt::kSpacingMd);
    lv_style_set_radius(&s_button_action, dt::kRoundedMd);
    lv_style_set_bg_color(&s_button_action, Level(dt::kColorSurfaceAction));
    lv_style_set_bg_opa(&s_button_action, LV_OPA_COVER);
    lv_style_set_border_width(&s_button_action, 0);
    lv_style_set_text_color(&s_button_action, Level(dt::kColorTextOnFill));
    lv_style_set_text_font(&s_button_action, Font(Type::kLabel));

    lv_style_init(&s_divider);
    lv_style_set_height(&s_divider, dt::kSpacingStrokeHairline);
    lv_style_set_width(&s_divider, lv_pct(100));
    lv_style_set_bg_color(&s_divider, Level(dt::kColorDivider));
    lv_style_set_bg_opa(&s_divider, LV_OPA_COVER);
    lv_style_set_border_width(&s_divider, 0);
    lv_style_set_pad_all(&s_divider, 0);

    // The ring: stroke.focus wide, inside the control — on the edge of one with no outline, focus-offset inside the
    // outline of one with it (STICKY_DESIGN_TOKENS §4). LVGL draws an outline outward from outline_pad, so a negative
    // pad of the ring's inner distance puts it inside.
    lv_style_init(&s_focus);
    lv_style_set_outline_width(&s_focus, dt::kSpacingStrokeFocus);
    lv_style_set_outline_color(&s_focus, Level(dt::kColorFocusRing));
    lv_style_set_outline_opa(&s_focus, LV_OPA_COVER);
    lv_style_set_outline_pad(&s_focus, -dt::kSpacingStrokeFocus);

    lv_style_init(&s_focus_on_fill);
    lv_style_set_outline_color(&s_focus_on_fill, Level(dt::kColorFocusRingOnFill));

    lv_style_init(&s_chosen);
    lv_style_set_bg_color(&s_chosen, Level(dt::kColorSurfaceChosen));
    lv_style_set_text_color(&s_chosen, Level(dt::kColorTextOnFill));
}

void AddFocusRing(lv_obj_t *obj, bool on_fill, int16_t inset)
{
    lv_obj_add_style(obj, &s_focus, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(obj, static_cast<int32_t>(-(inset + dt::kSpacingStrokeFocus)), LV_STATE_FOCUS_KEY);
    if (on_fill) {
        lv_obj_add_style(obj, &s_focus_on_fill, LV_STATE_FOCUS_KEY);
    }
}

}  // namespace

esp_err_t Start()
{
    for (Role &role : s_roles) {
        role.font = Bitcount(role.tokens.size_px, role.tokens.weight);
        ESP_RETURN_ON_FALSE(role.font != nullptr, ESP_ERR_NOT_FOUND, kTag, "no Bitcount at %d px, weight %d",
                            role.tokens.size_px, role.tokens.weight);
    }
    BuildStyles();
    return ESP_OK;
}

const lv_font_t *Bitcount(int16_t size_px, int16_t weight)
{
    for (const BitcountFont &f : kBitcount) {
        if (f.size_px == size_px && f.weight == weight) {
            return f.font;
        }
    }
    return nullptr;
}

const lv_font_t *Font(Type role)
{
    const Role &r = s_roles[static_cast<size_t>(role)];
    return r.font != nullptr ? r.font : LV_FONT_DEFAULT;
}

int16_t LineBox(Type role)
{
    return s_roles[static_cast<size_t>(role)].tokens.line_px;
}

lv_color_t Level(uint8_t level)
{
    const uint8_t v = static_cast<uint8_t>(level > 3 ? 0xFF : level * 0x55);
    return lv_color_make(v, v, v);
}

lv_color_t Fill(uint8_t level)
{
    const uint8_t v = lvgl_display::FillCode(level);  // levels 1 and 2 as their dithers (STICKY_DESIGN_TOKENS §4)
    return lv_color_make(v, v, v);
}

lv_obj_t *NewScreen()
{
    lv_obj_t *screen = lv_obj_create(nullptr);
    lv_obj_add_style(screen, &s_screen, 0);
    NoScroll(screen);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    return screen;
}

lv_obj_t *NewStatusBar(lv_obj_t *parent, const char *title)
{
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_add_style(bar, &s_status_bar, 0);
    NoScroll(bar);
    lv_obj_t *label = lv_label_create(bar);
    lv_label_set_text(label, title);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
    return bar;
}

lv_obj_t *NewLabel(lv_obj_t *parent, Type role, const char *text, bool secondary)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, Font(role), 0);
    lv_obj_set_style_text_line_space(label, LineSpace(role), 0);
    lv_obj_set_style_text_color(label, Level(secondary ? dt::kColorTextSecondary : dt::kColorTextPrimary), 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(label, lv_pct(100));
    return label;
}

lv_obj_t *NewRow(lv_obj_t *parent, const char *label, const char *supporting, lv_event_cb_t on_click,
                 void *user_data)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_add_style(row, &s_row, 0);
    lv_obj_add_style(row, &s_chosen, LV_STATE_CHECKED);
    NoScroll(row);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(row, 0, 0);
    lv_obj_t *title = lv_label_create(row);
    lv_label_set_text(title, label);
    lv_label_set_long_mode(title, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_width(title, lv_pct(100));
    if (supporting != nullptr) {
        lv_obj_t *sub = lv_label_create(row);
        lv_label_set_text(sub, supporting);
        lv_obj_set_style_text_font(sub, Font(Type::kCaption), 0);
        lv_obj_set_style_text_color(sub, Level(dt::kColorTextSecondary), 0);
        lv_obj_set_style_text_color(sub, Level(dt::kColorTextOnFill), LV_STATE_CHECKED);
        lv_label_set_long_mode(sub, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_set_width(sub, lv_pct(100));
    }
    AddFocusRing(row, false, 0);
    if (on_click != nullptr) {
        lv_obj_set_clickable(row, true);
        lv_obj_add_event_cb(row, on_click, LV_EVENT_CLICKED, user_data);
        lv_group_add_obj(lvgl_display::Group(), row);
    }
    return row;
}

lv_obj_t *NewButton(lv_obj_t *parent, const char *label, bool action, lv_event_cb_t on_click, void *user_data)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_add_style(button, action ? &s_button_action : &s_button_outline, 0);
    NoScroll(button);
    lv_obj_t *text = lv_label_create(button);
    lv_label_set_text(text, label);
    lv_obj_center(text);
    // An outlined button keeps the ring focus-offset inside its outline; an action button's ring sits on its edge.
    AddFocusRing(button, action, action ? 0 : static_cast<int16_t>(dt::kSpacingStrokeDefault + dt::kSpacingFocusOffset));
    if (on_click != nullptr) {
        lv_obj_add_event_cb(button, on_click, LV_EVENT_CLICKED, user_data);
    }
    lv_group_add_obj(lvgl_display::Group(), button);
    return button;
}

lv_obj_t *NewDivider(lv_obj_t *parent)
{
    lv_obj_t *divider = lv_obj_create(parent);
    lv_obj_add_style(divider, &s_divider, 0);
    NoScroll(divider);
    return divider;
}

void SetChosen(lv_obj_t *control, bool chosen)
{
    if (chosen) {
        lv_obj_add_state(control, LV_STATE_CHECKED);
        lv_obj_add_style(control, &s_focus_on_fill, LV_STATE_FOCUS_KEY | LV_STATE_CHECKED);
    } else {
        lv_obj_remove_state(control, LV_STATE_CHECKED);
    }
}

void RingOnFocus(lv_obj_t *obj)
{
    AddFocusRing(obj, false, 0);
}

}  // namespace ui
