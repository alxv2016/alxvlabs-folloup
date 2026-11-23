// A list view: a column of rows taller than its band, no box. It turns a page of whole rows at a time, never scrolling by
// pixel: UP and DOWN walk the rows and the band turns when the focus leaves it, a swipe up or down turns one page, and a scroll
// indicator shows which page it is on. A page with no row that takes focus — a page of detail rows or plain text — is one stop of
// the walk with the ring round its rows, so the keys reach every page; a screen can ask for the page in words under the list
// (DESIGN → Components → The kit; DESIGN → Layout → Paging).

#pragma once

#include <cstdint>

#include "lvgl.h"
#include "widgets/scroll_indicator.h"

namespace widgets {

struct ListView {
    lv_obj_t *frame;  // the object the screen places: full width, the page's rows tall
    lv_obj_t *root;   // add the rows here, then call UpdateListView; sends LV_EVENT_VALUE_CHANGED when the page changes
    ScrollIndicator indicator;
    int32_t rows_per_page;
    int32_t row_height;
    lv_obj_t *words;  // "1 of 2" under the view when asked for (`PageWords::kUnder`) and there is more than one page; else nullptr
};

// Whether the list says its page in words, as the horizontal pager does.
enum class PageWords : uint8_t { kNone, kUnder };

// A band of `rows_per_page` rows of `row_height` (`spacing.target` for a value row, `spacing.list-row-height` for a home row),
// `spacing.md` apart, in the content column — its parents let the focus ring out (`LetRingOut`); the band is `spacing.sm`
// wider and taller all round, which is the ring's reach, so a focused row's ring stays inside it. The rows are full-width
// controls of that height.
// The scroll indicator is centred on the view (`IndicatorCentre::kView`), or on the glass for a list that fills the content
// column, as Home's does (`kGlass`).
// `kUnder` adds the page in words ("1 of 2", `caption`, `text-secondary`) at the rows' left edge, `spacing.md` under the view's
// whole height so it stands in one place on every page: the frame grows by that line, and only while there is more than one
// page. A list with no room for it (Home's fills the content) never asks.
ListView MakeListView(lv_obj_t *parent, int32_t rows_per_page, int32_t row_height,
                      IndicatorCentre centre = IndicatorCentre::kView, PageWords words = PageWords::kNone);
// After the rows are added: completes the last page with empty room, makes each focusable row turn the page when it takes
// the focus, and shows the first page. A page in which no row is in the focus group gets a stop of its own, added to the group
// in page order beside the rows: before the first row of a later page that takes focus, else after the last row or stop before
// it. The stop is the rows' own ring round the page's rows, first row's top to last row's bottom, `rounded.lg`; focus on it turns
// to its page, UP or DOWN leaves it as they leave a row, OK on it does nothing and it takes no touch (a swipe turns the page as
// before). A page that holds a row that takes focus has no stop: its facts show with it.
// Calling this again after changing the rows makes the stops anew, in the same places: a list with rows by them keeps their order,
// and a list of facts only keeps the place its first stop had. The first call of a list of facts only has no row or stop to stand
// by, so make it before the controls that follow the list in the walk (the footer's Back): its stops are added at the end.
void UpdateListView(const ListView &view);
int ListViewPage(const ListView &view);   // 0-based
int ListViewPages(const ListView &view);

}  // namespace widgets
