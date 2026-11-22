// The select sheet's arithmetic, with no LVGL in it: how tall it stands for n rows, and how many rows a page may hold without
// covering the field that opened it. tests/host checks it on the computer with no board (STICKY_DESIGN_TOKENS §12).
//
// From the sheet's outer top edge: the outline, white, the title's line box, white, the rows (n of them, `spacing.md` apart),
// white, and the foot, the footer's height — every length a token.

#pragma once

#include <cstdint>

#include "design_tokens.h"

namespace widgets::detail {

namespace sheet {
namespace dt = design_tokens;

inline constexpr int kMostRows = 4;   // a page of the sheet holds up to four rows
inline constexpr int kFewestRows = 3;  // and never fewer: two would turn the page too often

// The rows' band: n rows of `spacing.target` and the gaps between them.
constexpr int32_t Band(int rows)
{
    return rows * dt::kSpacingTarget + (rows - 1) * dt::kSpacingMd;
}

// The sheet's height for n rows a page: 4 + 16 + 36 + 16 + band + 16 + 96 — 488 for four, 408 for three.
constexpr int32_t Height(int rows)
{
    return dt::kSpacingStrokeStrong + dt::kSpacingMd + dt::kTypographyLabel.line_px + dt::kSpacingMd + Band(rows) +
           dt::kSpacingMd + dt::kSpacingFooter;
}

// What must stand clear between a field's bottom edge and the sheet's top: the ring's reach, white, and the offset above the sheet.
constexpr int32_t Clearance()
{
    return dt::kSpacingSm + dt::kSpacingSm + dt::kSpacingOverlayOffset;
}

// Four rows a page when the sheet of four leaves the field clear, otherwise three — also for a field below even that, which the
// sheet covers: its title is the field's name.
constexpr int RowsPerPage(int32_t field_bottom, int32_t glass_height)
{
    return field_bottom + Clearance() <= glass_height - Height(kMostRows) ? kMostRows : kFewestRows;
}
}  // namespace sheet

}  // namespace widgets::detail
