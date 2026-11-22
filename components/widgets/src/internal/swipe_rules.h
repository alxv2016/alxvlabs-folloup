// The page swipe's decision, with no LVGL in it: a drag is logged as it goes, and on lift the log says whether it turns a
// page, and which way. Pure and constexpr-friendly, so tests/host checks it on the computer with no board
// (STICKY_DESIGN_TOKENS §10).
//
// A drag turns one page when it travelled `kTouchSwipeDistance`, or when it is a flick: it travelled at least
// `kTouchFlickDistance` and moved `kTouchFlickMovement` the same way in the last `kTouchFlickWindowMs` before the lift. A
// finger that rested, or one that pulled back, places and never throws.

#pragma once

#include <cstdint>

#include "design_tokens.h"

namespace widgets::detail {

// One move of the finger: when it was read, and how far the page had been dragged by then — positive for the finger
// moving up on a vertical band, or left on a horizontal one (the band's scroll position grows), so a swipe up or left turns
// forward.
struct DragSample {
    uint32_t at_ms;
    int32_t sum;
};

// A drag so far: the net travel and a ring of the latest 32 samples — 96 ms of a finger read every three milliseconds,
// 320 ms at the firmware's 10 ms read; a faster read drops the oldest, and the window then starts at the oldest kept.
struct Drag {
    static constexpr int kSamples = 32;
    DragSample log[kSamples] = {};
    int moves = 0;     // how many samples were ever logged; the ring keeps the newest kSamples
    int32_t sum = 0;   // net travel: moves in the other direction take it back
};

constexpr void BeginDrag(Drag &drag)
{
    drag = Drag{};
}

constexpr void LogMove(Drag &drag, uint32_t now_ms, int32_t by)
{
    drag.sum += by;
    drag.log[drag.moves % Drag::kSamples] = DragSample{now_ms, drag.sum};
    ++drag.moves;
}

// How far the page was dragged in the `window_ms` before `now_ms`: the net travel now, less the travel as it stood at the
// window's start — the newest sample older than the window, or nothing when the drag began inside it; a move exactly
// `window_ms` before `now_ms` is still inside. A finger that stopped moving has an empty window, however fast it was
// going before.
constexpr int32_t MovedWithin(const Drag &drag, uint32_t now_ms, uint32_t window_ms)
{
    const int kept = drag.moves < Drag::kSamples ? drag.moves : Drag::kSamples;
    for (int back = 1; back <= kept; ++back) {
        const DragSample &sample = drag.log[(drag.moves - back) % Drag::kSamples];
        if (now_ms - sample.at_ms > window_ms) {  // unsigned: a wrapped tick still reads as the time between
            return drag.sum - sample.sum;
        }
    }
    // Every kept sample is inside the window. When the ring dropped older ones the oldest kept is the best start there is.
    if (drag.moves > Drag::kSamples) {
        return drag.sum - drag.log[drag.moves % Drag::kSamples].sum;
    }
    return drag.sum;
}

constexpr int32_t Abs(int32_t value)
{
    return value < 0 ? -value : value;
}

// What a lift at `now_ms` does: +1 turns to the next page (a swipe up), -1 to the previous, 0 leaves the page.
constexpr int PageTurn(const Drag &drag, uint32_t now_ms)
{
    namespace dt = design_tokens;
    if (drag.sum == 0) {
        return 0;
    }
    const int dir = drag.sum > 0 ? 1 : -1;
    if (Abs(drag.sum) >= dt::kTouchSwipeDistance) {
        return dir;
    }
    const int32_t lately = MovedWithin(drag, now_ms, dt::kTouchFlickWindowMs);
    const bool same_way = lately != 0 && (lately > 0) == (drag.sum > 0);
    if (Abs(drag.sum) >= dt::kTouchFlickDistance && same_way && Abs(lately) >= dt::kTouchFlickMovement) {
        return dir;
    }
    return 0;
}

}  // namespace widgets::detail
