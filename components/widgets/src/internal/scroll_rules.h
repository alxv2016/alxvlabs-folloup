// Where the scroll indicator's thumb stands for a page, with no LVGL in it: pure arithmetic, so tests/host checks it on
// the computer with no board (DESIGN → Components → The kit; STICKY_DESIGN_TOKENS §9).
//
// The thumb travels `room` px (the lane less the thumb) over `pages` pages. Its outer ends never step in and out: it
// leaves an end only when `hold` (a gap and a dot of track) fits behind it, so an interior page is never inside the first
// or last `hold` px. A short list's pages are proportional, `room * page / (pages - 1)`, and already clear that zone. A
// crowded one (proportional steps under `hold`: past 15 pages at 248 px) would put pages 0 and 1, and the last two, on the
// same spot, so only the first page keeps 0 and only the last keeps `room`, and the pages between spread evenly from
// `hold` to `room - hold` — every page turn moves the thumb.

#pragma once

#include <cstdint>

namespace widgets::detail {

// The proportional position, as the indicator always drew it: `first` of `span`, the view's travel in the same unit.
constexpr int32_t ProportionalAt(int32_t room, int32_t first, int32_t span)
{
    return static_cast<int32_t>(static_cast<int64_t>(room) * first / span);
}

// The thumb's top, from the lane's top, for `page` (0-based) of `pages`, the thumb having `room` px to travel and holding
// `hold` px off each end.
constexpr int32_t ThumbAt(int32_t room, int32_t hold, int32_t page, int32_t pages)
{
    if (pages <= 1 || page <= 0) {
        return 0;
    }
    if (page >= pages - 1) {
        return room;
    }
    const int32_t lo = hold;
    const int32_t hi = room - hold;
    const int32_t span = pages - 1;
    const int32_t proportional = ProportionalAt(room, page, span);
    // Positions only grow, so the first and last interior pages are the nearest the ends: if both clear them, all do.
    const bool crowded = ProportionalAt(room, 1, span) < lo || ProportionalAt(room, pages - 2, span) > hi;
    if (!crowded) {
        return proportional;
    }
    if (hi < lo) {  // no room to spread in: hold at an end as the proportional rule does
        return proportional < hold ? 0 : (room - proportional < hold ? room : proportional);
    }
    const int32_t between = pages - 2;  // the interior pages
    if (between == 1) {
        return (lo + hi) / 2;
    }
    return lo + static_cast<int32_t>(static_cast<int64_t>(hi - lo) * (page - 1) / (between - 1));
}

}  // namespace widgets::detail
