// What every widget shares: a bare object, an outline, the disabled look, a one-line label, joining the focus group,
// and (swipe.h) the swipe that turns a page. Private to the widgets component (DESIGN → Components → States).

#pragma once

#include <cstdint>

#include "internal/swipe.h"
#include "lvgl.h"
#include "ui.h"

namespace widgets::detail {

// An object with LVGL's styles removed, that never scrolls and takes no tap: a widget that wants touches adds
// LV_OBJ_FLAG_CLICKABLE itself, so a child — a slider's fill, a mark's dot — never swallows one meant for its parent.
lv_obj_t *Bare(lv_obj_t *parent);
// A pill: a bare object whose radius is its own half height.
lv_obj_t *Pill(lv_obj_t *parent);
// `ui::LetRingOut`: lets the focus ring a child draws outside `obj` out of it. Call it on every container between a control
// with its ring outside and the page (STICKY_DESIGN_TOKENS §9).
void LetRingOut(lv_obj_t *obj);
// A black outline of `width`.
void Outline(lv_obj_t *obj, int32_t width);
// Disabled is one step lighter than the ground, with no edge (DESIGN → Components → States → Disabled): the object's face
// the light dither and its words `text-disabled`, the sparse dither of one black pixel in sixteen. An edge the object
// keeps at rest — a button's reserved 3 px, an outline button's drawn one — goes transparent, not zero, so its size
// does not change; its width stays reserved.
void DitherWhenDisabled(lv_obj_t *obj);
// `text-disabled`: words and marks on a disabled control's face. A colour for a style or for a label.
lv_color_t DisabledOnFace();
// `text-disabled-page`: a disabled row's name and state word, a slider's knob and value, a mark's dot — on the white.
lv_color_t DisabledOnPage();
// An image of a mark that make_marks.py compiled (marks.h), recoloured solid: `rest` is its colour level, `checked` its
// level while LV_STATE_CHECKED, and while LV_STATE_DISABLED it is `text-disabled` (on the face, either way). The mark
// takes those states from a parent that passes them down (LV_OBJ_FLAG_STATE_TRICKLE), so the caller sets that flag on
// the control. Not clickable; whether it shows at all (a check only while checked) is the caller's `image_opa` styles.
lv_obj_t *NewMark(lv_obj_t *parent, const lv_image_dsc_t *mark, uint8_t rest, uint8_t checked);
// One line in `role`, on the role's line box (`ui::FitLineBox`), so a name over a detail line stands a line box's spare room
// further apart (STICKY_DESIGN_TOKENS §9); LVGL ends one that does not fit with "..." (three full stops).
lv_obj_t *Line(lv_obj_t *parent, ui::Type role, const char *text, bool secondary);
// Words that wrap, in `role`, each line on the role's line box — the same for a name or a detail line that may take two.
lv_obj_t *Words(lv_obj_t *parent, ui::Type role, const char *text, bool secondary = false);
// Adds the object to the focus group, its ring outside it (`ui::RingOutside`), and has it brought into view at once.
void Focusable(lv_obj_t *obj);
// Adds the object to the focus group, with no ring of its own: the caller has drawn it. A page that scrolls brings the
// object, and its ring, into view when it is focused, with no animation: `ui::ShowWhenFocused`. A container that turns pages
// itself, a list view, has its rows bring the whole view into view instead (`ui::ShowWhenFocused(row, view)`); `show` is that
// object when a control draws a ring round more than itself (a text view's band, inside the box its ring belongs to).
void JoinGroup(lv_obj_t *obj, lv_obj_t *show = nullptr);
// Whether the event came from the buttons, not the touch panel.
bool FromKeys();
// OK on an editable widget opens it for editing, and OK again closes it, keeping what was done: UP and DOWN then reach the
// widget. A press that takes the focus elsewhere — a tap on another control — closes it too. `opened`, when given, runs as OK
// opens it, so the widget keeps what LV_EVENT_CANCEL puts back: holding OK while it is open cancels the edit (router::Escape,
// DESIGN → Focus and input).
void ToggleEditingOnKeyClick(lv_obj_t *obj, lv_event_cb_t opened = nullptr);

}  // namespace widgets::detail
