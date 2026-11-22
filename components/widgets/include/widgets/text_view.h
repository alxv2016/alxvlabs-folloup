// A text view: `caption` text on a loose line, no box, taller than its band. OK opens it for reading, UP and DOWN or a swipe
// up or down turn a page of whole lines — the last line of one page the first of the next — and OK closes it; a scroll
// indicator shows which page it is on (DESIGN → Components → The kit; DESIGN → Layout → Paging).

#pragma once

#include <cstdint>

#include "lvgl.h"
#include "widgets/scroll_indicator.h"

namespace widgets {

struct TextView {
    lv_obj_t *frame;  // the object the screen places: full width, the lines and their inset tall
    lv_obj_t *root;   // the band the words scroll in, inset in the frame; a focus stop; sends LV_EVENT_VALUE_CHANGED when the page changes
    lv_obj_t *label;
    ScrollIndicator indicator;
};

// A view of `lines` whole lines of `caption` on a 30 px line, full width, its words `spacing.sm` inside it on every side — the
// view is `lines` × 30 + 16 px tall and a line is 34 characters wide, the band that holds the words (`root`) inside the view
// — and `rounded.lg` round its ring: the ring is 4 px focused and 6 px while it is open for reading. Its parents let the
// ring out (`LetRingOut`).
TextView MakeTextView(lv_obj_t *parent, int32_t lines, const char *text);
void SetTextView(const TextView &view, const char *text);  // back to the first page
int TextViewPage(const TextView &view);   // 0-based
int TextViewPages(const TextView &view);

}  // namespace widgets
