#include "widgets/text_input.h"

#include <algorithm>
#include <cstring>

#include "design_tokens.h"
#include "internal/base.h"
#include "internal/icon_button_sized.h"
#include "internal/marks.h"
#include "internal/secure_zero.h"
#include "internal/text_input_state.h"
#include "lvgl_display.h"
#include "widgets/icon_button.h"

namespace widgets {
namespace dt = design_tokens;

namespace {
constexpr int32_t kButton = dt::kSpacingXl;                      // the in-field button, square
constexpr int32_t kButtonRadius = dt::kRoundedXl - dt::kSpacingMd;  // `rounded.xl` less `spacing.md` is 0: a square button
constexpr int32_t kCaretWidth = dt::kSpacingStrokeDefault;
constexpr const char *kBullet = "\xE2\x80\xA2";                  // • U+2022, in the font

using State = detail::InputState;

State *Of(const TextInput &input)
{
    return static_cast<State *>(lv_obj_get_user_data(input.field));
}

bool Enabled(const State *s)
{
    return !lv_obj_has_state(s->frame, LV_STATE_DISABLED);
}

// LVGL's label keeps its own copy of the visible text, and sets a new one by freeing the old, which leaves its bytes in the freed
// block. While the text is masked that copy is bullets; while it is shown, or a typed character is in the clear, it holds the
// password. So before the label's text changes, and before the label goes, the copy is wiped where it stands — through the
// pointer LVGL gives — as far as LVGL allows: the copy it makes inside lv_label_set_text is wiped by the next change, and a block
// the allocator moves is out of reach.
void WipeLabel(State *s)
{
    if (s->label_clear) {
        char *copy = lv_label_get_text(s->typed);
        if (copy != nullptr) {
            detail::SecureZero(copy, std::strlen(copy));
        }
        s->label_clear = false;
    }
}

void StopReveal(State *s)
{
    if (s->timer != nullptr) {
        lv_timer_delete(s->timer);
        s->timer = nullptr;
    }
    s->last_clear = false;
}

// The eye's face is the action: open while the text is hidden, slashed while it shows.
void ShowEyeFace(State *s)
{
    const Glyph glyph = s->shown ? Glyph::kEyeSlashed : Glyph::kEyeOpen;
    lv_image_set_src(lv_obj_get_child(s->button, 0), detail::GlyphImage(glyph));
    lv_obj_set_user_data(s->button, const_cast<char *>(s->shown ? "hides it" : "shows it"));
}

// Everything the text and the state change: the words (the tail, masked or not), the placeholder, the caret, the cross.
void Refresh(State *s)
{
    const size_t n = s->len;
    const size_t cap = static_cast<size_t>(s->capacity);
    const size_t start = n > cap ? n - cap : 0;
    // The visible text is a stack buffer, wiped once the label has copied it: a password is in it while it shows.
    char shown[detail::kInputShownBytes];
    size_t at = 0;
    bool any_clear = false;
    for (size_t i = start; i < n && at + 3 < sizeof shown; ++i) {
        const bool clear = !s->password || s->shown || (s->last_clear && i + 1 == n);
        if (clear) {
            shown[at++] = s->text[i];
            any_clear = true;
        } else {
            std::memcpy(shown + at, kBullet, 3);
            at += 3;
        }
    }
    shown[at] = '\0';
    WipeLabel(s);
    lv_label_set_text(s->typed, shown);
    s->label_clear = any_clear && s->password;  // a plain field's words are no secret
    detail::SecureZero(shown, sizeof shown);
    const bool enabled = Enabled(s);
    lv_obj_set_flag(s->placeholder, LV_OBJ_FLAG_HIDDEN, n > 0);
    lv_obj_set_flag(s->caret, LV_OBJ_FLAG_HIDDEN, !enabled || !(s->in_keyboard || s->focused));
    if (!s->password) {
        lv_obj_set_flag(s->button, LV_OBJ_FLAG_HIDDEN, n == 0 || !enabled);  // the cross only while there is text to clear
    }
}

void OnRevealOver(lv_timer_t *timer)
{
    State *s = static_cast<State *>(lv_timer_get_user_data(timer));
    s->timer = nullptr;  // a one-shot timer frees itself after this call
    s->last_clear = false;
    Refresh(s);  // no press made this frame: it counts nothing toward the partial limit
}

void Changed(State *s)
{
    lv_obj_send_event(s->frame, LV_EVENT_VALUE_CHANGED, nullptr);
}

void OnButtonClicked(lv_event_t *e)
{
    State *s = static_cast<State *>(lv_event_get_user_data(e));
    if (s->password) {
        StopReveal(s);
        s->shown = !s->shown;
        ShowEyeFace(s);
        Refresh(s);
        Changed(s);
        return;
    }
    detail::SecureZero(s->text, sizeof s->text);
    s->len = 0;
    lvgl_display::NoteTyping();  // the cross changes the text as Delete does: its frame counts toward the partial limit
    Refresh(s);  // the cross is hidden now, so the walk moves on to the next stop
    if (lv_obj_has_flag(s->button, LV_OBJ_FLAG_HIDDEN)) {
        lv_group_focus_next(lvgl_display::Group());
    }
    Changed(s);
}

void OnFocusChange(lv_event_t *e)
{
    State *s = static_cast<State *>(lv_event_get_user_data(e));
    s->focused = lv_event_get_code(e) == LV_EVENT_FOCUSED;
    Refresh(s);
}

void OnDelete(lv_event_t *e)
{
    State *s = static_cast<State *>(lv_event_get_user_data(e));
    StopReveal(s);
    WipeLabel(s);  // the children are still alive in the delete event
    detail::SecureZero(s->text, sizeof s->text);
    delete s;
}

TextInput Make(lv_obj_t *parent, const char *placeholder, int32_t width, bool in_keyboard, bool password)
{
    State *s = new State{};
    s->password = password;
    s->in_keyboard = in_keyboard;
    if (width <= 0) {
        width = dt::kSpacingContent;
    }

    // The frame: white, a reserved 3 px edge drawn black, `rounded.xl`; disabled, the light dither with no edge.
    lv_obj_t *frame = lv_button_create(parent);
    lv_obj_remove_style_all(frame);  // touch-exception: no pressed state (DESIGN → Motion → Press)
    lv_obj_set_scrollable(frame, false);
    lv_obj_set_size(frame, width, dt::kSpacingTarget);
    lv_obj_set_style_radius(frame, dt::kRoundedXl, 0);
    lv_obj_set_style_bg_opa(frame, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(frame, ui::Level(dt::kColorBackground), 0);
    detail::Outline(frame, dt::kSpacingStrokeDefault);
    lv_obj_set_style_text_color(frame, ui::Level(dt::kColorTextPrimary), 0);
    detail::DitherWhenDisabled(frame);
    // No state trickle: it passes every state a parent takes to its children, the focus too, and the button inside the frame is a
    // stop of its own with its own ring — a frame focused would ring the button as well. Disabled is passed by hand
    // (SetInputEnabled) to the words and the button.
    lv_obj_set_user_data(frame, s);
    s->frame = frame;

    // The typed area: `spacing.md` in from the edge to the button's hit square, which the frame's own right end holds.
    const int32_t area = width - dt::kSpacingTarget - dt::kSpacingMd;
    lv_obj_t *row = detail::Bare(frame);
    lv_obj_set_size(row, area, ui::LineBox(ui::Type::kLabel));
    lv_obj_align(row, LV_ALIGN_LEFT_MID, dt::kSpacingMd - dt::kSpacingStrokeDefault, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    const lv_font_t *font = ui::Font(ui::Type::kLabel);
    s->capacity = (area - kCaretWidth) / static_cast<int32_t>(lv_font_get_glyph_width(font, '0', 0));
    s->typed = lv_label_create(row);
    lv_label_set_long_mode(s->typed, LV_LABEL_LONG_MODE_CLIP);
    lv_obj_set_style_text_font(s->typed, font, 0);
    lv_obj_set_style_text_color(s->typed, detail::DisabledOnFace(), LV_STATE_DISABLED);
    ui::FitLineBox(s->typed, ui::Type::kLabel);
    lv_obj_set_width(s->typed, LV_SIZE_CONTENT);
    // A still bar after the last character: it never blinks.
    s->caret = detail::Bare(row);
    lv_obj_set_size(s->caret, kCaretWidth, ui::LineBox(ui::Type::kLabel));
    lv_obj_set_style_bg_opa(s->caret, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(s->caret, ui::Level(dt::kColorTextPrimary), 0);
    // The placeholder: `caption`, black, a gap after the caret's place, gone at the first character.
    s->placeholder = lv_label_create(row);
    lv_label_set_text(s->placeholder, placeholder != nullptr ? placeholder : "");
    lv_label_set_long_mode(s->placeholder, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_style_text_font(s->placeholder, ui::Font(ui::Type::kCaption), 0);
    lv_obj_set_style_text_color(s->placeholder, detail::DisabledOnFace(), LV_STATE_DISABLED);
    lv_obj_set_style_margin_left(s->placeholder, dt::kSpacingSm, 0);
    lv_obj_set_style_max_width(s->placeholder, area - kCaretWidth - dt::kSpacingSm, 0);
    ui::FitLineBox(s->placeholder, ui::Type::kCaption);

    // A stop on a page and in the keyboard alike: its ring round the whole frame, and the next move reaches its button. A page sets an
    // activate handler (OK opens the keyboard); in the keyboard there is none, so OK and a tap on the field do the tick and nothing else.
    detail::Focusable(frame);
    lv_obj_add_event_cb(frame, OnFocusChange, LV_EVENT_FOCUSED, s);
    lv_obj_add_event_cb(frame, OnFocusChange, LV_EVENT_DEFOCUSED, s);

    // The button: the icon button, ghost, `spacing.xl` square and square, `spacing.md` from the frame's edge; its hit area is the
    // 64 px square at the frame's right end.
    const IconAction action = password ? IconAction::kEyeOpen : IconAction::kClear;
    s->button = detail::MakeIconButtonSized(frame, action, ButtonVariant::kGhost, kButton, kButtonRadius,
                                            password ? "shows it" : "clears it");
    lv_obj_align(s->button, LV_ALIGN_RIGHT_MID, -(dt::kSpacingMd - dt::kSpacingStrokeDefault), 0);
    lv_obj_set_ext_click_area(s->button, (dt::kSpacingTarget - kButton) / 2);  // the hit is the spacing.target square
    lv_obj_add_event_cb(s->button, OnButtonClicked, LV_EVENT_CLICKED, s);
    detail::LetRingOut(frame);
    lv_obj_add_event_cb(frame, OnDelete, LV_EVENT_DELETE, s);
    Refresh(s);
    return TextInput{frame, s->button};
}

// The keyboard's callbacks over a state.
void TargetAppend(char c, void *user)
{
    InputAppend(TextInput{static_cast<State *>(user)->frame, nullptr}, c);
}
void TargetErase(void *user)
{
    InputErase(TextInput{static_cast<State *>(user)->frame, nullptr});
}
const char *TargetText(void *user)
{
    return static_cast<State *>(user)->text;
}
}  // namespace

TextInput MakeTextInput(lv_obj_t *parent, const char *placeholder, int32_t width, bool in_keyboard)
{
    return Make(parent, placeholder, width, in_keyboard, false);
}

TextInput MakePasswordInput(lv_obj_t *parent, const char *placeholder, int32_t width, bool in_keyboard)
{
    return Make(parent, placeholder, width, in_keyboard, true);
}

const char *InputText(const TextInput &input)
{
    return Of(input)->text;
}

void SetInputText(const TextInput &input, const char *text)
{
    State *s = Of(input);
    StopReveal(s);
    detail::SecureZero(s->text, sizeof s->text);
    s->len = 0;
    if (text != nullptr) {
        s->len = std::min(std::strlen(text), detail::kInputMaxText);
        std::memcpy(s->text, text, s->len);
    }
    Refresh(s);
}

void InputAppend(const TextInput &input, char c)
{
    State *s = Of(input);
    if (s->len >= detail::kInputMaxText) {
        return;  // full: the press ticked and types nothing
    }
    s->text[s->len++] = c;
    if (s->password && !s->shown) {
        // The new character is the one in the clear, and the time starts again.
        if (s->timer != nullptr) {
            lv_timer_delete(s->timer);
        }
        s->timer = lv_timer_create(OnRevealOver, dt::kMotionPasswordRevealDelayMs, s);
        lv_timer_set_repeat_count(s->timer, 1);
        s->last_clear = true;
    }
    Refresh(s);
}

void InputErase(const TextInput &input)
{
    State *s = Of(input);
    if (s->len > 0) {
        --s->len;
        detail::SecureZero(&s->text[s->len], 1);  // the freed byte is wiped
    }
    StopReveal(s);
    Refresh(s);
}

bool InputShown(const TextInput &input)
{
    State *s = Of(input);
    return !s->password || s->shown;
}

void SetInputShown(const TextInput &input, bool shown)
{
    State *s = Of(input);
    if (!s->password) {
        return;
    }
    StopReveal(s);
    s->shown = shown;
    ShowEyeFace(s);
    Refresh(s);
}

void SetInputEnabled(const TextInput &input, bool enabled)
{
    State *s = Of(input);
    lv_obj_set_state(s->frame, LV_STATE_DISABLED, !enabled);
    lv_obj_set_state(s->button, LV_STATE_DISABLED, !enabled);
    lv_obj_set_state(s->typed, LV_STATE_DISABLED, !enabled);        // the words' own disabled colour: not passed down
    lv_obj_set_state(s->placeholder, LV_STATE_DISABLED, !enabled);
    if (!enabled) {
        StopReveal(s);
    }
    Refresh(s);
}

void SetInputActivateHandler(const TextInput &input, lv_event_cb_t activate, void *user)
{
    lv_obj_add_event_cb(input.field, activate, LV_EVENT_CLICKED, user);
}

KeyboardTarget InputKeyboardTarget(const TextInput &input)
{
    return KeyboardTarget{TargetAppend, TargetErase, TargetText, Of(input)};
}

}  // namespace widgets
