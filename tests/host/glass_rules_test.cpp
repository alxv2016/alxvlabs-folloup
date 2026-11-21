// Host tests of the glass's rules (components/lvgl_display/include/glass_rules.h): the dither patterns, the fill
// codes and the back-to-back wait, run on the computer with no board (STICKY_DESIGN_TOKENS §4, §5).
// Build and run: bash tests/host/run.sh

#include <cstdio>

#include "glass_rules.h"

using namespace lvgl_display;

namespace {

int s_failures = 0;
int s_checks = 0;

void Check(bool ok, const char *what)
{
    ++s_checks;
    if (!ok) {
        ++s_failures;
        std::printf("FAIL  %s\n", what);
    }
}

// How many of a 2 x 2 cell's pixels draw white for one L8 value.
int WhiteInCell(uint8_t value)
{
    int white = 0;
    for (int row = 0; row < 2; ++row) {
        for (int column = 0; column < 2; ++column) {
            white += OneBitWhite(value, column, row) ? 1 : 0;
        }
    }
    return white;
}

void TestDither()
{
    Check(WhiteInCell(kDitherDark) == 1, "the dark dither draws one pixel in four white");
    Check(OneBitWhite(kDitherDark, 1, 1), "the dark dither's white pixel is at odd column, odd row");
    Check(WhiteInCell(kDitherLight) == 3, "the light dither draws three pixels in four white");
    Check(!OneBitWhite(kDitherLight, 0, 0), "the light dither's black pixel is at even column, even row");
    Check(kDitherDark == 0x55 + 1 && kDitherLight == 0xAA + 1, "each dither code is one step above its plain grey");
}

void TestThreshold()
{
    Check(WhiteInCell(0x00) == 0, "black draws black");
    Check(WhiteInCell(0x55) == 0, "plain dark grey, a text or line, thresholds to black");
    Check(WhiteInCell(0xAA) == 4, "plain light grey, a text or line, thresholds to white");
    Check(WhiteInCell(0xFF) == 4, "white draws white");
    Check(!OneBitWhite(127, 0, 0) && OneBitWhite(128, 0, 0), "the threshold sits at 128");
}

void TestFillCode()
{
    Check(FillCode(0) == 0x00, "a level 0 fill is black");
    Check(FillCode(1) == kDitherDark, "a level 1 fill is the dark dither");
    Check(FillCode(2) == kDitherLight, "a level 2 fill is the light dither");
    Check(FillCode(3) == 0xFF, "a level 3 fill is white");
    Check(FillCode(7) == 0xFF, "a level past 3 is white");
}

void TestFlashWait()
{
    Check(FlashWaitMs(false, 0, 1600) == 0, "nothing waits before the first full refresh");
    Check(FlashWaitMs(true, 0, 1600) == 1600, "a full asked for as the last ends waits its whole time");
    Check(FlashWaitMs(true, 400, 1000) == 600, "a full waits the rest of the last one's time");
    Check(FlashWaitMs(true, 1000, 1000) == 0, "a full asked for once the time has passed runs at once");
    Check(FlashWaitMs(true, 5000, 1000) == 0, "a full long after the last runs at once");
    Check(FlashWaitMs(true, -3, 1000) == 1000, "a clock read before the end never waits longer than the last took");
}

static_assert(OneBitWhite(kDitherDark, 1, 1) && !OneBitWhite(kDitherDark, 0, 0), "the rules are constexpr");

}  // namespace

int main()
{
    TestDither();
    TestThreshold();
    TestFillCode();
    TestFlashWait();
    std::printf("glass_rules: %d of %d checks pass\n", s_checks - s_failures, s_checks);
    return s_failures == 0 ? 0 : 1;
}
