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
LV_FONT_DECLARE(bitcount_400_30)
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
    {dt::kTypographyCaptionLoose, nullptr},
};

lv_style_t s_screen, s_status_bar, s_divider, s_focus;

struct BitcountFont {
    int16_t size_px;
    int16_t weight;
    const lv_font_t *font;
};
constexpr BitcountFont kBitcount[] = {
    {30, 400, &bitcount_400_30},
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
    lv_style_set_text_font(&s_status_bar, Font(Type::kLabel));
    lv_style_set_border_side(&s_status_bar, LV_BORDER_SIDE_BOTTOM);
    lv_style_set_border_width(&s_status_bar, dt::kSpacingStrokeHairline);
    lv_style_set_border_color(&s_status_bar, Level(dt::kColorDivider));

    lv_style_init(&s_divider);
    lv_style_set_height(&s_divider, dt::kSpacingStrokeHairline);
    lv_style_set_width(&s_divider, lv_pct(100));
    lv_style_set_bg_color(&s_divider, Level(dt::kColorDivider));
    lv_style_set_bg_opa(&s_divider, LV_OPA_COVER);
    lv_style_set_border_width(&s_divider, 0);
    lv_style_set_pad_all(&s_divider, 0);

    // The ring, one for every control: stroke.focus wide, spacing.focus-offset outside the control's outer edge and
    // following its corners, black on the white glass (STICKY_DESIGN_TOKENS §9).
    lv_style_init(&s_focus);
    lv_style_set_outline_width(&s_focus, dt::kSpacingStrokeFocus);
    lv_style_set_outline_color(&s_focus, Level(dt::kColorFocusRing));
    lv_style_set_outline_opa(&s_focus, LV_OPA_COVER);
    lv_style_set_outline_pad(&s_focus, dt::kSpacingFocusOffset);
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
    StyleScreen(screen);
    return screen;
}

