// A progress bar, the deck's: a black round-capped fill from the left, `spacing.sm` of white, then the rest of the track in
// the light dither; it steps, never glides (DESIGN → Components → The kit).

#pragma once

#include <cstdint>

#include "lvgl.h"

namespace widgets {

struct ProgressBar {
    lv_obj_t *root;
    lv_obj_t *fill;
    lv_obj_t *track;
    int32_t width;
    int32_t stroke;
};

// A bar `width` wide and `spacing.progress` high — `spacing.progress-lead` when `lead` (the one that runs out first).
ProgressBar MakeProgressBar(lv_obj_t *parent, int32_t width, bool lead = false);
// 0 to 100. The fill's end moves the same distance every step, and a dot of track shows after it until the top, which is one
// more such step with the track gone: at 100 the fill ends short of where the track would, as the deck's does.
void SetProgress(const ProgressBar &bar, int32_t percent);

}  // namespace widgets
