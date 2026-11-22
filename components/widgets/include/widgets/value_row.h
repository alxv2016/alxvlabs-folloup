// A value row: a setting's name left, its value right in text-secondary, the value ending in "..." (three full stops,
// which LVGL draws) when its full words are one press away (DESIGN → Components → The kit).

#pragma once

#include "lvgl.h"

namespace widgets {

struct ValueRow {
    lv_obj_t *root;
    lv_obj_t *label;
    lv_obj_t *value;
};

// A row `spacing.target` high, full width, its words `spacing.md` in from its edges, in the focus group with its ring
// outside it; no fill, so the ring is its only edge. The caller listens for LV_EVENT_CLICKED. Disabled
// (LV_STATE_DISABLED), the light dither with no edge and its words `text-disabled`.
ValueRow MakeValueRow(lv_obj_t *parent, const char *name, const char *value);
void SetValueRow(const ValueRow &row, const char *value);

}  // namespace widgets
