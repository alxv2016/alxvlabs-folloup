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

// The L8 code a fill of `level` (0 black … 3 white) is drawn with: levels 1 and 2 as their dithers.
constexpr uint8_t FillCode(uint8_t level)
{
    return level == 1   ? kDitherDark
           : level == 2 ? kDitherLight
           : level > 3  ? uint8_t{0xFF}
                        : static_cast<uint8_t>(level * 0x55);
}

// Whether an L8 pixel draws white in the one-bit frame, given the parity of its column and row on the panel: a dither
// code by its pattern, anything else by a threshold at the middle.
constexpr bool OneBitWhite(uint8_t value, int column_parity, int row_parity)
{
    if (value == kDitherDark) {
        return column_parity == 1 && row_parity == 1;  // one in four
    }
    if (value == kDitherLight) {
        return !(column_parity == 0 && row_parity == 0);  // three in four
    }
    return value >= 128;
}

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
