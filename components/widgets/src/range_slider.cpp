#include "widgets/range_slider.h"

#include <cstdio>

#include "design_tokens.h"
#include "internal/base.h"
#include "lvgl_display.h"
#include "router/router.h"

namespace widgets {
namespace dt = design_tokens;

namespace {
// The deck's parts, ported: a fill inset spacing.xs on all four sides, a knob inset by the same from the fill's end, and a
// minimum fill that holds the knob alone, so the knob reaches the track's edge at the bottom.
constexpr int32_t kInset = dt::kSpacingXs;
constexpr int32_t kFillHeight = dt::kSpacingSliderHeight - 2 * kInset;
constexpr int32_t kKnobWidth = dt::kSpacingSm;
constexpr int32_t kKnobHeight = kFillHeight - 2 * kInset;
constexpr int32_t kMinFill = kInset + kKnobWidth + kInset;

struct State {
    int32_t min, max, step, value;
    int32_t opened_value;   // the value when OK opened the edit: holding OK puts it back
    int32_t pressed_value;  // the value when a finger went down: a lift off the track puts it back
    const char *suffix;
    const char *at_min;
    lv_obj_t *fill, *knob, *label;
};

State *Of(lv_obj_t *root)
{
    return static_cast<State *>(lv_obj_get_user_data(root));
}

// The fill to the value, the knob at its end, and the words: right-aligned in the fill beside the knob, white on it,
// while they fit in it; else on the track past the knob, black. Disabled, the fill is a white part knocked out of the dither
// track, so the words inside it are `text-disabled-page` and those on the track `text-disabled`.
void Place(lv_obj_t *root)
{
    State *s = Of(root);
    lv_obj_update_layout(root);
    const int32_t track = lv_obj_get_width(root);
    const int32_t travel = track - 2 * kInset;
    const int32_t span = s->max - s->min;
    int32_t width = span > 0 ? travel * (s->value - s->min) / span : 0;
    width = width < kMinFill ? kMinFill : width > travel ? travel : width;
    lv_obj_set_size(s->fill, width, kFillHeight);
    lv_obj_set_pos(s->fill, kInset, kInset);
    const int32_t knob_x = kInset + width - kInset - kKnobWidth;
    lv_obj_set_pos(s->knob, knob_x, 2 * kInset);

    if (s->value == s->min && s->at_min != nullptr) {
        lv_label_set_text(s->label, s->at_min);
    } else {
        lv_label_set_text_fmt(s->label, "%ld%s", static_cast<long>(s->value), s->suffix);
    }
    lv_obj_update_layout(s->label);
    const int32_t words = lv_obj_get_width(s->label);
    const bool disabled = lv_obj_has_state(root, LV_STATE_DISABLED);
    const bool inside = knob_x - dt::kSpacingSm - words >= kInset + kMinFill;
    lv_obj_set_style_text_color(s->label,
                                disabled ? (inside ? detail::DisabledOnPage() : detail::DisabledOnFace())
                                         : ui::Level(inside ? dt::kColorTextOnFill : dt::kColorTextPrimary),
                                0);
    lv_obj_set_pos(s->label, inside ? knob_x - dt::kSpacingSm - words : knob_x + kKnobWidth + kInset + dt::kSpacingSm,
                   (dt::kSpacingSliderHeight - lv_obj_get_height(s->label)) / 2);
}

void Set(lv_obj_t *root, int32_t value, bool tell)
{
    State *s = Of(root);
    value = value < s->min ? s->min : value > s->max ? s->max : value;
    if (value == s->value) {
        return;
    }
    s->value = value;
    Place(root);
    if (tell) {
        lv_obj_send_event(root, LV_EVENT_VALUE_CHANGED, nullptr);
    }
}

// While open for editing, UP and DOWN step the value; a step past an end changes nothing, and the at-end sound says so
// after the press's tick — each repeated step of a held key too.
void OnKey(lv_event_t *e)
{
    lv_obj_t *root = lv_event_get_target_obj(e);
    const uint32_t key = lv_event_get_key(e);
    const int32_t before = Of(root)->value;
    if (key == LV_KEY_UP || key == LV_KEY_RIGHT) {
        Set(root, before + Of(root)->step, true);
    } else if (key == LV_KEY_DOWN || key == LV_KEY_LEFT) {
        Set(root, before - Of(root)->step, true);
    } else {
        return;
    }
    if (Of(root)->value == before) {
        lvgl_display::PlayCue(lvgl_display::Cue::kAtEnd);
    }
}

// OK opened the edit: what holding OK puts back.
void OnEditOpened(lv_event_t *e)
{
    Of(lv_event_get_current_target_obj(e))->opened_value = Of(lv_event_get_current_target_obj(e))->value;
}

// Holding OK while the edit is open: the value it had when it opened (router::Escape's first link).
void OnCancel(lv_event_t *e)
{
    lv_obj_t *root = lv_event_get_current_target_obj(e);
    Set(root, Of(root)->opened_value, true);
}

void OnPressed(lv_event_t *e)
{
    lv_obj_t *root = lv_event_get_current_target_obj(e);
    Of(root)->pressed_value = Of(root)->value;
}

// The press was lost — the finger lifted off the track past the slop, which the router cancels, or the page took the drag: the
// value the press found (DESIGN → Focus and input).
void OnPressLost(lv_event_t *e)
{
    lv_obj_t *root = lv_event_get_current_target_obj(e);
    Set(root, Of(root)->pressed_value, true);
}

// A finger lands the value where it lifts, to the nearest step: one refresh, never one a frame.
void OnReleased(lv_event_t *e)
{
    // A touch that began as a drag of the page behind — the kit page scrolls — is that drag's, not a value: LVGL names the
    // object the pointer scrolled, and it is not a tap on this track.
    lv_indev_t *indev = lv_indev_active();
    if (detail::FromKeys() || indev == nullptr || lv_indev_get_scroll_obj(indev) != nullptr) {
        return;
    }
    lv_obj_t *root = lv_event_get_target_obj(e);
    lv_point_t point;
    lv_indev_get_point(lv_indev_active(), &point);
    lv_area_t area;
    lv_obj_get_coords(root, &area);
    State *s = Of(root);
    const int32_t travel = lv_area_get_width(&area) - 2 * kInset;
    int32_t x = point.x - area.x1 - kInset;
    x = x < 0 ? 0 : x > travel ? travel : x;
    const int32_t raw = s->min + (s->max - s->min) * x / (travel > 0 ? travel : 1);
    Set(root, s->min + (raw - s->min + s->step / 2) / s->step * s->step, true);
}

void OnDelete(lv_event_t *e)
{
    delete Of(lv_event_get_target_obj(e));
}

// A disabled slider's fill and words follow the state: the fill white, the words the sparse dither.
void OnStateChanged(lv_event_t *e)
{
    lv_obj_t *root = lv_event_get_target_obj(e);
    if (((lv_event_get_prev_state(e) ^ lv_obj_get_state(root)) & LV_STATE_DISABLED) != 0) {
        Place(root);
    }
}
}  // namespace

RangeSlider MakeRangeSlider(lv_obj_t *parent, int32_t min, int32_t max, int32_t step, int32_t value,
                            const char *suffix, const char *at_min)
{
    RangeSlider slider{};
    slider.root = detail::Bare(parent);
    lv_obj_set_size(slider.root, lv_pct(100), dt::kSpacingSliderHeight);
    lv_obj_add_flag(slider.root, LV_OBJ_FLAG_CLICKABLE);
    // The track: the light dither, disabled too, with no edge.
    lv_obj_set_style_radius(slider.root, dt::kRoundedLg, 0);
    lv_obj_set_style_bg_opa(slider.root, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(slider.root, ui::Fill(dt::kColorBand), 0);
    detail::LetRingOut(slider.root);
    lv_obj_add_flag(slider.root, LV_OBJ_FLAG_STATE_TRICKLE);  // the fill and the knob follow the disabled state

    // The fill: black, its corners the track's less the inset; disabled, a white part knocked out of the track.
    slider.fill = detail::Bare(slider.root);
    lv_obj_set_style_radius(slider.fill, dt::kRoundedMd, 0);
    lv_obj_set_style_bg_opa(slider.fill, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(slider.fill, ui::Level(dt::kColorSurfaceOn), 0);
    lv_obj_set_style_bg_color(slider.fill, ui::Level(dt::kColorBackground), LV_STATE_DISABLED);

    slider.label = lv_label_create(slider.root);
    lv_obj_set_style_text_font(slider.label, ui::Font(ui::Type::kCaption), 0);

    // The knob: a white pill on the black fill; `text-disabled-page` on the disabled fill's white.
    slider.knob = detail::Bare(slider.root);
    lv_obj_set_size(slider.knob, kKnobWidth, kKnobHeight);
    lv_obj_set_style_radius(slider.knob, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(slider.knob, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(slider.knob, ui::Level(dt::kColorKnob), 0);
    lv_obj_set_style_bg_color(slider.knob, detail::DisabledOnPage(), LV_STATE_DISABLED);

    State *state = new State{min, max, step > 0 ? step : 1, min - 1, min - 1, min - 1, suffix, at_min, slider.fill,
                             slider.knob, slider.label};
    lv_obj_set_user_data(slider.root, state);
    lv_obj_add_event_cb(slider.root, OnDelete, LV_EVENT_DELETE, nullptr);
    lv_obj_add_event_cb(slider.root, OnKey, LV_EVENT_KEY, nullptr);
    lv_obj_add_event_cb(slider.root, OnReleased, LV_EVENT_RELEASED, nullptr);
    lv_obj_add_event_cb(slider.root, OnPressed, LV_EVENT_PRESSED, nullptr);
    lv_obj_add_event_cb(slider.root, OnPressLost, LV_EVENT_PRESS_LOST, nullptr);
    lv_obj_add_event_cb(slider.root, OnCancel, LV_EVENT_CANCEL, nullptr);
    router::CancelOnSlideOff(slider.root);  // a lift off the track, past the slop, lands nothing
    lv_obj_add_event_cb(slider.root, OnStateChanged, LV_EVENT_STATE_CHANGED, nullptr);
    detail::ToggleEditingOnKeyClick(slider.root, OnEditOpened);
    // Touches across spacing.target: the 52 px track padded 6 px above and below.
    lv_obj_set_ext_click_area(slider.root, (dt::kSpacingTarget - dt::kSpacingSliderHeight) / 2);
    ui::RingOutside(slider.root);
    ui::RingEditing(slider.root);
    detail::JoinGroup(slider.root);
    Set(slider.root, value, false);
    state->opened_value = state->value;
    state->pressed_value = state->value;
    return slider;
}

int32_t RangeSliderValue(const RangeSlider &slider)
{
    return Of(slider.root)->value;
}

void SetRangeSliderValue(const RangeSlider &slider, int32_t value)
{
    Set(slider.root, value, false);
}

void SetRangeSliderEnabled(const RangeSlider &slider, bool enabled)
{
    if (!enabled && lv_obj_has_state(slider.root, LV_STATE_EDITED)) {
        if (lv_group_t *group = lv_obj_get_group(slider.root)) {
            lv_group_set_editing(group, false);  // out of editing first, so the group does not edit nothing
        }
    }
    lv_obj_set_state(slider.root, LV_STATE_DISABLED, !enabled);
}

}  // namespace widgets
