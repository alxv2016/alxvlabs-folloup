// A segment: up to four choices on one light-dither track, the chosen one a black pill behind its white word, ported from
// the agent deck's segment (STICKY_DESIGN_TOKENS §9; DESIGN → Components → The kit).

#pragma once

#include "lvgl.h"

namespace widgets {

inline constexpr int kSegmentMax = 4;

// A track `spacing.target` high, full width, its items `spacing.sm` in from its edge and apart; each choice a focus stop
// with the ring round its item, inside the track. The root sends LV_EVENT_VALUE_CHANGED when the choice changes. A
// disabled track (LV_STATE_DISABLED) is the light dither with no edge, its words `text-disabled` and the chosen pill a white
// part knocked out of it, and no item is a stop.
lv_obj_t *MakeSegment(lv_obj_t *parent, const char *const labels[], int count, int chosen);
int SegmentChosen(lv_obj_t *segment);
void SetSegmentChosen(lv_obj_t *segment, int index);  // no event
// Shows the track disabled or enabled again; its reason belongs in words beside it, which the screen writes.
void SetSegmentEnabled(lv_obj_t *segment, bool enabled);

}  // namespace widgets
