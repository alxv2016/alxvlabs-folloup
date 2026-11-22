// The text input and the password input: BYTE-90's input components ported to the glass, the field the keyboard types into
// (DESIGN → Components → The kit; STICKY_DESIGN_TOKENS §11). A white frame `spacing.content` × `spacing.target`, a
// `spacing.stroke.default` outline, `rounded.xl`; the typed words in `label` `spacing.md` in from the edge, a placeholder in
// `caption` after a still caret (never blinking), and at the right end an icon button: the clear cross on a text input while it
// holds text, the eye on a password input. The button is the kit's icon button drawn `spacing.xl` square with square corners —
// the frame's `rounded.xl` less the gap `spacing.md` is none, so its corner and its ring share the frame's corner centre — and
// hit across the `spacing.target` square at the frame's right end (DESIGN → Exceptions → The icon button).
//
// A password input masks what is typed with bullets, keeping the last typed character in the clear for
// `motion.password-reveal` (1500 ms) and then masking it; the next typing press, Delete, the eye and the field going away end
// it at once. The eye shows the ACTION a press takes: the open eye while the text is hidden, the slashed eye while it shows.
// The hide when the time is up is no press and counts nothing toward the partial limit.
//
// A value longer than the field holds (19 characters) shows its tail, the caret in view. On a page the frame is a focus stop
// (OK activates it, through the handler the page sets) and its button the next; inside the keyboard the frame is a stop too, with no handler, so OK on it
// does nothing but tick, and the caret always shows (MakeTextInput's `in_keyboard`); the walk's first stop is the field and its button the next, and KeyboardJoinField puts the field and then the button first in the walk. A disabled
// input is the light-dither face with no outline, its words `text-disabled`, its button the same tile; the reason is the page's,
// in words under it. Everything runs under lvgl_display::Lock().

#pragma once

#include "lvgl.h"
#include "widgets/keyboard.h"

namespace widgets {

struct TextInput {
    lv_obj_t *field;   // the frame: place it; LV_EVENT_VALUE_CHANGED is sent to it when the eye or the cross changed the text or the view
    lv_obj_t *button;  // the eye or the cross, the frame's child, in the focus group after the frame; hidden when there is no cross to show
};

// A text input with a placeholder (kept by pointer: a literal); the clear cross shows while it holds text. `width` is the frame's,
// `spacing.content` by default. `in_keyboard`: the field the keyboard types into — the caret always shown.
TextInput MakeTextInput(lv_obj_t *parent, const char *placeholder, int32_t width = 0, bool in_keyboard = false);
// A password input: as above, with the eye in place of the cross; the text starts masked.
TextInput MakePasswordInput(lv_obj_t *parent, const char *placeholder, int32_t width = 0, bool in_keyboard = false);

// What has been typed (the real text, never the mask), valid until the next change.
const char *InputText(const TextInput &input);
// Replaces the text (a page's value; not a typing press, so a password shows no last character).
void SetInputText(const TextInput &input, const char *text);
// A typing press's character, and Delete: the password's last character shows in the clear for the reveal time after the first.
void InputAppend(const TextInput &input, char c);
void InputErase(const TextInput &input);
// Whether a password input shows its text (the eye pressed), and setting it. A text input is always shown.
bool InputShown(const TextInput &input);
void SetInputShown(const TextInput &input, bool shown);
// Disabled is the one disabled look; the page says why in words.
void SetInputEnabled(const TextInput &input, bool enabled);
// OK or a tap on the frame, on a page, calls `activate`: it opens the keyboard.
void SetInputActivateHandler(const TextInput &input, lv_event_cb_t activate, void *user);
// The keyboard's target for this input: `append`, `erase` and `text` over it, valid while the input lives.
KeyboardTarget InputKeyboardTarget(const TextInput &input);

}  // namespace widgets
