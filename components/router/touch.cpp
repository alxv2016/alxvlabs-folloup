// The touch side of the focus rules: a touch shows no ring and drops the one showing, keeping the focus place — the ring is
// the buttons' (DESIGN → Focus and input; APP_ARCHITECTURE §7) — and a lift off a pressed control presses nothing (slide-off).
// The touch read hook, and which input was given last.

#include "design_tokens.h"
#include "lvgl_display.h"
#include "router/router.h"
#include "router_internal.h"

namespace router {
namespace {

bool s_touch_down = false;
bool s_keys_last = false;  // the last input given was a button press, not a touch

// The flag CancelOnSlideOff sets: LVGL's first user flag, used for nothing else in the app.
constexpr lv_obj_flag_t kSlideOffFlag = LV_OBJ_FLAG_USER_1;
// How far past a control's hit area a finger may lift and still press it: `spacing.md`, a fingertip's wobble at the lift.
constexpr int32_t kSlideOffSlop = design_tokens::kSpacingMd;

bool CancelsOnSlideOff(lv_obj_t *obj)
{
    return lv_obj_check_type(obj, &lv_button_class) || lv_obj_get_group(obj) != nullptr || lv_obj_has_flag(obj, kSlideOffFlag);
}

// The control a contact pressed: the object LVGL presses, found as LVGL finds it — the system layer, the top layer, the screen,
// the bottom layer — when the contact begins. The roots are press-locked, as every child is, so the contact keeps it however far
// the finger slides. Held: it may go while the finger is down.
lv_obj_t *s_pressed = nullptr;

lv_obj_t *PressedAt(lv_point_t point)
{
    lv_display_t *display = lv_display_get_default();
    for (lv_obj_t *layer : {lv_display_get_layer_sys(display), lv_display_get_layer_top(display),
                            lv_display_get_screen_active(display), lv_display_get_layer_bottom(display)}) {
        if (lv_obj_t *found = layer != nullptr ? lv_indev_search_obj(layer, &point) : nullptr; found != nullptr) {
            return found;
        }
    }
    return nullptr;
}

// At the lift: the control the contact pressed, when the finger left it — its hit area and the slop — and no scroll took the
// contact, is told the press was lost, and LVGL forgets the contact before it reads the lift, so no RELEASED and no CLICKED
// reach it. Read at the lift, not as the finger moves: a drag the swipe or the page has not claimed yet stays theirs.
void CancelSlidOffPress(lv_indev_t *indev, lv_point_t point)
{
    lv_obj_t *pressed = s_pressed;
    Hold(&s_pressed, nullptr);
    if (indev == nullptr || pressed == nullptr || lv_indev_get_scroll_obj(indev) != nullptr || !CancelsOnSlideOff(pressed)) {
        return;
    }
    lv_area_t area;
    lv_obj_get_click_area(pressed, &area);
    lv_area_increase(&area, kSlideOffSlop, kSlideOffSlop);
    if (point.x >= area.x1 && point.x <= area.x2 && point.y >= area.y1 && point.y <= area.y2) {
        return;
    }
    lv_obj_send_event(pressed, LV_EVENT_PRESS_LOST, indev);
    lv_indev_reset(indev, pressed);
}

}  // namespace

void CancelOnSlideOff(lv_obj_t *obj)
{
    if (obj != nullptr) {
        lv_obj_add_flag(obj, kSlideOffFlag);
    }
}

namespace detail {

void NoteKeyInput(bool keys)
{
    s_keys_last = keys;
}

bool KeysWereLast()
{
    return s_keys_last;
}

// Every read of the touch panel, before LVGL acts: a contact that begins drops the ring, before LVGL's click focus moves the
// place to what it landed on — which a finger's focus never rings; a lift off the control it pressed presses nothing.
void TouchReadHook(lv_indev_t *indev, lv_indev_data_t *data)
{
    const bool down = data->state == LV_INDEV_STATE_PRESSED;
    if (down && !s_touch_down) {
        NoteKeyInput(false);
        ClearRing();
        Hold(&s_pressed, PressedAt(data->point));
    } else if (!down && s_touch_down) {
        CancelSlidOffPress(indev, data->point);
    }
    s_touch_down = down;
}

}  // namespace detail
}  // namespace router
