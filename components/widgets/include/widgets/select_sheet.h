// The select sheet: a dropdown's options as a bottom sheet over the lower part of the page — full width, anchored to the glass's
// bottom, its top corners `rounded.2xl`, a `spacing.stroke.strong` outline and an offset on its top edge — BYTE-90's sheet
// ported (DESIGN → Components → The kit). A title names the field, the options are a list view of rows `spacing.md` apart, and the
// foot, in the footer's place, holds Cancel and what the buttons do in words. It appears in the next frame, closes the same way,
// and the page above stays as it was.
//
// It is an overlay on the router's stack (router/router.h): its own focus group while it is open, holding OK as Cancel, and
// focus back on the field, ringed if it was, when it closes. Its objects stand on the screen it covers.

#pragma once

#include "lvgl.h"
#include "widgets/dropdown.h"

namespace widgets {

// Opens the sheet for `dropdown` on the active screen: the field's name for a title, a row for each option — the option in
// `caption-strong`, about 30 characters to a line, wrapping to a second line and growing the row when a third is needed, never cut
// — the chosen one inverted with the check beside its words, and a foot with Cancel and "UP or DOWN picks. OK chooses." It takes
// focus on the chosen row, on its page. Rows a page: four, or three when four would cover the field that opened it. OK on a row
// chooses it (ChooseDropdown, which reports a change once) and closes; Cancel, holding OK, or choosing nothing new closes with
// nothing changed; focus goes back to the field. A touch above the sheet is eaten. One sheet at a time.
void OpenSelectSheet(Dropdown *dropdown);
// Closes the sheet as Cancel does; does nothing when none is open. Holding OK does the same, through the overlay stack.
void CloseSelectSheet();
bool SelectSheetOpen();

// How many rows a page holds for a field whose bottom edge is at `field_bottom` on a glass `glass_height` tall: four when the sheet
// of four rows leaves the field clear — its bottom, the ring's 8, 8 of white and the offset's 6 above the sheet's top — otherwise
// three (never fewer: two would turn pages too often; a field below three rows' reach is covered, and the title names it).
int SelectSheetRowsPerPage(int32_t field_bottom, int32_t glass_height);

}  // namespace widgets
