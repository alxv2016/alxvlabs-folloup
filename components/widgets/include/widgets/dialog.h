// A dialog: a decision the person must make now, with one action button and Cancel, standing on a `spacing.stroke.strong`
// outline with an offset in the dark dither (`shadow`) on its right and bottom edges, never solid black (DESIGN → Elevation &
// Depth; DESIGN → Components → Dialog). MakeDialog is the box alone, a static piece of a page — the kit's specimen; OpenDialog
// holds a real one open as an overlay on the router's stack, which takes focus and gives it back.

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

// The dialog answered: `accepted` is the action, false for Cancel or holding OK. Runs under the lock, after the dialog has closed
// and focus is back on what opened it.
using DialogDone = void (*)(bool accepted, void *user);

struct DialogConfig {
    const char *title;
    const char *message;
    const char *action;  // named for what it does: "Delete"
    const char *cancel;  // "Cancel"; nullptr is "Cancel"
    bool destructive;    // the action destroys something: the dialog opens with focus on Cancel (DESIGN → Components → Dialog)
    DialogDone done;
    void *user;
};

// Opens MakeDialog's box over the middle of the active screen, as an overlay: its two buttons are the only stops, focus on the
// action — on Cancel for a destructive one — and a touch anywhere outside the box is eaten, as the select sheet's is. The action
// or Cancel closes it and answers; holding OK is Cancel. It opens and closes on a partial refresh (DESIGN → Motion). One at a
// time; it may open over a select sheet, and closing it gives the buttons back to the sheet.
void OpenDialog(const DialogConfig &config);
// Closes the open dialog as Cancel does; does nothing when none is open.
void CloseDialog();
bool DialogOpen();

}  // namespace widgets
