// A list row: a title, and a supporting line under it when there is one — the kit's only list row, the Home list's and the
// shell the Wi-Fi network row is built on (DESIGN → Components → The kit).

#pragma once

#include "lvgl.h"

namespace widgets {

// A row `spacing.list-row-height` high, full width, `rounded.lg`, its ring outside it, no rule below: the title in `label`, one
// line, and under it the supporting line in `caption` and `text-secondary`, each ending in LVGL's three full stops when too long.
// A row with a click handler joins the focus group and is brought into view when focused; one without it (`on_click` null) is
// the shell of a composite, which joins the group itself. `on_click` runs on LV_EVENT_CLICKED with `user_data`.
lv_obj_t *MakeListRow(lv_obj_t *parent, const char *label, const char *supporting, lv_event_cb_t on_click, void *user_data);
// list-row-chosen: the row inverted, `surface-chosen` with its words `text-on-fill` — never the focus, which is the ring.
void SetListRowChosen(lv_obj_t *row, bool chosen);

}  // namespace widgets
