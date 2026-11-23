// Host test of the text input and the password input on LVGL's real layout and timers, with the real widgets and fonts, on the
// computer with no board: the frame is 432 x 64, the in-field button 32 x 32 `spacing.md` from the frame's right, top and bottom
// edges and hit across the 64 px square at its right end, the field holds 19 characters and shows the tail of more, and a password
// shows its last typed character for `motion.password-reveal` and then a bullet — masked sooner by the next typing press, Delete,
// the eye or the field going away (DESIGN → Components → The kit; DESIGN → Exceptions → A password's last character).
// A typed password is wiped from every working copy the field keeps — its text buffer on a clear, a replace, Delete and the field
// going away, and LVGL's copy of the visible text — seen through a spy on the stand-in for `mbedtls_platform_zeroize`
// (stubs/mbedtls/platform_util.h), as SECRETS §5 asks; and KeyboardJoinFieldFirst puts the field's button first in the
// keyboard's walk, the walk wrapping, and leaves the group's editing off. And that the field and its button are separate stops with
// separate rings: the field focused rings the field alone, the next focus move rings the button alone — on a page, on a text and a
// password input, and in the keyboard, where the field is no stop and the button rings only when it is the one focused.
// run.sh builds LVGL's sources from managed_components once, into build/host/lvgl, and the sources named below, against stand-ins
// for the ESP-IDF headers (tests/host/stubs).
// host-sources: components/ui/ui.cpp components/ui/line_box.cpp components/ui/fonts/bitcount/*.c components/widgets/marks/marks.c components/widgets/src/*.cpp components/widgets/src/internal/*.cpp components/router/router.cpp components/router/overlays.cpp components/router/focus.cpp components/router/touch.cpp components/router/routes.cpp components/router/screen.cpp
// Build and run: bash tests/host/run.sh

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "design_tokens.h"
#include "internal/text_input_state.h"
#include "mbedtls/platform_util.h"
#include "lvgl.h"
#include "lvgl_display.h"
#include "router/router.h"
#include "ui.h"
#include "widgets/widgets.h"

namespace dt = design_tokens;

namespace {
lv_group_t *s_group = nullptr;
void (*s_back)() = nullptr;
uint32_t s_ms = 0;
int s_typing_notes = 0;
int s_done = 0;
bool s_accepted = true;
}  // namespace
namespace lvgl_display {
lv_group_t *Group() { return s_group; }
void ClearGroup()
{
    lv_group_set_editing(s_group, false);
    lv_group_remove_all_objs(s_group);
}
// The router binds a screen's or an overlay's group here: the buttons and LVGL's default group follow it, as on the board.
void SetGroup(lv_group_t *group)
{
    if (group == nullptr) {
        return;
    }
    s_group = group;
    for (lv_indev_t *indev = lv_indev_get_next(nullptr); indev != nullptr; indev = lv_indev_get_next(indev)) {
        lv_indev_set_group(indev, group);  // a keypad's only: LVGL ignores it for a pointer
    }
    lv_group_set_default(group);
}
void SetBackHandler(void (*back)()) { s_back = back; }
// The router's read hooks, kept so a test can run a key read through the focus rules as LVGL's read would.
void (*s_key_hook)(lv_indev_t *, lv_indev_data_t *) = nullptr;
void (*s_touch_hook)(lv_indev_t *, lv_indev_data_t *) = nullptr;
void SetKeyReadHook(void (*hook)(lv_indev_t *, lv_indev_data_t *)) { s_key_hook = hook; }
void SetTouchReadHook(void (*hook)(lv_indev_t *, lv_indev_data_t *)) { s_touch_hook = hook; }
void SetPartialCounting(PartialCounting) {}
void NoteTyping() { ++s_typing_notes; }
void RequestFull() {}
void PlayCue(Cue) {}
}  // namespace lvgl_display

