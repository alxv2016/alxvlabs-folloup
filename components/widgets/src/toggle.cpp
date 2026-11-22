#include "widgets/toggle.h"

#include "design_tokens.h"
#include "internal/base.h"

namespace widgets {
namespace dt = design_tokens;

namespace {
// The thumb is the kit's, not lv_switch's: lv_switch draws its knob a square the switch's height, travelling the switch's
// width less that, where the thumb is a pill wider than tall inset spacing.xs and travels spacing.toggle-width less its
// width and two insets. So the switch keeps its input — OK, a tap, LV_STATE_CHECKED, the ring — with its own knob
// unpainted, and a child is the thumb, as the deck's is.
constexpr int32_t kThumbHeight = dt::kSpacingToggleHeight - 2 * dt::kSpacingXs;
constexpr int32_t kThumbOn = dt::kSpacingToggleWidth - dt::kSpacingXs - dt::kSpacingToggleThumbWidth;

// The thumb lands on the end the state says, in the same frame (DESIGN → Motion → Nothing animates).
void PlaceThumb(lv_event_t *e)
{
    lv_obj_t *toggle = lv_event_get_target_obj(e);
    lv_obj_t *thumb = static_cast<lv_obj_t *>(lv_event_get_user_data(e));
    lv_obj_set_x(thumb, lv_obj_has_state(toggle, LV_STATE_CHECKED) ? kThumbOn : dt::kSpacingXs);
}
}  // namespace

lv_obj_t *MakeToggle(lv_obj_t *parent)
{
    lv_obj_t *toggle = lv_switch_create(parent);
    lv_obj_remove_style_all(toggle);
    lv_obj_set_size(toggle, dt::kSpacingToggleWidth, dt::kSpacingToggleHeight);
    lv_obj_set_style_anim_duration(toggle, 0, 0);  // nothing slides (DESIGN → Motion)
    lv_obj_set_style_bg_opa(toggle, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(toggle, LV_OPA_TRANSP, LV_PART_INDICATOR);
    // The track: the light dither off, black on; disabled the light dither either way, with no edge (DESIGN → Components → States).
    lv_obj_set_style_radius(toggle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(toggle, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(toggle, ui::Fill(dt::kColorBand), 0);
    lv_obj_set_style_bg_color(toggle, ui::Level(dt::kColorSurfaceOn), LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(toggle, ui::Fill(dt::kColorBand), LV_STATE_DISABLED);
    lv_obj_set_style_bg_color(toggle, ui::Fill(dt::kColorBand),
                              static_cast<lv_style_selector_t>(LV_STATE_DISABLED) | LV_STATE_CHECKED);

    // The thumb, a child that follows the switch's states (the switch passes them on, below).
    lv_obj_t *thumb = detail::Bare(toggle);
    lv_obj_set_size(thumb, dt::kSpacingToggleThumbWidth, kThumbHeight);
    lv_obj_set_pos(thumb, dt::kSpacingXs, dt::kSpacingXs);
    lv_obj_set_style_radius(thumb, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(thumb, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(thumb, ui::Level(dt::kColorThumb), 0);
    lv_obj_set_style_bg_color(thumb, ui::Level(dt::kColorThumbOn), LV_STATE_CHECKED);
    // Disabled, the thumb is a white part knocked out of the dither track, on either end: no edge, no change of size.
    lv_obj_set_style_bg_color(thumb, ui::Level(dt::kColorThumbOn), LV_STATE_DISABLED);
    lv_obj_set_style_bg_color(thumb, ui::Level(dt::kColorThumbOn),
                              static_cast<lv_style_selector_t>(LV_STATE_DISABLED) | LV_STATE_CHECKED);
    lv_obj_add_event_cb(toggle, PlaceThumb, LV_EVENT_STATE_CHANGED, thumb);

    // The thumb follows the track's disabled and checked states.
    lv_obj_add_flag(toggle, LV_OBJ_FLAG_STATE_TRICKLE);

    // Touches across spacing.target: the 44 px track padded 10 px above and below.
    lv_obj_set_ext_click_area(toggle, (dt::kSpacingTarget - dt::kSpacingToggleHeight) / 2);
    // The ring outside the track, 4 px clear of it, black on the white glass whether the toggle is on or off.
    ui::RingOutside(toggle);
    detail::JoinGroup(toggle);
    return toggle;
}

}  // namespace widgets
