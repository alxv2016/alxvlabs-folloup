// A dialog: a decision the person must make now, with one action button and Cancel, standing on a `spacing.stroke.strong`
// outline with an offset in the dark dither (`shadow`) on its right and bottom edges, never solid black (DESIGN → Elevation &
// Depth). This is the box alone — a static piece of a page; the overlay that holds a real one open, takes focus and gives it
// back belongs to the interaction framework, which has not landed.

#pragma once

#include "lvgl.h"

namespace widgets {

struct Dialog {
    lv_obj_t *frame;   // the object the page places: full width, as tall as the box
    lv_obj_t *box;     // the outlined box, which draws the offset
    lv_obj_t *action;  // the one action button (black)
    lv_obj_t *cancel;  // Cancel (outline)
};

// A box `spacing.content` wide: `spacing.stroke.strong` outline, `rounded.md`, `spacing.lg` inside it; the title in `label`, the
// message in `body`, then the two half buttons `spacing.md` apart. The offset stands `spacing.overlay-offset` right of the box
// and below it, inside the page's margin. Both buttons are focus stops, the action first; a destructive dialog's caller
// focuses Cancel. The caller listens for LV_EVENT_CLICKED on each.
Dialog MakeDialog(lv_obj_t *parent, const char *title, const char *message, const char *action, const char *cancel);

}  // namespace widgets
