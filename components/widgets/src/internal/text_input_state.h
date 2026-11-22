// What a text input or password input keeps, in one struct hung on its frame's user data (text_input.cpp). Private to the widgets
// component; tests/host reads it to see that a typed password is wiped.

#pragma once

#include <cstddef>

#include "lvgl.h"

namespace widgets::detail {

// The most characters a field holds. The text is a fixed buffer, never a std::string: a string that grows reallocates and leaves its
// old buffer, the password in it, unwiped; a buffer that never moves is wiped where it stands (SECRETS §5). The
// keyboard's `max_length` is under it.
inline constexpr size_t kInputMaxText = 64;
// The visible text as the field draws it: a character's UTF-8 is at most a bullet's three bytes.
inline constexpr size_t kInputShownBytes = kInputMaxText * 3 + 1;

struct InputState {
    bool password = false;
    bool shown = false;         // a password's eye pressed: every character in the clear
    bool last_clear = false;    // the last typed character is in the clear until the reveal time is up
    bool in_keyboard = false;
    bool focused = false;
    bool label_clear = false;   // the typed label holds clear text in LVGL's own copy of it
    size_t len = 0;
    int32_t capacity = 0;       // characters the typed area holds beside the caret
    lv_obj_t *frame = nullptr;
    lv_obj_t *typed = nullptr;
    lv_obj_t *caret = nullptr;
    lv_obj_t *placeholder = nullptr;
    lv_obj_t *button = nullptr;
    lv_timer_t *timer = nullptr;
    char text[kInputMaxText + 1] = {};  // the real text, nul-terminated; the bytes past `len` are always zero
};

}  // namespace widgets::detail
