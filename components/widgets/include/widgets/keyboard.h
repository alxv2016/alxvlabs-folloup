// The keyboard: a grid of six 64 px keys a row — letters in the order of the alphabet, digits with common punctuation, the rest
// of the punctuation, Shift, Delete, Space — under a slot for the text field, with a count line and a footer holding
// Cancel and one action. BYTE-90's keyboard ported to the glass's own size (DESIGN → Components → The kit).
//
// It walks on three keys, key by key: UP and DOWN step through the stops in the order of the glass — the field, its button, every key
// row by row from the left, Cancel, the action — and wrap; OK presses the focused key, and holding OK cancels (DESIGN → Focus and
// input). Each key takes the one focus ring, `spacing.stroke.focus`; there is no row level and no editing state. Every key types once
// however long it is held; the field's caret is a still bar at the end, so a mistake is Delete. A touch on a key types it on release; a
// finger that slides off cancels it.
//
// The text field is not the keyboard's: BYTE-90's input and password input are ported as their own widgets (text_input.h), with
// the eye and the clear cross inside the field. The keyboard keeps no text — it calls the caller's `KeyboardTarget` to append a
// character, delete the last one and read what has been typed (text_input.h's InputKeyboardTarget is one), and leaves a slot at
// the field's place for the field. The field and its button are the walk's first two stops (KeyboardJoinField).
//
// Built today as a screen of its own that a page opens and closes, as the Display opens its sub-screens; it becomes an
// overlay when the interaction framework lands, with the same configuration and the same callbacks.

#pragma once

#include <cstdint>

#include "lvgl.h"

namespace widgets {

// Where the typed text lives: the field component, when it lands. `append` takes a typed character, `erase` takes the last one
// away, `text` is what has been typed (ASCII, nul-terminated, valid until the next call). All run under the lock.
struct KeyboardTarget {
    void (*append)(char c, void *user);
    void (*erase)(void *user);
    const char *(*text)(void *user);
    void *user;
};

// The keyboard finished: `accepted` is the action, or false for Cancel or a hold of OK; what was typed is in the
// target. Runs under the lock, from an event of the keyboard's own screen: the caller loads the screen to go back to, then
// deletes this one.
using KeyboardDone = void (*)(bool accepted, void *user);

struct KeyboardConfig {
    const char *title;         // the status bar's: what is being typed for ("Join HomeNet")
    KeyboardTarget target;
    int min_length;            // 0: none. The action stays disabled until the text has this many characters, and the count line says so
    int max_length;            // characters the target takes; a key past it ticks and types nothing
    const char *action;        // the footer's one action, named for what it does: "Join", "Done"
    KeyboardDone done;
    void *user;
};

struct Keyboard {
    lv_obj_t *screen;      // the whole screen — status bar, field slot, key grid, footer; the caller loads it
    lv_obj_t *field_slot;  // `spacing.content` wide and `spacing.target` high at the field's place, empty, for the field component
};

// Builds the keyboard's screen and sets the partial refresh to count only the presses that type, and the page switches, while it is open
// (STICKY_DESIGN_TOKENS §5). Call lvgl_display::ClearGroup() first: its stops join the group in focus order — the keys row by row, then
// Cancel and the action — and focus starts on the first key; a field joins the walk through KeyboardJoinField. The
// caller sets KeyboardEscape as the back handler for as long as it shows, so holding OK cancels. After the
// text changes any other way — the field's clear cross — the caller calls KeyboardTextChanged.
Keyboard MakeKeyboardScreen(const KeyboardConfig &config);
// Sets the target after the screen is made, for a field that is made in `field_slot`, which the screen makes: the config's target
// may be empty (null functions) until then, and the keyboard types nothing and reads "" in the meantime.
void SetKeyboardTarget(const Keyboard &keyboard, const KeyboardTarget &target);
// Puts the field and its button — already made in `field_slot` — first in the walk, before the keys, where they stand on the glass:
// the field, the button, every key in reading order, Cancel, the action, and round again. The field is a stop with its own ring and
// OK on it does nothing but tick; its button has its own ring on the next move, never both at once. The keyboard shows no hint
// line: a keyboard is known. `button` may be null. Focus stays on the first key, where the keyboard opens. Call after MakeKeyboardScreen
// and SetKeyboardTarget; a second call makes the walk again, whole, and changes nothing.
void KeyboardJoinField(const Keyboard &keyboard, lv_obj_t *field, lv_obj_t *button);
// The text changed behind the keyboard's back: the count line and the action's disabled look are read again.
void KeyboardTextChanged(lv_obj_t *screen);
// Hold OK: cancels the keyboard. A function of no arguments, for
// lvgl_display::SetBackHandler. Does nothing when no keyboard shows.
void KeyboardEscape();

}  // namespace widgets