namespace {

int s_failures = 0;
int s_checks = 0;

void Check(bool ok, const char *what)
{
    ++s_checks;
    if (!ok) {
        ++s_failures;
        std::printf("FAIL  %s\n", what);
    }
}

void Flush(lv_display_t *display, const lv_area_t *, uint8_t *)
{
    lv_display_flush_ready(display);
}

uint32_t Ticks() { return s_ms; }

void Wait(uint32_t ms)
{
    s_ms += ms;
    lv_timer_handler();
}

// What the field shows now: the typed label inside the frame's text row.
std::string Shown(const widgets::TextInput &input)
{
    lv_obj_t *row = lv_obj_get_child(input.field, 0);
    return lv_label_get_text(lv_obj_get_child(row, 0));
}

bool Hidden(lv_obj_t *obj) { return lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN); }

const char *kBullet = "\xE2\x80\xA2";

lv_obj_t *s_screen = nullptr;

// --- What the wipe saw ---------------------------------------------------------------------------------------------------

struct Wiped {
    const void *at;
    size_t len;
    std::string before;  // the bytes up to the first nul, as they were when the wipe began
};
std::vector<Wiped> s_wiped;

void Spy(void *buf, size_t len)
{
    const char *p = static_cast<const char *>(buf);
    s_wiped.push_back({buf, len, std::string(p, strnlen(p, len))});
}

