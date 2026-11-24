#include "widgets/button.h"

#include "design_tokens.h"
#include "internal/base.h"
#include "internal/button_face.h"
#include "internal/marks.h"

namespace widgets {
namespace dt = design_tokens;

namespace detail {
void StyleButtonFace(lv_obj_t *button, ButtonVariant variant)
{
    // The edge is reserved at rest on every variant, so the compact buttons are as wide as each other's words.
    lv_obj_set_style_border_width(button, dt::kSpacingStrokeDefault, 0);
    lv_obj_set_style_border_color(button, ui::Level(dt::kColorOutline), 0);
    lv_obj_set_style_border_opa(button, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(button, dt::kRoundedLg, 0);  // the deck's one base radius
    lv_obj_set_style_text_font(button, ui::Font(ui::Type::kLabel), 0);
    lv_obj_set_style_text_color(button, ui::Level(dt::kColorTextPrimary), 0);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(button, ui::Level(dt::kColorBackground), 0);
    switch (variant) {
    case ButtonVariant::kAction:
        lv_obj_set_style_bg_color(button, ui::Level(dt::kColorSurfaceAction), 0);
        lv_obj_set_style_text_color(button, ui::Level(dt::kColorTextOnFill), 0);
        break;
    case ButtonVariant::kOutline:
        lv_obj_set_style_border_opa(button, LV_OPA_COVER, 0);
        break;
    case ButtonVariant::kTonal:
        lv_obj_set_style_bg_color(button, ui::Fill(dt::kColorBand), 0);  // black words only on the light dither
        break;
    case ButtonVariant::kGhost:
        lv_obj_set_style_bg_opa(button, LV_OPA_TRANSP, 0);
        break;
    }
    DitherWhenDisabled(button);
}
}  // namespace detail

lv_obj_t *MakeButton(lv_obj_t *parent, const char *label, ButtonVariant variant, int32_t width, Glyph glyph)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_remove_style_all(button);  // touch-exception: no pressed state (DESIGN → Motion → Press)
    lv_obj_set_scrollable(button, false);
    lv_obj_set_size(button, width > 0 ? width : LV_SIZE_CONTENT, dt::kSpacingTarget);
    detail::StyleButtonFace(button, variant);
    // Label room: spacing.md either side in a button of a set width, which holds its words inside the edge; spacing.lg in a
    // compact one, as wide as its words (DESIGN → Components → The kit).
    lv_obj_set_style_pad_hor(button, width > 0 ? dt::kSpacingMd : dt::kSpacingLg, 0);
    lv_obj_t *text = lv_label_create(button);
    lv_label_set_text(text, label);
    if (const lv_image_dsc_t *mark = detail::GlyphImage(glyph)) {
        // The glyph and the words are one group, centred, spacing.md between the ink and the first letter (the mark is
        // trimmed to its ink). The glyph takes the words' colour, and `text-disabled` when disabled, from the states
        // the button passes down.
        const uint8_t words = variant == ButtonVariant::kAction ? dt::kColorTextOnFill : dt::kColorTextPrimary;
        lv_obj_set_flex_flow(button, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(button, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(button, dt::kSpacingMd, 0);
        lv_obj_add_flag(button, LV_OBJ_FLAG_STATE_TRICKLE);
        lv_obj_move_to_index(detail::NewMark(button, mark, words, words), 0);
    } else {
        lv_obj_center(text);
    }
    detail::Focusable(button);
    return button;
}

namespace {
// A control's link to its reason. It clears itself when the reason goes, and the control's going frees it, so it never
// points at a deleted object (lv_obj_is_valid would read through such a pointer to test it).
struct ReasonCell {
    lv_obj_t *reason;
};

void ReasonGone(lv_event_t *e)
{
    static_cast<ReasonCell *>(lv_event_get_user_data(e))->reason = nullptr;
}

// On the control, LV_EVENT_DELETE: unhooks the reason and frees the cell. DisabledReason finds the link by this
// function's address, on purpose — no flag or user data of the control's own is taken.
void ReasonLink(lv_event_t *e)
{
    auto *cell = static_cast<ReasonCell *>(lv_event_get_user_data(e));
    if (cell->reason != nullptr) {
        lv_obj_remove_event_cb_with_user_data(cell->reason, ReasonGone, cell);
    }
    delete cell;
}

ReasonCell *CellOf(lv_obj_t *control)
{
    for (uint32_t i = 0; i < lv_obj_get_event_count(control); ++i) {
        lv_event_dsc_t *dsc = lv_obj_get_event_dsc(control, i);
        if (lv_event_dsc_get_cb(dsc) == ReasonLink) {
            return static_cast<ReasonCell *>(lv_event_dsc_get_user_data(dsc));
        }
    }
    return nullptr;
}
}  // namespace

void LinkDisabledReason(lv_obj_t *control, lv_obj_t *reason)
{
    if (reason == control) {
        return;  // a control is never its own reason
    }
    ReasonCell *cell = CellOf(control);
    if (cell == nullptr) {  // one link per control: a second call replaces the reason
        cell = new ReasonCell{nullptr};  // freed by ReasonLink when the control is deleted
        lv_obj_add_event_cb(control, ReasonLink, LV_EVENT_DELETE, cell);
    } else if (cell->reason != nullptr) {
        lv_obj_remove_event_cb_with_user_data(cell->reason, ReasonGone, cell);
    }
    cell->reason = reason;
    if (reason != nullptr) {
        lv_obj_add_event_cb(reason, ReasonGone, LV_EVENT_DELETE, cell);
    }
}

lv_obj_t *DisabledReason(lv_obj_t *control)
{
    const ReasonCell *cell = CellOf(control);
    return cell != nullptr ? cell->reason : nullptr;
}

}  // namespace widgets
