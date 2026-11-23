// The focus rules: the first button press with no ring on the glass shows the ring and presses nothing (the key read hook),
// and the hand-focus helpers every widget that moves focus by hand goes through (router.h; DESIGN → Focus and input;
// APP_ARCHITECTURE §7). The touch side is touch.cpp's.

#include "lvgl_display.h"
#include "router/router.h"
#include "router_internal.h"

namespace router {
namespace {

bool s_key_held = false;      // a button is down, as the last key read reported it
bool s_key_consumed = false;  // the press now held only rang: LVGL is told it is up until it comes up

bool HiddenInTree(const lv_obj_t *obj)
{
    for (const lv_obj_t *o = obj; o != nullptr; o = lv_obj_get_parent(o)) {
        if (lv_obj_has_flag(o, LV_OBJ_FLAG_HIDDEN)) {
            return true;
        }
    }
    return false;
}

// Whether `obj` is `of` or inside it.
bool Within(const lv_obj_t *obj, const lv_obj_t *of)
{
    for (const lv_obj_t *o = obj; o != nullptr; o = lv_obj_get_parent(o)) {
        if (o == of) {
            return true;
        }
    }
    return false;
}

// On the glass: shown, and not scrolled out of the containers it is in.
bool OnGlass(lv_obj_t *obj)
{
    return obj != nullptr && !HiddenInTree(obj) && lv_obj_is_visible(obj);
}

bool RingShowing(lv_group_t *group)
{
    lv_obj_t *focused = lv_group_get_focused(group);
    return focused != nullptr && lv_obj_has_state(focused, LV_STATE_FOCUS_KEY) && OnGlass(focused);
}

// No ring showing: the focused control takes it when it is on the glass; otherwise the first control in view, in the walk's
// order; with none in view, the focused one, brought into view by its own FOCUSED handler (ui::ShowWhenFocused).
void RingFromNone(lv_group_t *group)
{
    lv_obj_t *focused = lv_group_get_focused(group);
    if (focused != nullptr && OnGlass(focused)) {
        lv_obj_add_state(focused, LV_STATE_FOCUS_KEY);
        return;
    }
    const uint32_t n = lv_group_get_obj_count(group);
    for (uint32_t i = 0; i < n; ++i) {
        lv_obj_t *obj = lv_group_get_obj_by_index(group, i);
        if (OnGlass(obj) && !lv_obj_has_state(obj, LV_STATE_DISABLED)) {
            Ring(obj);
            return;
        }
    }
    if (focused != nullptr) {
        lv_obj_send_event(focused, LV_EVENT_FOCUSED, nullptr);  // into view, as a move to it would bring it
        lv_obj_add_state(focused, LV_STATE_FOCUS_KEY);
    }
}

// Sets whether `obj` wears the ring, for a focus moved by hand.
void SetRinged(lv_obj_t *obj, bool ringed)
{
    if (ringed) {
        lv_obj_add_state(obj, LV_STATE_FOCUS_KEY);
    } else {
        lv_obj_remove_state(obj, LV_STATE_FOCUS_KEY);
    }
}

// Takes the focus away from `obj`, or what is focused inside it, before it goes: to `next`, with the ring only if the focused
// control wore it, or — with no `next` — off it, its group still naming it. Nothing when no control in `obj` is focused.
void HandFocusAway(lv_obj_t *obj, lv_obj_t *next)
{
    lv_group_t *group = next != nullptr ? lv_obj_get_group(next) : lv_obj_get_group(obj);
    if (group == nullptr) {
        group = lvgl_display::Group();
    }
    lv_obj_t *focused = group != nullptr ? lv_group_get_focused(group) : nullptr;
    if (focused == nullptr || !Within(focused, obj)) {
        return;
    }
    const bool ringed = lv_obj_has_state(focused, LV_STATE_FOCUS_KEY);
    if (next != nullptr && next != focused && lv_obj_get_group(next) == group) {
        lv_group_focus_obj(next);
        SetRinged(next, ringed);
    } else {
        lv_obj_send_event(focused, LV_EVENT_DEFOCUSED, nullptr);  // LVGL moves focus on a hide only from an object wearing FOCUSED
    }
}

void OnHeldDelete(lv_event_t *e)
{
    lv_obj_t **slot = static_cast<lv_obj_t **>(lv_event_get_user_data(e));
    *slot = nullptr;
}

}  // namespace

namespace detail {

// Every read of the buttons, after lvgl_display's keypad read has filled it and before LVGL acts. With no ring on the glass,
// the first press of UP, DOWN or OK — not while a control is open for editing, whose keys they are — shows the ring and
// presses nothing: LVGL is told the button is up until it comes up (DESIGN → Focus and input; BYTE-90's ConsumeUnringedPress).
// The tick has sounded already, on the input task. A held UP or DOWN steps on from there at `motion.key-repeat-delay`: each
// step is a release and a fresh press, which the ring now showing lets through.
void KeyReadHook(lv_indev_t *, lv_indev_data_t *data)
{
    const bool down = data->state == LV_INDEV_STATE_PRESSED;
    if (down && !s_key_held) {
        NoteKeyInput(true);
        lv_obj_update_layout(lv_screen_active());  // a screen just pushed is laid out before what is in view is read
        lv_group_t *group = lvgl_display::Group();
        if (group != nullptr && !lv_group_get_editing(group) && !RingShowing(group)) {
            RingFromNone(group);
            s_key_consumed = true;
        }
    }
    s_key_held = down;
    if (s_key_consumed) {
        data->state = LV_INDEV_STATE_RELEASED;
        if (!down) {
            s_key_consumed = false;
        }
    }
}

}  // namespace detail

void Focus(lv_obj_t *obj)
{
    lv_group_t *group = obj != nullptr ? lv_obj_get_group(obj) : nullptr;
    if (group == nullptr) {
        return;
    }
    if (lv_group_get_focused(group) != obj) {
        lv_group_focus_obj(obj);
    }
    // The ring is the buttons': a move a key press made shows it, one a finger made does not. With no input being read — a timer,
    // an async call — the last input given decides.
    lv_indev_t *indev = lv_indev_active();
    const bool keys = indev != nullptr ? lv_indev_get_type(indev) == LV_INDEV_TYPE_KEYPAD : detail::KeysWereLast();
    SetRinged(obj, keys);
}

void Ring(lv_obj_t *obj)
{
    lv_group_t *group = obj != nullptr ? lv_obj_get_group(obj) : nullptr;
    if (group == nullptr || group != lvgl_display::Group() || HiddenInTree(obj)) {
        return;  // never a ring under an overlay, nor on a control that is not on the glass
    }
    if (lv_group_get_focused(group) != obj) {
        lv_group_focus_obj(obj);
    }
    lv_obj_add_state(obj, LV_STATE_FOCUS_KEY);
}

void ClearRing()
{
    lv_group_t *group = lvgl_display::Group();
    if (group == nullptr || lv_group_get_editing(group)) {
        return;  // an open control keeps its ring: its keys are UP and DOWN until OK closes it
    }
    if (lv_obj_t *focused = lv_group_get_focused(group); focused != nullptr) {
        lv_obj_remove_state(focused, LV_STATE_FOCUS_KEY);
    }
}

void SetHiddenHandingFocus(lv_obj_t *obj, bool hidden, lv_obj_t *next)
{
    if (obj == nullptr || lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN) == hidden) {
        return;
    }
    if (hidden) {
        HandFocusAway(obj, next);
    }
    lv_obj_set_hidden(obj, hidden);
}

void DeleteHandingFocus(lv_obj_t *obj, lv_obj_t *next)
{
    if (obj == nullptr) {
        return;
    }
    if (next != nullptr && lv_obj_get_group(next) != nullptr) {
        HandFocusAway(obj, next);
    }
    lv_obj_delete(obj);
}

void Hold(lv_obj_t **slot, lv_obj_t *obj)
{
    if (slot == nullptr) {
        return;
    }
    if (*slot != nullptr) {
        lv_obj_remove_event_cb_with_user_data(*slot, OnHeldDelete, slot);
    }
    *slot = obj;
    if (obj != nullptr) {
        lv_obj_add_event_cb(obj, OnHeldDelete, LV_EVENT_DELETE, slot);
    }
}

}  // namespace router
