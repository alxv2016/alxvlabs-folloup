// A status block, the deck's: a mark beside a heading in label over a line in caption — an empty or a done state
// (DESIGN → Components → The kit).

#pragma once

#include <cstdint>

#include "lvgl.h"

namespace widgets {

// The deck's three status marks, compiled at one bit into this component (marks/make_marks.py); FolloUp has no icon
// set. A block with kNone leaves the slot empty: its words stand `spacing.lg` in, where they would after a mark.
enum class StatusMark : uint8_t { kNone, kCheck, kClock, kLinkOff };

// The mark's ink flush with the block's left edge, `spacing.lg` before the words, the pair centred on each other; full
// width. Not a control. The heading's words say what the mark means.
lv_obj_t *MakeStatusBlock(lv_obj_t *parent, const char *heading, const char *description,
                          StatusMark mark = StatusMark::kNone);

}  // namespace widgets
