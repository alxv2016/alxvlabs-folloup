#include "internal/base.h"

#include "design_tokens.h"
#include "lvgl_display.h"

namespace widgets::detail {
namespace dt = design_tokens;

lv_obj_t *Bare(lv_obj_t *parent)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_scrollable(obj, false);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    return obj;
}

lv_obj_t *Pill(lv_obj_t *parent)
{
    lv_obj_t *pill = Bare(parent);
    lv_obj_set_style_radius(pill, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(pill, LV_OPA_COVER, 0);
    return pill;
}

void LetRingOut(lv_obj_t *obj)
{
    ui::LetRingOut(obj);
}

void Outline(lv_obj_t *obj, int32_t width)
{
    lv_obj_set_style_border_color(obj, ui::Level(dt::kColorOutline), 0);
    lv_obj_set_style_border_width(obj, width, 0);
    lv_obj_set_style_border_opa(obj, LV_OPA_COVER, 0);
}

lv_color_t DisabledOnFace()
{
    return ui::Fill(dt::kColorTextDisabled);
}

lv_color_t DisabledOnPage()
{
    return ui::Fill(dt::kColorTextDisabledPage);
}

void DitherWhenDisabled(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, ui::Fill(dt::kColorBand), LV_STATE_DISABLED);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_STATE_DISABLED);
    lv_obj_set_style_border_opa(obj, LV_OPA_TRANSP, LV_STATE_DISABLED);  // no edge of any kind; the width stays reserved
    lv_obj_set_style_text_color(obj, DisabledOnFace(), LV_STATE_DISABLED);
}

lv_obj_t *NewMark(lv_obj_t *parent, const lv_image_dsc_t *mark, uint8_t rest, uint8_t checked)
{
    constexpr lv_style_selector_t kOnDisabled = static_cast<lv_style_selector_t>(LV_STATE_DISABLED) | LV_STATE_CHECKED;
    lv_obj_t *image = lv_image_create(parent);
    lv_image_set_src(image, mark);
    lv_obj_remove_flag(image, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_image_recolor_opa(image, LV_OPA_COVER, 0);
    lv_obj_set_style_image_recolor(image, ui::Level(rest), 0);
    lv_obj_set_style_image_recolor(image, ui::Level(checked), LV_STATE_CHECKED);
    lv_obj_set_style_image_recolor(image, DisabledOnFace(), LV_STATE_DISABLED);
    lv_obj_set_style_image_recolor(image, DisabledOnFace(), kOnDisabled);
    return image;
}

lv_obj_t *Line(lv_obj_t *parent, ui::Type role, const char *text, bool secondary)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, ui::Font(role), 0);
    if (secondary) {
        lv_obj_set_style_text_color(label, ui::Level(dt::kColorTextSecondary), 0);
    }
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_DOTS);
    ui::FitLineBox(label, role);  // one line, on the role's line box: LVGL dots only a label of a fixed height
    return label;
}

lv_obj_t *Words(lv_obj_t *parent, ui::Type role, const char *text, bool secondary)
{
    lv_obj_t *label = ui::NewLabel(parent, role, text, secondary);
    ui::FitLineBox(label, role);
    return label;
}

void Focusable(lv_obj_t *obj)
{
    ui::RingOutside(obj);
    JoinGroup(obj);
}

void JoinGroup(lv_obj_t *obj, lv_obj_t *show)
{
    ui::ShowWhenFocused(obj, show);
    lv_group_add_obj(lvgl_display::Group(), obj);
}

bool FromKeys()
{
    lv_indev_t *indev = lv_indev_active();
    return indev != nullptr && lv_indev_get_type(indev) == LV_INDEV_TYPE_KEYPAD;
}

namespace {
void OnEditableClicked(lv_event_t *)
{
    if (FromKeys()) {
        lv_group_t *group = lvgl_display::Group();
        lv_group_set_editing(group, !lv_group_get_editing(group));
    }
}

// LVGL leaves editing when the group moves its focus (lv_group_focus_obj); this says so where a tap, which takes the
// focus the same way, is concerned, so the open ring never stays on a control the finger has left.
void OnEditableDefocused(lv_event_t *)
{
    lv_group_set_editing(lvgl_display::Group(), false);
}
}  // namespace

void ToggleEditingOnKeyClick(lv_obj_t *obj)
{
    lv_obj_add_event_cb(obj, OnEditableClicked, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(obj, OnEditableDefocused, LV_EVENT_DEFOCUSED, nullptr);
}

}  // namespace widgets::detail
