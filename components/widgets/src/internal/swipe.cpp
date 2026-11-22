#include "internal/swipe.h"

#include "internal/swipe_rules.h"

namespace widgets::detail {

namespace {
struct Swipe {
    SwipeTurn turn;
    SwipeAxis axis;
    int32_t home;     // where the page keeps the band along its axis, taken as the drag begins
    bool settling;    // a scroll the swipe makes itself is under way: LVGL sends it the same scroll events a finger's makes
    bool armed;       // a finger's drag has begun and its lift has not been read yet
    Drag drag;        // the drag so far and its latest moves (swipe_rules.h)
};

// The band's place along its axis, and the put-back to a place.
int32_t Scrolled(const Swipe *s, lv_obj_t *band)
{
    return s->axis == SwipeAxis::kHorizontal ? lv_obj_get_scroll_x(band) : lv_obj_get_scroll_y(band);
}

// Every scroll the swipe makes — the put-back under the finger and the page turn — goes through
// lv_obj_scroll_to_y (or _x), which sends LV_EVENT_SCROLL_BEGIN, LV_EVENT_SCROLL and LV_EVENT_SCROLL_END as a finger's does,
// while the finger's pointer still holds the band. `settling` has the handlers below leave those events alone; without it
// the put-back begins a drag of its own, which puts the band back, and so on without end.
void SettleAt(Swipe *s, lv_obj_t *band, int32_t at)
{
    s->settling = true;
    if (s->axis == SwipeAxis::kHorizontal) {
        lv_obj_scroll_to_x(band, at, LV_ANIM_OFF);
    } else {
        lv_obj_scroll_to_y(band, at, LV_ANIM_OFF);
    }
    s->settling = false;
}

// Only a finger's drag is read: a key's or a focus's scroll comes with no pointer holding this band, and the scroll
// LVGL's throw makes after the lift runs from an animation callback, where lv_indev_active() is null.
bool Dragged(lv_obj_t *band)
{
    lv_indev_t *indev = lv_indev_active();
    return indev != nullptr && lv_indev_get_scroll_obj(indev) == band;
}

void OnSwipeBegin(lv_event_t *e)
{
    lv_obj_t *band = lv_event_get_target_obj(e);
    Swipe *s = static_cast<Swipe *>(lv_event_get_user_data(e));
    if (s->settling || !Dragged(band)) {
        return;
    }
    s->home = Scrolled(s, band);
    BeginDrag(s->drag);
    s->armed = true;
}

void OnSwipeMove(lv_event_t *e)
{
    lv_obj_t *band = lv_event_get_target_obj(e);
    Swipe *s = static_cast<Swipe *>(lv_event_get_user_data(e));
    if (s->settling || !Dragged(band)) {
        return;
    }
    const int32_t moved = Scrolled(s, band) - s->home;
    if (moved == 0) {
        return;
    }
    LogMove(s->drag, lv_tick_get(), moved);
    SettleAt(s, band, s->home);  // nothing moves under the finger: the page lands on lift
}

// What the lift does, once per drag: the log says whether it turns a page.
void Decide(Swipe *s, lv_obj_t *band)
{
    const int dir = PageTurn(s->drag, lv_tick_get());
    BeginDrag(s->drag);
    s->armed = false;
    if (dir != 0) {
        s->settling = true;  // the turn scrolls the band to its new page, and that is not the finger
        s->turn(band, dir);
        s->settling = false;
    }
}

// The lift: LVGL sends LV_EVENT_SCROLL_THROW_BEGIN to the scrolled object as the finger leaves, inside the touch read, so
// the window is the last kTouchFlickWindowMs before the lift.
void OnSwipeLift(lv_event_t *e)
{
    Swipe *s = static_cast<Swipe *>(lv_event_get_user_data(e));
    if (s->armed) {
        Decide(s, lv_event_get_target_obj(e));
    }
}

// The lift of a drag that began on a disabled child of the band: LVGL sends no LV_EVENT_SCROLL_THROW_BEGIN for it (it
// sends neither the release nor the throw for a disabled object), only the LV_EVENT_SCROLL_END its throw ends with, a
// little after the lift and from its animation. The drag is read then: its distance is exact, a flick at its end may read
// as a finger that rested if the task was busy meanwhile. LVGL's own scroll ends carry no parameter; this one carries the
// input device.
void OnSwipeEnd(lv_event_t *e)
{
    Swipe *s = static_cast<Swipe *>(lv_event_get_user_data(e));
    if (s->settling || !s->armed || lv_event_get_param(e) == nullptr) {
        return;
    }
    Decide(s, lv_event_get_target_obj(e));
}

void OnSwipeDelete(lv_event_t *e)
{
    delete static_cast<Swipe *>(lv_event_get_user_data(e));
}
}  // namespace

void ClaimSwipe(lv_obj_t *band, SwipeTurn turn, SwipeAxis axis)
{
    Swipe *s = new Swipe{turn, axis, 0, false, false, Drag{}};
    lv_obj_set_scrollable(band, true);
    lv_obj_set_scroll_dir(band, axis == SwipeAxis::kHorizontal ? LV_DIR_HOR : LV_DIR_VER);
    lv_obj_set_scrollbar_mode(band, LV_SCROLLBAR_MODE_OFF);
    // The page behind never takes a drag along the band's axis; the other axis chains to it, so a vertical drag on a horizontal
    // strip is the page's.
    lv_obj_remove_flag(band, axis == SwipeAxis::kHorizontal ? LV_OBJ_FLAG_SCROLL_CHAIN_HOR : LV_OBJ_FLAG_SCROLL_CHAIN_VER);  // touch-exception: only the band's own axis is turned off, the other chains to the page
    lv_obj_remove_flag(band, LV_OBJ_FLAG_SCROLL_MOMENTUM);    // touch-exception: a throw would move the band by pixels on the glass; a swipe turns one page
    lv_obj_remove_flag(band, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_flag(band, LV_OBJ_FLAG_CLICKABLE);  // a touch between rows still drags it
    lv_obj_add_event_cb(band, OnSwipeBegin, LV_EVENT_SCROLL_BEGIN, s);
    lv_obj_add_event_cb(band, OnSwipeMove, LV_EVENT_SCROLL, s);
    lv_obj_add_event_cb(band, OnSwipeLift, LV_EVENT_SCROLL_THROW_BEGIN, s);
    lv_obj_add_event_cb(band, OnSwipeEnd, LV_EVENT_SCROLL_END, s);
    lv_obj_add_event_cb(band, OnSwipeDelete, LV_EVENT_DELETE, s);
}

}  // namespace widgets::detail
