// A dropdown: a setting's name left, the chosen option right in `text-secondary`, and a select chevron beside the value —
// BYTE-90's dropdown ported as a value row (DESIGN → Components → The kit). The field is the closed control and owns its
// options; the list a press opens is the select sheet (select_sheet.h), a bottom sheet over the lower part of the page, which
// the field opens through its opener and which reports a choice with ChooseDropdown.

#pragma once

#include <string>

#include "lvgl.h"

namespace widgets {

struct Dropdown;

// What the field does on OK or a tap: the sheet opens here. It runs under the lock, from the input handler.
using DropdownOpener = void (*)(Dropdown *dropdown, void *user);

struct Dropdown {
    lv_obj_t *frame;   // the object the page places: the field, and the reason under it while it is disabled
    lv_obj_t *field;   // a value row `spacing.target` high, the focus stop; sends LV_EVENT_VALUE_CHANGED once per committed change
    lv_obj_t *value;   // the chosen option's words
    lv_obj_t *reason;  // a `caption` under the field while it is disabled, hidden otherwise
    std::string name;  // the field's name: the sheet's title
    const char *const *options;
    int count;
    int selected;
    DropdownOpener opener;
    void *user;
};

// The field takes the caller's `options` — `count` strings that outlive it, as long as a sheet row's two lines hold (about 60
// characters; a longer one grows the row) — and starts on `selected` (the first when that is outside the options). OK or a tap opens the select sheet; SetDropdownOpener
// swaps in something else. Hand the struct back to the functions below by pointer: it lives as long as the field.
// The chevron is 14 x 28 at one bit, `spacing.md` after the value, in the value's ink (`text-disabled` when disabled).
Dropdown *MakeDropdown(lv_obj_t *parent, const char *name, const char *const *options, int count, int selected = 0);
int DropdownSelected(const Dropdown *dropdown);
// Shows option `index` with no event: the value changed under the field, not by the person.
void SetDropdownSelected(Dropdown *dropdown, int index);
// A choice committed: shows option `index` and sends LV_EVENT_VALUE_CHANGED when it is not the one already chosen. The sheet's
// OK calls this; choosing the chosen option sends nothing.
void ChooseDropdown(Dropdown *dropdown, int index);
// Disabled is the one disabled look — the light dither, no edge, its words and chevron `text-disabled` — never reached by the
// buttons, with the reason in words under it; enabled again hides the reason.
void SetDropdownEnabled(Dropdown *dropdown, bool enabled, const char *reason = nullptr);
// What OK or a tap does on the enabled field: `open` runs instead of the select sheet; nullptr does nothing.
void SetDropdownOpener(Dropdown *dropdown, DropdownOpener open, void *user);

}  // namespace widgets