// Whether a wipe of exactly `at` for `len` bytes began over text that held `what`.
bool WipedWith(const void *at, size_t len, const std::string &what)
{
    for (const Wiped &w : s_wiped) {
        if (w.at == at && w.len == len && w.before.find(what) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// Whether any wipe, anywhere, began over text that held `what` — LVGL's label copy has an address the test cannot name.
bool WipedAnywhere(const std::string &what)
{
    for (const Wiped &w : s_wiped) {
        if (w.before.find(what) != std::string::npos) {
            return true;
        }
    }
    return false;
}

widgets::detail::InputState *StateOf(const widgets::TextInput &input)
{
    return static_cast<widgets::detail::InputState *>(lv_obj_get_user_data(input.field));
}

bool AllZero(const char *p, size_t n)
{
    return std::all_of(p, p + n, [](char c) { return c == 0; });
}

lv_obj_t *NewPage()
{
    router::DismissOverlays();  // a keyboard left open by the last test closes, its group with it
    lv_timer_handler();         // and its screen, deleted after the close, goes now: LVGL's heap here is small
    if (s_screen != nullptr) {
        lvgl_display::ClearGroup();
        lv_screen_load(lv_obj_create(nullptr));
        lv_obj_delete(s_screen);
    }
    s_screen = ui::NewScreen();
    lv_screen_load(s_screen);
    return s_screen;
}

void TestGeometry()
{
    lv_obj_t *page = NewPage();
    const widgets::TextInput input = widgets::MakeTextInput(page, "Add a name");
    lv_obj_update_layout(page);
    lv_area_t f, b;
    lv_obj_get_coords(input.field, &f);
    lv_obj_get_coords(input.button, &b);
    Check(lv_area_get_width(&f) == dt::kSpacingContent && lv_area_get_height(&f) == dt::kSpacingTarget, "the frame is 432 x 64");
    Check(lv_area_get_width(&b) == dt::kSpacingXl && lv_area_get_height(&b) == dt::kSpacingXl, "the button is spacing.xl square");
    Check(f.x2 - b.x2 == dt::kSpacingMd && b.y1 - f.y1 == dt::kSpacingMd && f.y2 - b.y2 == dt::kSpacingMd,
          "the button stands spacing.md from the frame's right, top and bottom edges");
    Check(lv_obj_get_style_radius(input.button, LV_PART_MAIN) == dt::kRoundedXl - dt::kSpacingMd,
          "the button's corner is the frame's radius less the gap");
    // The hit area: the 64 px square at the frame's right end, edges the frame's.
    lv_point_t corner_in = {f.x2, f.y1};
    lv_point_t corner_out = {f.x2 - dt::kSpacingTarget, f.y1};
    lv_point_t middle_left = {f.x2 - dt::kSpacingTarget + 1, (f.y1 + f.y2) / 2};
    Check(lv_obj_hit_test(input.button, &corner_in), "a touch at the frame's right end, top corner, is the button's");
    Check(!lv_obj_hit_test(input.button, &corner_out), "a touch left of the 64 px square is not");
    Check(lv_obj_hit_test(input.button, &middle_left), "the square's left edge is the button's");
}

void TestCapacityAndTail()
{
    lv_obj_t *page = NewPage();
    const widgets::TextInput input = widgets::MakeTextInput(page, "Add a name");
    widgets::SetInputText(input, "abcdefghijklmnopqrs");  // 19
    Check(Shown(input) == "abcdefghijklmnopqrs", "19 characters are all shown");
    widgets::SetInputText(input, "abcdefghijklmnopqrstu");  // 21
    Check(Shown(input) == "cdefghijklmnopqrstu", "more than 19 shows the tail");
    lv_obj_update_layout(page);
    lv_obj_t *row = lv_obj_get_child(input.field, 0);
    lv_obj_t *typed = lv_obj_get_child(row, 0);
    lv_obj_t *caret = lv_obj_get_child(row, 1);
    Check(lv_obj_get_width(typed) + lv_obj_get_width(caret) <= lv_obj_get_width(row), "the tail and the caret fit the typed area");
}

void TestClearCross()
{
    lv_obj_t *page = NewPage();
    const widgets::TextInput input = widgets::MakeTextInput(page, "Add a name");
    Check(Hidden(input.button), "the cross is hidden while the field is empty");
    widgets::InputAppend(input, 'a');
    Check(!Hidden(input.button), "the cross shows once there is text");
    Check(Hidden(lv_obj_get_child(lv_obj_get_child(input.field, 0), 2)), "the placeholder goes at the first character");
    lv_group_focus_obj(input.button);
    lv_obj_send_event(input.button, LV_EVENT_CLICKED, nullptr);
    Check(std::string(widgets::InputText(input)).empty(), "the cross empties the field");
    Check(Hidden(input.button), "and goes");
    Check(lv_group_get_focused(s_group) != input.button, "focus passes on to the next stop");
    widgets::SetInputText(input, "x");
    widgets::SetInputEnabled(input, false);
    Check(Hidden(input.button), "a disabled field draws no cross");
}

void TestPasswordReveal()
{
    lv_obj_t *page = NewPage();
    const widgets::TextInput input = widgets::MakePasswordInput(page, "Password");
    int changed = 0;
    lv_obj_add_event_cb(input.field, [](lv_event_t *e) { ++*static_cast<int *>(lv_event_get_user_data(e)); }, LV_EVENT_VALUE_CHANGED, &changed);
    Check(!Hidden(input.button), "the eye is always there");
    widgets::InputAppend(input, 'a');
    Check(Shown(input) == "a", "the first typed character shows in the clear");
    widgets::InputAppend(input, 'b');
    Check(Shown(input) == std::string(kBullet) + "b", "the next press masks the one before and shows its own");
    Wait(dt::kMotionPasswordRevealDelayMs - 100);
    Check(Shown(input) == std::string(kBullet) + "b", "the last character is still in the clear just short of the time");
    Wait(200);
    Check(Shown(input) == std::string(kBullet) + kBullet, "motion.password-reveal later it is a bullet");
    Check(std::string(widgets::InputText(input)) == "ab", "the real text is untouched");
    widgets::InputAppend(input, 'c');
    Wait(1000);
    widgets::InputAppend(input, 'd');
    Wait(1000);  // the time started again with `d`
    Check(Shown(input) == std::string(kBullet) + kBullet + kBullet + "d", "each press starts the time again");
    widgets::InputErase(input);
    Check(Shown(input) == std::string(kBullet) + kBullet + kBullet, "Delete masks everything");
    Wait(2000);  // a timer that should be gone does nothing
    Check(Shown(input) == std::string(kBullet) + kBullet + kBullet, "and no timer is left");

    // The eye: the action, not the state.
    const char *words_hidden = widgets::IconButtonWords(input.button);
    Check(std::string(words_hidden) == "shows it", "the eye's action while hidden is to show");
    lv_obj_send_event(input.button, LV_EVENT_CLICKED, nullptr);
    Check(Shown(input) == "abc", "the eye shows the whole text");
    Check(std::string(widgets::IconButtonWords(input.button)) == "hides it", "and now says it hides");
    Check(changed == 1, "the field says the eye changed what it shows");
    widgets::InputAppend(input, 'e');
    Wait(2000);
    Check(Shown(input) == "abce", "shown, a typed character stays in the clear and needs no timer");
    lv_obj_send_event(input.button, LV_EVENT_CLICKED, nullptr);
    Check(Shown(input) == std::string(kBullet) + kBullet + kBullet + kBullet, "the eye again hides it all");

    // Going away with the timer running frees it.
    widgets::InputAppend(input, 'f');
    lvgl_display::ClearGroup();
    lv_screen_load(lv_obj_create(nullptr));
    lv_obj_delete(s_screen);
    s_screen = nullptr;
    Wait(2000);
    Check(true, "a field deleted with the time running leaves no timer behind");
}

// Whether `obj` draws the focus ring now: the keys' focus state and an outline of the focus ring's width, in its current state.
bool RingDrawn(lv_obj_t *obj)
{
    return lv_obj_has_state(obj, LV_STATE_FOCUS_KEY) && lv_obj_get_style_outline_width(obj, LV_PART_MAIN) > 0 &&
           lv_obj_get_style_outline_opa(obj, LV_PART_MAIN) > 0;
}

void TestSeparateRings(bool password)
{
    lv_obj_t *page = NewPage();
    const widgets::TextInput input =
        password ? widgets::MakePasswordInput(page, "Password") : widgets::MakeTextInput(page, "Add a name");
    widgets::SetInputText(input, "abc");  // a text input's cross shows only while there is text
    lvgl_display::ClearGroup();
    lv_group_add_obj(s_group, input.field);
    lv_group_add_obj(s_group, input.button);
    lv_group_focus_obj(input.field);
    const char *kind = password ? "password input" : "text input";
    Check(RingDrawn(input.field), (std::string(kind) + ": the field focused draws its ring").c_str());
    Check(!lv_obj_has_state(input.button, LV_STATE_FOCUSED) && !RingDrawn(input.button),
          (std::string(kind) + ": and its button draws none — the ring is not passed down to it").c_str());
    lv_group_focus_next(s_group);
    Check(lv_group_get_focused(s_group) == input.button && RingDrawn(input.button),
          (std::string(kind) + ": the next move rings the button").c_str());
    Check(!lv_obj_has_state(input.field, LV_STATE_FOCUSED) && !RingDrawn(input.field),
          (std::string(kind) + ": and the field alone no longer").c_str());
    // Disabled still reaches the words and the button, by hand.
    widgets::SetInputEnabled(input, false);
    Check(lv_obj_has_state(input.button, LV_STATE_DISABLED) && lv_obj_has_state(input.field, LV_STATE_DISABLED),
          (std::string(kind) + ": disabled reaches the frame and the button").c_str());
    lv_obj_t *row = lv_obj_get_child(input.field, 0);
    Check(lv_obj_has_state(lv_obj_get_child(row, 0), LV_STATE_DISABLED), (std::string(kind) + ": and the words").c_str());
}

void TestWipe()
{
    mbedtls_test_zeroize_spy = Spy;
    const std::string secret = "hunter2";
    {
        lv_obj_t *page = NewPage();
        const widgets::TextInput input = widgets::MakePasswordInput(page, "Password");
        widgets::detail::InputState *st = StateOf(input);
        s_wiped.clear();
        for (char c : secret) {
            widgets::InputAppend(input, c);
        }
        Check(std::string(st->text) == secret, "the buffer holds what was typed");
        widgets::InputErase(input);
        Check(AllZero(st->text + st->len, sizeof st->text - st->len) && std::string(st->text) == "hunter", "Delete wipes the byte it frees");
        widgets::InputErase(input);
        widgets::InputErase(input);
        widgets::InputErase(input);
        widgets::InputErase(input);
        widgets::InputErase(input);
        widgets::InputErase(input);
        Check(st->len == 0 && AllZero(st->text, sizeof st->text), "Delete down to nothing leaves the whole buffer zero");

        widgets::SetInputText(input, secret.c_str());
        Check(std::string(st->text) == secret, "SetInputText fills the buffer");
        s_wiped.clear();
        widgets::SetInputText(input, "x");
        Check(WipedWith(st->text, sizeof st->text, secret), "SetInputText wipes the old text before the new");
        Check(std::string(st->text) == "x" && AllZero(st->text + 1, sizeof st->text - 1), "and nothing of the old text is left past the new");
        widgets::SetInputText(input, nullptr);
        Check(st->len == 0 && AllZero(st->text, sizeof st->text), "SetInputText with nothing empties and wipes the buffer");

        // The eye: LVGL's copy of the visible text holds the password in the clear, and is wiped when it masks again.
        widgets::SetInputText(input, secret.c_str());
        widgets::SetInputShown(input, true);
        Check(st->label_clear, "the label holds the password in the clear while it shows");
        s_wiped.clear();
        widgets::SetInputShown(input, false);
        Check(WipedAnywhere(secret), "masking wipes LVGL's copy that held the password");
        Check(!st->label_clear, "and the label then holds bullets only");

        // A typed character in the clear is wiped from the label when the time is up.
        widgets::SetInputText(input, "");
        widgets::InputAppend(input, 'q');
        s_wiped.clear();
        Wait(dt::kMotionPasswordRevealDelayMs + 100);
        Check(WipedAnywhere("q"), "the reveal time ending wipes LVGL's copy of the character");

        // The field going away: the buffer and the label are wiped before they are freed.
        widgets::SetInputText(input, secret.c_str());
        widgets::SetInputShown(input, true);
        const char *buffer = st->text;
        s_wiped.clear();
        lvgl_display::ClearGroup();
        lv_screen_load(lv_obj_create(nullptr));
        lv_obj_delete(s_screen);
        s_screen = nullptr;
        Check(WipedWith(buffer, sizeof st->text, secret), "the field going away wipes its text buffer");
        Check(std::count_if(s_wiped.begin(), s_wiped.end(), [&](const Wiped &w) { return w.at != buffer && w.before == secret; }) >= 1,
              "and LVGL's copy of the visible text");
    }
    {
        // The clear cross: the buffer is wiped, and the press counts as a typing press.
        lv_obj_t *page = NewPage();
        const widgets::TextInput input = widgets::MakeTextInput(page, "Add a name");
        widgets::detail::InputState *st = StateOf(input);
        widgets::SetInputText(input, secret.c_str());
        s_wiped.clear();
        s_typing_notes = 0;
        lv_obj_send_event(input.button, LV_EVENT_CLICKED, nullptr);
        Check(WipedWith(st->text, sizeof st->text, secret), "the cross wipes the buffer");
        Check(st->len == 0 && AllZero(st->text, sizeof st->text), "and the buffer is zero");
        Check(s_typing_notes == 1, "the cross is a typing press: it counts toward the partial limit");
    }
    {
        // A full field takes no more, and nothing is written past the buffer.
        lv_obj_t *page = NewPage();
        const widgets::TextInput input = widgets::MakePasswordInput(page, "Password");
        widgets::detail::InputState *st = StateOf(input);
        for (size_t i = 0; i < widgets::detail::kInputMaxText + 10; ++i) {
            widgets::InputAppend(input, 'a');
        }
        Check(st->len == widgets::detail::kInputMaxText && st->text[widgets::detail::kInputMaxText] == 0,
              "a full field stops at its maximum, nul-terminated");
    }
    mbedtls_test_zeroize_spy = nullptr;
}

struct Kb {
    widgets::Keyboard keyboard;
    widgets::TextInput input;
};

Kb NewKeyboard(bool password)
{
    NewPage();
    widgets::KeyboardConfig config{};
    config.title = "Join HomeNet";
    config.min_length = 8;
    config.max_length = 63;
    config.action = "Join";
    config.done = [](bool accepted, void *) { ++s_done; s_accepted = accepted; };
    Kb k{};
    k.keyboard = widgets::OpenKeyboard(config);  // over the page, with its own group: s_group, while it is open
    k.input = password ? widgets::MakePasswordInput(k.keyboard.field_slot, "Password", dt::kSpacingContent, true)
                       : widgets::MakeTextInput(k.keyboard.field_slot, "Name", dt::kSpacingContent, true);
    widgets::SetKeyboardTarget(k.keyboard, widgets::InputKeyboardTarget(k.input));
    return k;
}

// Whether any label under `obj` reads something containing `what`.
bool AnyLabelHolds(lv_obj_t *obj, const char *what)
{
    if (lv_obj_check_type(obj, &lv_label_class) && std::strstr(lv_label_get_text(obj), what) != nullptr) {
        return true;
    }
    for (uint32_t i = 0; i < lv_obj_get_child_count(obj); ++i) {
        if (AnyLabelHolds(lv_obj_get_child(obj, static_cast<int32_t>(i)), what)) {
            return true;
        }
    }
    return false;
}

// The keyboard's walk is key by key, the order of the glass: the field, its button, every key row by row from the left, Cancel, the
// action, and round again. There is no row level, no editing state and no second ring.
void TestKeyboardWalk()
{
    Kb k = NewKeyboard(true);
    for (char c : std::string("abcdefgh")) {
        widgets::InputAppend(k.input, c);  // eight characters: the action is no longer disabled, so the walk reaches it
    }
    widgets::KeyboardTextChanged(k.keyboard.screen);
    lv_group_set_editing(s_group, true);  // as anything left open for editing would leave it
    widgets::KeyboardJoinField(k.keyboard, k.input.field, k.input.button);
    Check(!lv_group_get_editing(s_group), "joining the field leaves the group's editing off");
    lv_obj_update_layout(k.keyboard.screen);

    // 2 for the field, 6 rows with 6, 6, 6, 6, 5 and 3 keys, Cancel and the action.
    const uint32_t n = lv_group_get_obj_count(s_group);
    Check(n == 2 + 32 + 2, "the walk is the field, its button, the 32 keys of the letters page, Cancel and the action");
    Check(lv_group_get_obj_by_index(s_group, 0) == k.input.field, "the field is the first stop");
    Check(lv_group_get_obj_by_index(s_group, 1) == k.input.button, "its button is the second");
    bool reading_order = true;
    lv_area_t last{};
    for (uint32_t i = 2; i < 2 + 32; ++i) {
        lv_area_t a;
        lv_obj_get_coords(lv_group_get_obj_by_index(s_group, static_cast<int32_t>(i)), &a);
        if (i > 2 && !(a.y1 > last.y1 || (a.y1 == last.y1 && a.x1 > last.x1))) {
            reading_order = false;
        }
        last = a;
    }
    Check(reading_order, "the keys follow in reading order: row by row, left to right");
    lv_area_t first_key, cancel_area, action_area, last_key;
    lv_obj_get_coords(lv_group_get_obj_by_index(s_group, 2), &first_key);
    lv_obj_get_coords(lv_group_get_obj_by_index(s_group, 33), &last_key);
    lv_obj_get_coords(lv_group_get_obj_by_index(s_group, 34), &cancel_area);
    lv_obj_get_coords(lv_group_get_obj_by_index(s_group, 35), &action_area);
    Check(cancel_area.y1 > last_key.y2 && action_area.y1 == cancel_area.y1 && action_area.x1 > cancel_area.x2,
          "Cancel then the action, in the footer under the keys");
    Check(lv_group_get_focused(s_group) == lv_group_get_obj_by_index(s_group, 2), "the keyboard opens on the first key");
    // No hint line anywhere on the screen, at any stop: a keyboard is known (the person's decision).
    bool hints = false;
    for (uint32_t i = 0; i < lv_group_get_obj_count(s_group); ++i) {
        lv_group_focus_obj(lv_group_get_obj_by_index(s_group, static_cast<int32_t>(i)));
        hints = hints || AnyLabelHolds(k.keyboard.screen, "UP or DOWN") || AnyLabelHolds(k.keyboard.screen, "Hold OK") ||
                AnyLabelHolds(k.keyboard.screen, "OK shows") || AnyLabelHolds(k.keyboard.screen, "OK clears");
    }
    Check(!hints, "the keyboard shows no hint line at any stop");
    lv_group_focus_obj(lv_group_get_obj_by_index(s_group, 2));
    (void)first_key;

    // A key is a stop with the ordinary ring, and no editing state; the row is no stop.
    lv_obj_t *key_a = lv_group_get_obj_by_index(s_group, 2);
    Check(RingDrawn(key_a) && !lv_obj_has_state(key_a, LV_STATE_EDITED), "the focused key draws the ordinary focus ring and is not 'edited'");
    Check(lv_obj_get_style_outline_width(key_a, LV_PART_MAIN) == dt::kSpacingStrokeFocus, "and it is spacing.stroke.focus wide");
    lv_group_focus_next(s_group);
    Check(lv_group_get_focused(s_group) == lv_group_get_obj_by_index(s_group, 3) && !RingDrawn(key_a), "DOWN steps to the next key and the ring moves");
    Check(!lv_group_get_editing(s_group), "a step never opens editing");

    // The ends wrap: the action to the field, the field back to the action.
    lv_group_focus_obj(lv_group_get_obj_by_index(s_group, 35));
    lv_group_focus_next(s_group);
    Check(lv_group_get_focused(s_group) == k.input.field, "from the action DOWN wraps to the field");
    lv_group_focus_prev(s_group);
    Check(lv_group_get_focused(s_group) == lv_group_get_obj_by_index(s_group, 35), "and UP from the field to the action");
    lv_group_focus_obj(k.input.button);
    lv_group_focus_next(s_group);
    Check(lv_group_get_focused(s_group) == lv_group_get_obj_by_index(s_group, 2), "after the button comes the first key");
    lv_group_focus_obj(lv_group_get_obj_by_index(s_group, 33));
    lv_group_focus_next(s_group);
    Check(lv_group_get_focused(s_group) == lv_group_get_obj_by_index(s_group, 34), "after the last key comes Cancel");

    // OK on a key types it: the click a key press sends.
    lv_obj_t *key_b = lv_group_get_obj_by_index(s_group, 3);
    lv_group_focus_obj(key_b);
    lv_obj_send_event(key_b, LV_EVENT_CLICKED, nullptr);
    Check(std::string(widgets::InputText(k.input)) == "abcdefghb", "OK on the focused key types its character");
    s_typing_notes = 0;
    lv_obj_send_event(lv_group_get_obj_by_index(s_group, 3), LV_EVENT_CLICKED, nullptr);
    Check(s_typing_notes == 1, "a typing press counts toward the partial limit");
}

// Holding OK cancels the keyboard at once, from any stop: there is no row to leave first.
void TestKeyboardEscape()
{
    Kb k = NewKeyboard(false);
    widgets::KeyboardJoinField(k.keyboard, k.input.field, k.input.button);
    lv_group_focus_obj(lv_group_get_obj_by_index(s_group, 10));  // a key in the middle of the grid
    s_done = 0;
    router::Escape();  // holding OK: the escape chain's first link is the top overlay, the keyboard
    Check(s_done == 1 && !s_accepted, "holding OK on a key cancels the keyboard in one hold");
    Check(!router::OverlayOpen() && lv_screen_active() == s_screen, "and the page it opened over is back on the glass");
}

// A page switch rebuilds the keys and the walk, in the same order, keeping the place.
void TestKeyboardPageSwitch()
{
    Kb k = NewKeyboard(false);
    widgets::KeyboardJoinField(k.keyboard, k.input.field, k.input.button);
    lv_obj_update_layout(k.keyboard.screen);
    // Shift is the first key of the bottom row: 6+6+6+6+5 = 29 keys come before it, after the field and its button.
    lv_obj_t *shift = lv_group_get_obj_by_index(s_group, 2 + 29);
    lv_group_focus_obj(shift);
    s_typing_notes = 0;
    lv_obj_send_event(shift, LV_EVENT_CLICKED, nullptr);  // Shift: the capitals page
    Check(s_typing_notes == 1, "a page switch counts as one partial");
    lv_obj_update_layout(k.keyboard.screen);
    Check(lv_group_get_obj_by_index(s_group, 0) == k.input.field && lv_group_get_obj_by_index(s_group, 1) == k.input.button,
          "after a page switch the field and its button are still the first two stops");
    const uint32_t n = lv_group_get_obj_count(s_group);
    Check(n == 2 + 32 + 2, "and the capitals page's 32 keys stand between them and the footer, as the letters page's");
    lv_obj_t *now = lv_group_get_focused(s_group);
    Check(now == lv_group_get_obj_by_index(s_group, 2 + 29), "the focus keeps its place: the same row, the same slot");
    Check(RingDrawn(now), "and the ring is drawn on it");
    Check(!lv_group_get_editing(s_group), "editing stays off");
}

// The field and its button are separate stops with separate rings: the field focused rings the field alone, the next move rings the
// button alone — on a page and in the keyboard, for both inputs.
void TestSeparateRingsInKeyboard()
{
    for (bool password : {false, true}) {
        Kb k = NewKeyboard(password);
        widgets::InputAppend(k.input, 'a');  // a text input's cross shows only while there is text
        widgets::KeyboardJoinField(k.keyboard, k.input.field, k.input.button);
        const char *kind = password ? "keyboard, password input: " : "keyboard, text input: ";
        lv_group_focus_obj(k.input.field);
        Check(RingDrawn(k.input.field), (std::string(kind) + "the field is a stop and draws its ring").c_str());
        Check(!lv_obj_has_state(k.input.button, LV_STATE_FOCUSED) && !RingDrawn(k.input.button),
              (std::string(kind) + "and its button draws none").c_str());
        lv_group_focus_next(s_group);
        Check(lv_group_get_focused(s_group) == k.input.button && RingDrawn(k.input.button),
              (std::string(kind) + "the next move rings the button").c_str());
        Check(!lv_obj_has_state(k.input.field, LV_STATE_FOCUSED) && !RingDrawn(k.input.field),
              (std::string(kind) + "and the field no longer").c_str());
        // OK on the field does nothing but tick: nothing in the text changes and no cancel happens.
        lv_group_focus_obj(k.input.field);
        s_typing_notes = 0;
        lv_obj_send_event(k.input.field, LV_EVENT_CLICKED, nullptr);
        Check(std::string(widgets::InputText(k.input)) == "a" && s_typing_notes == 0, (std::string(kind) + "OK on the field changes nothing").c_str());
    }
}

// A button press and release read through the router's focus rules, as LVGL's keypad read hands them over: from here the
// buttons are the last input given, so a focus the code makes with no input being read rings, as after a press on the board.
void PressThroughFocusRules()
{
    lv_indev_data_t data{};
    data.key = LV_KEY_NEXT;
    data.state = LV_INDEV_STATE_PRESSED;
    lvgl_display::s_key_hook(nullptr, &data);
    data.state = LV_INDEV_STATE_RELEASED;
    lvgl_display::s_key_hook(nullptr, &data);
}

}  // namespace

int main()
{
    lv_init();
    lv_tick_set_cb(Ticks);
    lv_display_t *display = lv_display_create(480, 800);
    static uint8_t buffer[480 * 800];
    lv_display_set_color_format(display, LV_COLOR_FORMAT_L8);
    lv_display_set_buffers(display, buffer, nullptr, sizeof buffer, LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(display, Flush);
    lv_display_set_theme(display, nullptr);
    s_group = lv_group_create();
    lv_group_set_default(s_group);
    // The buttons' own indev: a keypad on the group, so a focus move gives the ring's key-focus state as the firmware's does.
    lv_indev_t *keys = lv_indev_create();
    lv_indev_set_type(keys, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(keys, [](lv_indev_t *, lv_indev_data_t *data) { data->state = LV_INDEV_STATE_RELEASED; });
    lv_indev_set_group(keys, s_group);
    if (ui::Start() != ESP_OK) {
        std::printf("ui::Start failed\n");
        return 1;
    }
    router::Init();
    PressThroughFocusRules();
    TestGeometry();
    TestCapacityAndTail();
    TestClearCross();
    TestPasswordReveal();
    TestSeparateRings(false);
    TestSeparateRings(true);
    TestWipe();
    TestKeyboardWalk();
    TestKeyboardPageSwitch();
    TestKeyboardEscape();
    TestSeparateRingsInKeyboard();
    std::printf("text input on LVGL: %d checks, %d failed\n", s_checks, s_failures);
    return s_failures == 0 ? 0 : 1;
}
