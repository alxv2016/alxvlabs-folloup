// A section heading: a group's name in caption, text-secondary, above it (DESIGN → Components → The kit).

#pragma once

#include "lvgl.h"

namespace widgets {

// Where the heading's words start: at `spacing.edge`, the left of the boxes and controls under it and of the setting rows' words
// (buttons, chips, cards, inputs, sliders, toggle, checkbox and radio button rows), or `spacing.md` in from it, where the words of
// the rows under it stand (value rows, detail rows, list rows, network rows, dropdowns, status blocks).
enum class HeadingX { kEdge, kRowWords };

// `spacing.section` stands over the heading, from the content above it: its `pad_top` is the section less the parent's own gap
// between children, so the two add up to it however the parent sets that gap. The first heading in `parent` has the page's top
// padding above it and keeps `spacing.lg`.
lv_obj_t *MakeSectionHeading(lv_obj_t *parent, const char *text, HeadingX x = HeadingX::kEdge);

}  // namespace widgets