void StyleScreen(lv_obj_t *screen)
{
    lv_obj_add_style(screen, &s_screen, 0);
    NoScroll(screen);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
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

// The role's line height token as the line box (STICKY_DESIGN_TOKENS §3, §9).
void FitLineBox(lv_obj_t *label, Type role)
{
    FitLineBox(label, static_cast<int32_t>(LineBox(role)));
}

lv_obj_t *NewDivider(lv_obj_t *parent)
{
    lv_obj_t *divider = lv_obj_create(parent);
    lv_obj_add_style(divider, &s_divider, 0);
    NoScroll(divider);
    return divider;
}

void RingOutside(lv_obj_t *obj, int32_t inset)
{
    lv_obj_add_style(obj, &s_focus, LV_STATE_FOCUS_KEY);
    if (inset > 0) {
        lv_obj_set_style_outline_pad(obj, dt::kSpacingFocusOffset + inset, LV_STATE_FOCUS_KEY);
    }
}

namespace {
void RingReachCb(lv_event_t *e)
{
    lv_event_set_ext_draw_size(e, dt::kSpacingFocusOffset + dt::kSpacingStrokeEdit);
}
}  // namespace

void LetRingOut(lv_obj_t *obj)
{
    lv_obj_set_overflow_visible(obj, true);
    lv_obj_add_event_cb(obj, RingReachCb, LV_EVENT_REFR_EXT_DRAW_SIZE, nullptr);
    lv_obj_refresh_ext_draw_size(obj);
}

namespace {
// The scale: each elevation level's tokens, by level — one table, so a change to the scale lands in one place.
constexpr dt::ElevationLevel kLevels[] = {dt::kElevation0, dt::kElevation1, dt::kElevation2, dt::kElevation3};

// The offset is the box moved `offset_px` away on a side, drawn before the box, which covers all of it but the exposed edge; the
// keyline is a white rectangle a little larger under it, its radius grown by the same, so a white line rings the offset's outer
// side. Over white it is invisible; over a dither it keeps the offset an edge.
void DrawOffsetCb(lv_event_t *e)
{
    const uintptr_t packed = reinterpret_cast<uintptr_t>(lv_event_get_user_data(e));
    const dt::ElevationLevel &level = kLevels[packed & 0xFF];
    const OffsetSide side = static_cast<OffsetSide>(packed >> 8);
    if (level.shadow == dt::kNoShadow) {
        return;
    }
    lv_obj_t *box = lv_event_get_target_obj(e);
    lv_layer_t *layer = lv_event_get_layer(e);
    lv_area_t area;
    lv_obj_get_coords(box, &area);
    if (side == OffsetSide::kTop) {
        lv_area_move(&area, 0, -level.offset_px);
    } else {
        lv_area_move(&area, level.offset_px, level.offset_px);
    }
    const int32_t radius = lv_obj_get_style_radius(box, LV_PART_MAIN);
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_opa = LV_OPA_COVER;
    // The keyline first: the offset's rectangle grown on every side.
    lv_area_t outer = area;
    outer.x1 -= level.keyline_px;
    outer.y1 -= level.keyline_px;
    outer.x2 += level.keyline_px;
    outer.y2 += level.keyline_px;
    dsc.bg_color = Level(dt::kColorKeyline);
    dsc.radius = radius + level.keyline_px;
    lv_draw_rect(layer, &dsc, &outer);
    dsc.bg_color = Fill(level.shadow);
    dsc.radius = radius;
    lv_draw_rect(layer, &dsc, &area);
}

void OffsetReachCb(lv_event_t *e)
{
    const uintptr_t packed = reinterpret_cast<uintptr_t>(lv_event_get_user_data(e));
    const dt::ElevationLevel &level = kLevels[packed & 0xFF];
    lv_event_set_ext_draw_size(e, level.offset_px + level.keyline_px);
}
}  // namespace

void AddOffset(lv_obj_t *box, Elevation level, OffsetSide side)
{
    void *packed = reinterpret_cast<void *>(static_cast<uintptr_t>(static_cast<uint8_t>(level)) |
                                            (static_cast<uintptr_t>(static_cast<uint8_t>(side)) << 8));
    lv_obj_add_event_cb(box, DrawOffsetCb, LV_EVENT_DRAW_MAIN_BEGIN, packed);
    lv_obj_add_event_cb(box, OffsetReachCb, LV_EVENT_REFR_EXT_DRAW_SIZE, packed);
    lv_obj_refresh_ext_draw_size(box);
}

namespace {
// The page's scroll on focus, at once. LVGL's own (LV_OBJ_FLAG_SCROLL_ON_FOCUS) animates with LV_ANIM_ON, which the glass
// would draw frame by frame (DESIGN → Motion → Nothing animates).
void ShowCb(lv_event_t *e)
{
    lv_obj_t *show = static_cast<lv_obj_t *>(lv_event_get_user_data(e));
    if (show == nullptr) {
        show = lv_event_get_target_obj(e);
    }
    lv_obj_scroll_to_view_recursive(show, LV_ANIM_OFF);
    // The ring reaches past the control, 6 px weight open for editing: the nearest page that scrolls takes the ring in too.
    constexpr int32_t kRingReach = dt::kSpacingFocusOffset + dt::kSpacingStrokeEdit;
    lv_obj_t *page = lv_obj_get_parent(show);
    while (page != nullptr &&
           !(lv_obj_has_flag(page, LV_OBJ_FLAG_SCROLLABLE) && (lv_obj_get_scroll_dir(page) & LV_DIR_VER) != 0)) {
        page = lv_obj_get_parent(page);
    }
    if (page == nullptr) {
        return;
    }
    lv_area_t at, view;
    lv_obj_get_coords(show, &at);
    lv_obj_get_coords(page, &view);
    if (at.y2 + kRingReach > view.y2) {
        lv_obj_scroll_by_bounded(page, 0, view.y2 - (at.y2 + kRingReach), LV_ANIM_OFF);
    } else if (at.y1 - kRingReach < view.y1) {
        lv_obj_scroll_by_bounded(page, 0, view.y1 - (at.y1 - kRingReach), LV_ANIM_OFF);
    }
}
}  // namespace

void ShowWhenFocused(lv_obj_t *obj, lv_obj_t *show)
{
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_remove_event_cb(obj, ShowCb);
    lv_obj_add_event_cb(obj, ShowCb, LV_EVENT_FOCUSED, show);
}

void RingEditing(lv_obj_t *obj, int32_t inset)
{
    constexpr lv_style_selector_t kEditing = LV_STATE_FOCUS_KEY | LV_STATE_EDITED;  // outranks the focus ring alone
    lv_obj_set_style_outline_width(obj, dt::kSpacingStrokeEdit, kEditing);
    lv_obj_set_style_outline_pad(obj, dt::kSpacingFocusOffset + inset, kEditing);
    lv_obj_set_style_outline_color(obj, Level(dt::kColorFocusRing), kEditing);
    lv_obj_set_style_outline_opa(obj, LV_OPA_COVER, kEditing);
}

}  // namespace ui
