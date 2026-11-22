#include "widgets/segment.h"

#include "design_tokens.h"
#include "internal/base.h"

namespace widgets {
namespace dt = design_tokens;

namespace {
void OnItemClicked(lv_event_t *e)
{
    lv_obj_t *item = lv_event_get_target_obj(e);
    lv_obj_t *segment = lv_obj_get_parent(item);
    const int index = static_cast<int>(lv_obj_get_index(item));
    if (index == SegmentChosen(segment)) {
        return;
    }
    SetSegmentChosen(segment, index);
    lv_obj_send_event(segment, LV_EVENT_VALUE_CHANGED, nullptr);
}

}  // namespace

lv_obj_t *MakeSegment(lv_obj_t *parent, const char *const labels[], int count, int chosen)
{
    constexpr lv_style_selector_t kOnDisabled = static_cast<lv_style_selector_t>(LV_STATE_DISABLED) | LV_STATE_CHECKED;
    lv_obj_t *segment = detail::Bare(parent);
    lv_obj_set_size(segment, lv_pct(100), dt::kSpacingTarget);
    lv_obj_set_style_radius(segment, dt::kRoundedXl, 0);
    // The track is the light dither, disabled too, with no edge (DESIGN → Components → States → Disabled).
    lv_obj_set_style_bg_opa(segment, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(segment, ui::Fill(dt::kColorBand), 0);
    lv_obj_set_style_pad_all(segment, dt::kSpacingSm, 0);
    lv_obj_add_flag(segment, LV_OBJ_FLAG_STATE_TRICKLE);  // a disabled track disables its items: no stop, no touch
    lv_obj_set_style_pad_column(segment, dt::kSpacingSm, 0);
    lv_obj_set_flex_flow(segment, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_text_font(segment, ui::Font(ui::Type::kLabel), 0);
    count = count > kSegmentMax ? kSegmentMax : count;
    for (int i = 0; i < count; ++i) {
        lv_obj_t *item = lv_button_create(segment);
        lv_obj_remove_style_all(item);  // touch-exception: no pressed state (DESIGN → Motion → Press)
        lv_obj_set_scrollable(item, false);
        lv_obj_set_height(item, lv_pct(100));
        lv_obj_set_flex_grow(item, 1);
        lv_obj_set_style_radius(item, dt::kRoundedMd, 0);
        lv_obj_set_style_text_color(item, ui::Level(dt::kColorTextPrimary), 0);
        // The chosen item is the pill: black with a white word. Disabled, the words are `text-disabled` on the track's face;
        // the chosen pill is a white part knocked out of it, its word `text-disabled-page` on that white.
        lv_obj_set_style_bg_color(item, ui::Level(dt::kColorSurfaceChosen), LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(item, LV_OPA_COVER, LV_STATE_CHECKED);
        lv_obj_set_style_text_color(item, ui::Level(dt::kColorTextOnFill), LV_STATE_CHECKED);
        lv_obj_set_style_text_color(item, detail::DisabledOnFace(), LV_STATE_DISABLED);
        lv_obj_set_style_bg_color(item, ui::Level(dt::kColorBackground), kOnDisabled);
        lv_obj_set_style_text_color(item, detail::DisabledOnPage(), kOnDisabled);
        // The touch box: the 48 px item padded spacing.sm all round — 64 px high, and across the 8 px gap, where the later
        // of two items takes the touch.
        lv_obj_set_ext_click_area(item, dt::kSpacingSm);
        lv_obj_t *label = lv_label_create(item);
        lv_label_set_text(label, labels[i]);
        lv_obj_center(label);
        lv_obj_add_event_cb(item, OnItemClicked, LV_EVENT_CLICKED, nullptr);
        // The ring is round the item and inside the track: the track's inset is the ring's reach, so it ends on the
        // track's edge (the item's radius plus the ring is the track's); black, on the dither, chosen or not.
        ui::RingOutside(item);
        detail::JoinGroup(item);
    }
    SetSegmentChosen(segment, chosen);
    return segment;
}

int SegmentChosen(lv_obj_t *segment)
{
    const uint32_t n = lv_obj_get_child_count(segment);
    for (uint32_t i = 0; i < n; ++i) {
        if (lv_obj_has_state(lv_obj_get_child(segment, static_cast<int32_t>(i)), LV_STATE_CHECKED)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void SetSegmentChosen(lv_obj_t *segment, int index)
{
    const uint32_t n = lv_obj_get_child_count(segment);
    for (uint32_t i = 0; i < n; ++i) {
        lv_obj_set_state(lv_obj_get_child(segment, static_cast<int32_t>(i)), LV_STATE_CHECKED,
                         static_cast<int>(i) == index);
    }
}

void SetSegmentEnabled(lv_obj_t *segment, bool enabled)
{
    lv_obj_set_state(segment, LV_STATE_DISABLED, !enabled);
}

}  // namespace widgets
