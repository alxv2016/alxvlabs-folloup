// The glass's rules that need no hardware: the codes a grey fill is drawn with, which pixel of the one-bit frame each
// draws white, and how long a full refresh waits after the last one. Pure and constexpr, so tests/host checks them on
// the computer with no board (STICKY_DESIGN_TOKENS §4, §5).

#pragma once

#include <cstdint>

namespace lvgl_display {

// A grey fill's code in LVGL's L8 frame: one step above the plain grey, so the one-bit frame draws it as a dither —
// a fixed black-and-white pattern locked to the glass's pixels — that every partial refresh keeps, while a grey text
// or line of the plain level still thresholds to solid black or white. A greyscale refresh reads both as the grey
// (DESIGN → Colors → The four levels; STICKY_DESIGN_TOKENS §4).
inline constexpr uint8_t kDitherDark = 0x56;   // #555555's fill: one pixel in four white
inline constexpr uint8_t kDitherLight = 0xAB;  // #AAAAAA's fill: three pixels in four white

// The two sparse dithers a disabled control's words are drawn in, lighter than the light dither: one black pixel in
// sixteen on the control's face, one in eight on the white page (DESIGN → Components → States → Disabled). They are no
// panel level — a greyscale refresh reads them, like any code, by its top two bits, as white. Their codes sit mid-range
// in that band, away from white and from the plain greys, so a rounded edge's blend rarely lands on one.
inline constexpr uint8_t kDitherSixteenth = 0xC1;  // one pixel in sixteen black: a disabled control's words on its face
inline constexpr uint8_t kDitherEighth = 0xC2;     // one pixel in eight black: a disabled row's name on the page

// The levels of the token header past the panel's four: the two sparse dithers, fills (and so text colours) only.
inline constexpr uint8_t kLevelSparseSixteenth = 4;
inline constexpr uint8_t kLevelSparseEighth = 5;

// The L8 code a fill of `level` (0 black … 3 white, then the two sparse dithers) is drawn with: levels 1 and 2 as their
// dithers.
constexpr uint8_t FillCode(uint8_t level)
{
    return level == 1                      ? kDitherDark
           : level == 2                    ? kDitherLight
           : level == kLevelSparseSixteenth ? kDitherSixteenth
           : level == kLevelSparseEighth    ? kDitherEighth
           : level > 3                     ? uint8_t{0xFF}
                                           : static_cast<uint8_t>(level * 0x55);
}

// Whether an L8 pixel draws white in the one-bit frame, given its column and row on the panel (the frame's own pixel,
// so a pattern never moves with what is drawn over it): a dither code by its pattern, anything else by a threshold at
// the middle. Every pattern's black pixels are among the light dither's, so words in a sparse dither are the face with
// most of its dots taken out.
constexpr bool OneBitWhite(uint8_t value, int column, int row)
{
    if (value == kDitherDark) {
        return (column & 1) == 1 && (row & 1) == 1;  // one in four
    }
    if (value == kDitherLight) {
        return !((column & 1) == 0 && (row & 1) == 0);  // three in four
    }
    if (value == kDitherEighth) {
        return !((column & 3) == 0 && (row & 1) == 0);  // seven in eight
    }
    if (value == kDitherSixteenth) {
        return !((column & 3) == 0 && (row & 3) == 0);  // fifteen in sixteen
    }
    return value >= 128;
}

// Which partial refreshes count toward the limit that forces a full one (DESIGN → Motion; STICKY_DESIGN_TOKENS §5): every one,
// or — while the keyboard is open — only the frames a typing press caused, so the ring moving between keys and rows draws
// partials that do not count — up to the ceiling below.
enum class PartialCounting : uint8_t { kEvery, kTypingOnly };

// How many partials a frame that took `needed` adds to the count: all of them, or none when only typing counts and the frame
// is not a typing press's.
constexpr uint32_t PartialsCounted(PartialCounting counting, bool typed, uint32_t needed)
{
    return counting == PartialCounting::kTypingOnly && !typed ? 0 : needed;
}

// Whether a frame that counts `counted` partials is drawn full, because they would take the count since the last full
// refresh past `limit`. A frame that counts none is never forced full.
constexpr bool PartialsOverLimit(uint32_t partials, uint32_t counted, uint32_t limit)
{
    return counted != 0 && partials + counted > limit;
}

// The ceiling on the partials that do not count: while only typing counts, a ring move and a page switch's frame add to a second
// count, and the frame that would take it past this many is drawn full, so the dither and the rings of a long walk never build up
// unseen. A page switch (Shift, 123, ABC, #+=) is not among them: it counts as one partial, like a typing press. The 60 is a start,
// to be tuned on the glass (STICKY_DESIGN_TOKENS §5).
inline constexpr uint32_t kUncountedPartialLimit = 60;

// How long a full or grey refresh waits so two never run back to back: the rest of the last one's own time since it
// ended, or nothing when none has run (STICKY_DESIGN_TOKENS §5).
constexpr uint32_t FlashWaitMs(bool flashed_before, int64_t since_ms, uint32_t flash_ms)
{
    if (!flashed_before || since_ms >= static_cast<int64_t>(flash_ms)) {
        return 0;
    }
    return since_ms <= 0 ? flash_ms : flash_ms - static_cast<uint32_t>(since_ms);
}

}  // namespace lvgl_display
