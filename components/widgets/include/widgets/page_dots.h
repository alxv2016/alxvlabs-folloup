// Pagination, the deck's straight page bar: the current page a `spacing.page-pill` by `spacing.page-dot` black pill, every
// other a `spacing.page-dot` hollow dot, `spacing.sm` apart; an indicator, never a control — the deck's dots are not a tap
// target either (DESIGN → Components → The kit).

#pragma once

#include "lvgl.h"

namespace widgets {

inline constexpr int kPageDotsMax = 13;

lv_obj_t *MakePageDots(lv_obj_t *parent, int count, int current);
void SetPageDot(lv_obj_t *dots, int current);

}  // namespace widgets
