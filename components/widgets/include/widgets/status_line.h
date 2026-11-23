// A status line: news that asks nothing, as plain words in a band under the screen's title (DESIGN → Components → The kit).

#pragma once

#include "lvgl.h"

namespace widgets {

// The band a screen that can get news reserves, built in its OnCreate right after its title bar (ui::NewStatusBar) and before its
// content: `spacing.list-row-height` high and full width, white, with its words in `caption-strong` at `spacing.edge`,
// `spacing.content` wide, centred between the title bar's rule above and a `spacing.stroke.hairline` rule below that shows only
// while news does. Words wrap to two lines on the words and never cut; a third line is ended by LVGL's dots. No box, no fill,
// no edge, no glyph, no focus group and no press: nothing in it can be taken for a control, and a touch on it falls to the page.
//
// It is the router's to draw into: this hands it the label and the rule (router::AttachStatusLine), and router::Say and
// router::Clear set and empty them. Its height is the same empty, with one line or with two, so nothing below it moves
// when news comes. One per screen. Returns the band's zone.
lv_obj_t *MakeStatusLine(lv_obj_t *parent);

}  // namespace widgets
