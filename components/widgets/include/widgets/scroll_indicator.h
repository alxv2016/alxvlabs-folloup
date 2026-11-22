// A scroll indicator, the deck's: one narrow lane of separate pieces, each a rounded stroke with round caps — a short track
// piece in the light dither above, a gap, the black thumb (the view's share of the length), a gap, and the rest of the
// track below. The pieces never touch and no line runs behind them. The thumb is never shorter than a dot; the first and last
// page keep its ends, and the pages between stand a gap and a dot in from them — proportionally for a short list, spread
// evenly for a crowded one, so every page turn moves it; everything fitting hides it (DESIGN → Components → The kit).

#pragma once

#include <cstdint>

#include "lvgl.h"

namespace widgets {

struct ScrollIndicator {
    lv_obj_t *lane;
    lv_obj_t *before;  // the track above the thumb
    lv_obj_t *thumb;
    lv_obj_t *after;   // the track below it
    int32_t length;
};

// What the lane is centred on: the host's own height, or the glass's.
enum class IndicatorCentre : uint8_t { kView, kGlass };

// A lane `spacing.scroll-thumb` wide in `host`'s right margin, its x `spacing.scroll-x` on the glass, `spacing.scroll-length`
// long at most. Centred on the host's height `band_height` for a view in a page (`kView`), or on the glass's height, the same
// on every page, for a view that fills the content (`kGlass`: Home). `host` is a full-width object in the content column;
// it lets its children out (`LetRingOut`), and this widens its reach to the lane, which LVGL would otherwise clip.
ScrollIndicator MakeScrollIndicator(lv_obj_t *host, int32_t band_height, IndicatorCentre centre = IndicatorCentre::kView);
// Where the view stands: page `page` (0-based) of `pages`, the view `view` tall and the content `content` tall, in the same
// unit — they give the thumb's length, `page` and `pages` its place. Both views are paged (a page turn is all they do),
// so the place is a page's, not a pixel's. Content that fits its view, or a single page, hides the indicator.
void UpdateScrollIndicator(const ScrollIndicator &indicator, int32_t page, int32_t pages, int32_t view, int32_t content);

}  // namespace widgets
