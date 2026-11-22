// A range slider, ported from the agent deck's (STICKY_DESIGN_TOKENS §9): a light-dither track `spacing.slider-height` high with a black fill inset
// `spacing.xs`, a white knob bar at the fill's end, and the value in `caption` right-aligned in the fill beside the knob —
// on the track past the knob when the fill is too short. OK opens it for editing (the ring thickens to
// `spacing.stroke.edit`), UP and DOWN step it — one partial refresh a step — and OK closes it; a finger's drag lands its
// value on lift (DESIGN → Components → The kit).

#pragma once

#include <cstdint>

#include "lvgl.h"

namespace widgets {

struct RangeSlider {
    lv_obj_t *root;  // the track and the focus stop; sends LV_EVENT_VALUE_CHANGED
    lv_obj_t *fill;
    lv_obj_t *knob;
    lv_obj_t *label;
};

// A track full width, its ring outside it; its parents let the ring out (`LetRingOut`) and its name — a `caption` kept
// `spacing.md` above it — is the screen's. `suffix` follows the number in its words ("%", " min", no space before
// "%"), kept by pointer, so a literal; `at_min` is the words at the bottom ("Off") in place of the number, null for the
// number.
RangeSlider MakeRangeSlider(lv_obj_t *parent, int32_t min, int32_t max, int32_t step, int32_t value,
                            const char *suffix, const char *at_min = nullptr);
int32_t RangeSliderValue(const RangeSlider &slider);
void SetRangeSliderValue(const RangeSlider &slider, int32_t value);  // no event
// Shows the slider disabled — the track the light dither with no edge, the fill a white part knocked out of it, the knob and
// the value `text-disabled-page`, the buttons skip it — or enabled again; its reason belongs in words under it, which the
// screen writes.
void SetRangeSliderEnabled(const RangeSlider &slider, bool enabled);

}  // namespace widgets
