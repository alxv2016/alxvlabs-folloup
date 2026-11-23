// A roller: one value in a long ordered list or range, shown between its neighbours — three rows of `spacing.target`, the value
// in `label` in the middle row, the value above and the value below it in `caption` `text-secondary` as still words, never a
// grey or a fade — with its name in `caption` `spacing.md` above it (DESIGN → Components → The kit). Nothing rolls, nothing
// animates and nothing follows a finger: a step is the next frame, one partial refresh of the three rows.
//
// KEYS. OK opens it for editing — the ring thickens to `spacing.stroke.edit`, the middle row is the chosen inversion — and UP
// and DOWN are then its own: UP steps to the value above (the earlier index), DOWN to the one below, a held key repeating; OK keeps
// the value and closes, and the roller sends LV_EVENT_VALUE_CHANGED once, only when the value is not the one OK opened it on.
// Holding OK puts back the value it opened on (router::Escape's first link, as the range slider's edit). A roller marked circular
// (hours, minutes, months) wraps at its ends with the key's tick alone; any other stops there, and the step that ran into the end
// sounds `sound.at-end`.
//
// TOUCH. A tap on a closed roller opens it. Open, a tap on the row above or below steps one, a tap on the middle row keeps, and
// one vertical swipe steps one on the lift (the swipe the pages share, widgets/internal/swipe.h): by one value, never by pixel.
//
// THE FOOTER. While a roller is open the screen's footer shows Cancel and what the buttons do in words (MakeRollerFooter, bound
// with SetRollerFooter): a touch stop that puts the value back; by the buttons that is holding OK, which the hint says.

#pragma once

#include <cstddef>

#include "lvgl.h"

namespace widgets {

struct Roller {
    lv_obj_t *frame;  // the object the screen places: the name, then the roller; its parents let the focus ring out (`LetRingOut`)
    lv_obj_t *root;   // the roller and the focus stop; sends LV_EVENT_VALUE_CHANGED on keep
};

// A roller of `count` values (at least two) whose text is `label(index, buf, size, user)`: it returns `buf`, or any string that
// outlives the call. A null `label` shows `first + index * step` in at least `digits` figures, zero-filled ("08"). `selected` is
// the index it starts on. `circular` wraps it at its ends. The roller is as wide as its widest value and `spacing.lg` either side,
// `spacing.target` at least.
struct RollerSpec {
    const char *name;
    int count;
    const char *(*label)(int index, char *buf, size_t size, void *user);
    void *user;
    int selected;
    bool circular;
    int first;
    int step;
    int digits;
};

Roller MakeRoller(lv_obj_t *parent, const RollerSpec &spec);
// A roller of numbers: first, first + step, … up to `last`, starting on `value` (the nearest value of the list when it is not
// one), shown in at least `digits` figures.
Roller MakeNumberRoller(lv_obj_t *parent, const char *name, int first, int last, int step, int digits, bool circular, int value);

int RollerSelected(const Roller &roller);     // the kept index; the shown one while it is open
int RollerValue(const Roller &roller);        // a number roller's value, `first + index * step`
void SetRoller(const Roller &roller, int index);       // closed: no event
void SetRollerValue(const Roller &roller, int value);  // a number roller's value, no event
bool RollerOpen(const Roller &roller);

// The footer's bar for open rollers, made once in the screen's footer, after its other parts: Cancel (outline, half width) and
// "UP or DOWN steps. OK keeps. Hold OK cancels." in `caption` `text-secondary`. It is hidden until a bound roller opens, when it
// takes the footer's place and the footer's other parts are hidden; when the roller closes the parts are back. Cancel is a touch
// stop, in no focus group.
lv_obj_t *MakeRollerFooter(lv_obj_t *footer);
// Binds `roller` to the footer's bar.
void SetRollerFooter(const Roller &roller, lv_obj_t *bar);

}  // namespace widgets
