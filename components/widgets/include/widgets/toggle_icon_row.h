// A toggle icon row: a setting's name, its state in words, then the toggle icon button at the content's right edge — a toggle
// row with the tile for its control; OK on the tile or a tap anywhere on the row flips it (DESIGN → Components → The kit).

#pragma once

#include "lvgl.h"

namespace widgets {

struct ToggleIconRow {
    lv_obj_t *root;
    lv_obj_t *label;
    lv_obj_t *state;   // "On" or "Off"
    lv_obj_t *reason;  // a `caption` under the name while the row is disabled, hidden otherwise
    lv_obj_t *button;  // the tile, the row's focus stop; a tap on the row forwards its flip here
};

// A row at least `spacing.target` high, full width; the tile is the focus stop and sends LV_EVENT_VALUE_CHANGED. Its ring
// leaves 8 px, so rows stand `spacing.md` apart or more. `on` sets the start.
ToggleIconRow MakeToggleIconRow(lv_obj_t *parent, const char *name, bool on);
// Shows the row disabled — the tile the light dither with its glyph `text-disabled`, the words `text-disabled-page`, the
// reason in words under the name — or enabled again.
void SetToggleIconRowEnabled(const ToggleIconRow &row, bool enabled, const char *reason = nullptr);

}  // namespace widgets
