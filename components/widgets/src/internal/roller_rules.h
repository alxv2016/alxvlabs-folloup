// The roller's rules, with no LVGL in it: how a step moves the value, where a list wraps and where it stops, which neighbour
// stands in a row, what a tap on a row does, and what keeping and cancelling an edit leave. Pure and constexpr, so
// tests/host checks it on the computer with no board (STICKY_DESIGN_TOKENS §9).
//
// The roller shows three rows: the value above, the value, the value below. UP steps to the value above — the earlier index —
// and DOWN to the one below. A circular roller (hours, minutes, months) wraps with the tick alone; any other stops at its ends,
// and the caller sounds `sound.at-end` for a step that did not move.

#pragma once

namespace widgets::detail::roller {

inline constexpr int kRows = 3;    // the value above, the value, the value below
inline constexpr int kMiddle = 1;  // the row the value stands in

// The index brought into 0 .. count - 1 by going round: -1 is the last, count is the first.
constexpr int Wrap(int index, int count)
{
    const int rem = index % count;
    return rem < 0 ? rem + count : rem;
}

struct Move {
    int index;   // where the value stands after the step
    bool moved;  // false when a step ran into an end of a list that does not wrap
};

// One step: `dir` is -1 for UP (the value above) and +1 for DOWN. A circular roller always moves, and goes round at its ends.
constexpr Move Step(int index, int count, int dir, bool circular)
{
    const int next = index + dir;
    if (circular) {
        return Move{Wrap(next, count), count > 1};
    }
    if (next < 0 || next >= count) {
        return Move{index, false};
    }
    return Move{next, true};
}

// The value standing `offset` rows from the middle one (-1 above, +1 below) when the value is `index`; -1 for the blank row a list
// that does not wrap shows past its end.
constexpr int Neighbour(int index, int count, int offset, bool circular)
{
    const int at = index + offset;
    if (circular) {
        return Wrap(at, count);
    }
    return at < 0 || at >= count ? -1 : at;
}

// What a tap does: on a closed roller, any row opens it; on an open one the row above steps up, the row below steps down and the
// middle row keeps the value.
enum class Tap { kOpen, kStepUp, kStepDown, kKeep };

constexpr Tap TapOn(bool open, int row)
{
    if (!open) {
        return Tap::kOpen;
    }
    return row < kMiddle ? Tap::kStepUp : row > kMiddle ? Tap::kStepDown : Tap::kKeep;
}

// The row a touch at `y` px down from the roller's top lands in, for rows of `row_px`; a touch past an end lands in the end row.
constexpr int RowAt(int y, int row_px)
{
    const int row = y / row_px;
    return y < 0 ? 0 : row >= kRows ? kRows - 1 : row;
}

// An edit: OK (or a tap) opens it on a value; stepping changes the value; OK keeps what it shows, and holding OK or Cancel puts
// back what it had when it opened. Keeping reports a change once, and only when the value is not the one it opened on.
struct Edit {
    bool open = false;
    int opened = 0;  // the value when OK opened it
};

constexpr void Open(Edit &edit, int index)
{
    edit.open = true;
    edit.opened = index;
}

// Closes it keeping `index`; true when that is a change to report.
constexpr bool Keep(Edit &edit, int index)
{
    const bool changed = edit.open && index != edit.opened;
    edit.open = false;
    return changed;
}

// Closes it putting back what it opened on, which is returned; nothing to report.
constexpr int Cancel(Edit &edit)
{
    edit.open = false;
    return edit.opened;
}

}  // namespace widgets::detail::roller
