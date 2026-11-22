// The swipe that turns a page on a paged band, on LVGL's own scroll events. Private to the widgets component; it needs
// LVGL and the swipe's rules and nothing else, so tests/host links it with LVGL and drives it with a scripted finger
// (STICKY_DESIGN_TOKENS §10).

#pragma once

#include <cstdint>

#include "lvgl.h"

namespace widgets::detail {

// Which way a band's pages run: a list's and a text view's down the page, a pager's across it.
enum class SwipeAxis : uint8_t { kVertical, kHorizontal };
// A swipe up (`dir` +1) or down (-1) on a vertical band, or a swipe left (+1) or right (-1) on a horizontal one, turns one page.
using SwipeTurn = void (*)(lv_obj_t *band, int32_t dir);
// Makes `band` the object a vertical drag claims, so the page behind it never scrolls by a pixel: the drag is added up,
// the band is put back where the page keeps it at once — nothing moves under the finger — and on lift one swipe turns one
// page, by its distance or a flick at the end of it (swipe_rules.h; STICKY_DESIGN_TOKENS §10). The band must be
// a scrollable object (LV_DIR_VER, or LV_DIR_HOR on the horizontal axis) with more content than its view, whole pages of it. A key's or a focus's own scroll
// is not a drag and is left alone, and so is every scroll the swipe makes itself: the put-back and the turn. A horizontal
// band claims a horizontal drag only: a vertical drag that starts on it chains to the page behind, which is the page's.
void ClaimSwipe(lv_obj_t *band, SwipeTurn turn, SwipeAxis axis = SwipeAxis::kVertical);

}  // namespace widgets::detail
