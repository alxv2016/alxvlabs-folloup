// A horizontal pager: a strip of items — chips, or any focus stop — that turns by page, never by pixel (DESIGN → Layout →
// Paging), the agent deck's cousin BYTE-90's horizontal scroller ported. A page is as many whole items as fit the content
// column; "1 of 2", in words alone, says which page it is on. UP and DOWN walk the items and the strip turns when
// the focus leaves a page; a swipe left or right turns one page (DESIGN → Components → The kit).

#pragma once

#include "lvgl.h"

namespace widgets {

struct HorizontalPager {
    lv_obj_t *frame;     // the object the page places: the strip, `spacing.md`, then the words
    lv_obj_t *root;      // add the items here, then call UpdateHorizontalPager; sends LV_EVENT_VALUE_CHANGED when the page changes
    lv_obj_t *words_row;  // "1 of 2" in `caption`, left-aligned, made by UpdateHorizontalPager and hidden when everything fits one page
};

// A strip `spacing.target` high, full width, in the content column: its band is `spacing.sm` wider and taller all round, which
// is the ring's reach (as a list view's), and claims a horizontal swipe only — a vertical drag that starts on it is the
// page's. Its parents let the focus ring out (`LetRingOut`). Items sit `spacing.md` apart.
HorizontalPager MakeHorizontalPager(lv_obj_t *parent);
// After the items are added: works the pages out from the items' widths — each page the whole items that fit
// `spacing.content`, the next page starting with the next item — makes each item turn the page when it takes the focus,
// draws "1 of N", and shows the first page. Call it again after items are added or removed.
void UpdateHorizontalPager(const HorizontalPager &pager);
int HorizontalPagerPage(const HorizontalPager &pager);   // 0-based
int HorizontalPagerPages(const HorizontalPager &pager);

}  // namespace widgets
