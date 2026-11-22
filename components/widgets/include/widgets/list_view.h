// A list view: a column of rows taller than its band, no box. It turns a page of whole rows at a time, never scrolling by
// pixel: UP and DOWN walk the rows and the band turns when the focus leaves it, a swipe up or down turns one page, and a scroll
// indicator shows which page it is on (DESIGN → Components → The kit; DESIGN → Layout → Paging).

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
};

// A band of `rows_per_page` rows of `row_height` (`spacing.target` for a value row, `spacing.list-row-height` for a home row),
// `spacing.md` apart, in the content column — its parents let the focus ring out (`LetRingOut`); the band is `spacing.sm`
// wider and taller all round, which is the ring's reach, so a focused row's ring stays inside it. The rows are full-width
// controls of that height.
// The scroll indicator is centred on the view (`IndicatorCentre::kView`), or on the glass for a list that fills the content
// column, as Home's does (`kGlass`).
ListView MakeListView(lv_obj_t *parent, int32_t rows_per_page, int32_t row_height,
                      IndicatorCentre centre = IndicatorCentre::kView);
// After the rows are added: completes the last page with empty room, makes each focusable row turn the page when it takes
// the focus, and shows the first page.
void UpdateListView(const ListView &view);
int ListViewPage(const ListView &view);   // 0-based
int ListViewPages(const ListView &view);

}  // namespace widgets
