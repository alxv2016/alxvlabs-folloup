// A step bar, the deck's: up to five round-capped pieces `spacing.step-gap` apart — done black, running part-filled with
// the rest of its track in the light dither after a gap, waiting the track alone, failed a hollow outline and said so in
// words beside it (DESIGN → Components → The kit).

#pragma once

#include <cstdint>

#include "lvgl.h"

namespace widgets {

inline constexpr int kStepMax = 5;
enum class StepState : uint8_t { kWaiting, kRunning, kDone, kFailed };

struct StepBar {
    lv_obj_t *root;
    lv_obj_t *fill[kStepMax];
    lv_obj_t *track[kStepMax];
    int count;
    int32_t width;
};

StepBar MakeStepBar(lv_obj_t *parent, int32_t width, int count);
// `permille`, 0 to 1000, is how much of a running step is filled; a running one shows at least a dot of fill, and a dot of
// track after it until it is done.
void SetStep(const StepBar &bar, int index, StepState state, int32_t permille = 0);

}  // namespace widgets
