// Host tests of the glass's rules (components/lvgl_display/include/glass_rules.h): the dither patterns, the fill
// codes and the back-to-back wait, run on the computer with no board (STICKY_DESIGN_TOKENS §4, §5). A dither is
// checked over a whole 4 x 4 cell of the panel's pixels, the period of the sparsest pattern.
// Build and run: bash tests/host/run.sh

#include <cstdio>
#include <initializer_list>

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

// How many of a 4 x 4 cell's sixteen pixels draw white for one L8 value.
int WhiteInCell(uint8_t value)
{
    int white = 0;
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            white += OneBitWhite(value, column, row) ? 1 : 0;
        }
    }
    return white;
}

void TestDither()
{
    Check(WhiteInCell(kDitherDark) == 4, "the dark dither draws one pixel in four white");
    Check(OneBitWhite(kDitherDark, 1, 1), "the dark dither's white pixel is at odd column, odd row");
    Check(WhiteInCell(kDitherLight) == 12, "the light dither draws three pixels in four white");
    Check(!OneBitWhite(kDitherLight, 0, 0), "the light dither's black pixel is at even column, even row");
    Check(kDitherDark == 0x55 + 1 && kDitherLight == 0xAA + 1, "each dither code is one step above its plain grey");
}

// The two sparse dithers of a disabled control's words: one black pixel in eight on the page, in sixteen on a face.
void TestSparseDither()
{
    Check(WhiteInCell(kDitherEighth) == 14, "the eighth dither draws seven pixels in eight white");
    Check(WhiteInCell(kDitherSixteenth) == 15, "the sixteenth dither draws fifteen pixels in sixteen white");
    Check(!OneBitWhite(kDitherEighth, 0, 0) && !OneBitWhite(kDitherEighth, 4, 2) && OneBitWhite(kDitherEighth, 2, 0) &&
              OneBitWhite(kDitherEighth, 0, 1),
          "the eighth dither's black pixel is at every fourth column, every second row");
    Check(!OneBitWhite(kDitherSixteenth, 0, 0) && !OneBitWhite(kDitherSixteenth, 4, 4) && OneBitWhite(kDitherSixteenth, 0, 2) &&
              OneBitWhite(kDitherSixteenth, 2, 0),
          "the sixteenth dither's black pixel is at every fourth column and row");
    // Every black pixel of a sparse pattern is one of the light dither's: words in it are the face with dots taken out, never
    // a dot where the face has none.
    bool subset = true;
    for (int row = 0; row < 8; ++row) {
        for (int column = 0; column < 8; ++column) {
            for (uint8_t sparse : {kDitherEighth, kDitherSixteenth}) {
                if (!OneBitWhite(sparse, column, row) && OneBitWhite(kDitherLight, column, row)) {
                    subset = false;
                }
            }
        }
    }
    Check(subset, "every sparse black pixel is also a light-dither black pixel");
    Check(WhiteInCell(kDitherSixteenth) > WhiteInCell(kDitherEighth) && WhiteInCell(kDitherEighth) > WhiteInCell(kDitherLight),
          "the patterns run lighter: dark, light, eighth, sixteenth");
    Check(kDitherEighth >= 0xC0 && kDitherSixteenth >= 0xC0, "a greyscale refresh reads both as white (top two bits 3)");
    Check(kDitherEighth != kDitherSixteenth && kDitherEighth != 0xFF && kDitherSixteenth != 0xFF,
          "each code is its own, and neither is white");
}

void TestThreshold()
{
    Check(WhiteInCell(0x00) == 0, "black draws black");
    Check(WhiteInCell(0x55) == 0, "plain dark grey, a text or line, thresholds to black");
    Check(WhiteInCell(0xAA) == 16, "plain light grey, a text or line, thresholds to white");
    Check(WhiteInCell(0xFF) == 16, "white draws white");
    Check(WhiteInCell(0xC0) == 16 && WhiteInCell(0xC3) == 16, "the codes beside the sparse ones threshold to white");
    Check(!OneBitWhite(127, 0, 0) && OneBitWhite(128, 0, 0), "the threshold sits at 128");
}

void TestFillCode()
{
    Check(FillCode(0) == 0x00, "a level 0 fill is black");
    Check(FillCode(1) == kDitherDark, "a level 1 fill is the dark dither");
    Check(FillCode(2) == kDitherLight, "a level 2 fill is the light dither");
    Check(FillCode(3) == 0xFF, "a level 3 fill is white");
    Check(FillCode(kLevelSparseSixteenth) == kDitherSixteenth, "level 4 is the sixteenth dither");
    Check(FillCode(kLevelSparseEighth) == kDitherEighth, "level 5 is the eighth dither");
    Check(FillCode(9) == 0xFF, "a level past the sparse two is white");
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

// A run of frames on the partial counter the way display.cpp's Send keeps it: `partials` since the last full refresh, each
// frame typed or not, `needed` 1 (a V2.0 frame) or 2 (a V1.0 page change). Returns how many were drawn full.
struct Counter {
    uint32_t partials = 0;
    uint32_t uncounted = 0;  // the ceiling's count: the frames that did not count
    uint32_t fulls = 0;
    PartialCounting counting = PartialCounting::kEvery;
    void Frame(bool typed, uint32_t needed = 1)
    {
        const uint32_t counted = PartialsCounted(counting, typed, needed);
        const uint32_t not_counted = needed - counted;
        if (PartialsOverLimit(partials, counted, 20) || PartialsOverLimit(uncounted, not_counted, kUncountedPartialLimit)) {
            ++fulls;
            partials = 0;
            uncounted = 0;
        } else {
            partials += counted;
            uncounted += not_counted;
        }
    }
};

void TestPartialCounting()
{
    Check(PartialsCounted(PartialCounting::kEvery, false, 1) == 1 && PartialsCounted(PartialCounting::kEvery, true, 2) == 2,
          "every frame counts what it needed by default, typed or not");
    Check(PartialsCounted(PartialCounting::kTypingOnly, true, 1) == 1 && PartialsCounted(PartialCounting::kTypingOnly, true, 2) == 2,
          "a typing press's frame counts all it needed while only typing counts");
    Check(PartialsCounted(PartialCounting::kTypingOnly, false, 1) == 0 && PartialsCounted(PartialCounting::kTypingOnly, false, 2) == 0,
          "a ring move or a page switch counts nothing while only typing counts");
    Check(!PartialsOverLimit(19, 1, 20) && !PartialsOverLimit(20, 0, 20), "the 20th counts, and a frame counting none is never full");
    Check(PartialsOverLimit(20, 1, 20) && PartialsOverLimit(19, 2, 20) && !PartialsOverLimit(18, 2, 20),
          "the 21st partial is drawn full, and a V1.0 page change that would pass the limit with its two");

    Counter every;  // the rule as it stood: 20 partials, then the next is full
    for (int i = 0; i < 21; ++i) {
        every.Frame(false);
    }
    Check(every.fulls == 1 && every.partials == 0, "with every frame counting, the 21st frame is the first full");

    Counter typing;
    typing.counting = PartialCounting::kTypingOnly;
    for (int i = 0; i < static_cast<int>(kUncountedPartialLimit); ++i) {
        typing.Frame(false);  // sixty ring moves, rows entered and left
    }
    Check(typing.fulls == 0 && typing.partials == 0, "ring moves in the keyboard do not count toward the twenty, nor force a full up to the ceiling");
    typing.Frame(false);
    Check(typing.fulls == 1 && typing.uncounted == 0, "the 61st uncounted partial is drawn full, and the full resets both counts");
    for (int i = 0; i < static_cast<int>(kUncountedPartialLimit); ++i) {
        typing.Frame(false);
    }
    typing.Frame(true);
    Check(typing.fulls == 1 && typing.partials == 1 && typing.uncounted == 60, "a typing press counts as one and adds nothing to the ceiling");
    typing = Counter{};  // fresh for the walk below
    typing.counting = PartialCounting::kTypingOnly;
    for (int i = 0; i < 20; ++i) {
        typing.Frame(true);
        typing.Frame(false);  // each typed character is followed by a ring move
    }
    Check(typing.fulls == 0 && typing.partials == 20, "twenty typed characters, with their ring moves, are the twenty");
    typing.Frame(false);
    Check(typing.fulls == 0, "a ring move after the twentieth character is still a partial");
    typing.Frame(true);
    Check(typing.fulls == 1 && typing.partials == 0, "the 21st typed character is drawn full");

    Counter pages;  // a page switch (Shift, 123, ABC, #+=) counts as one partial, a typing press's: the 21st is drawn full
    pages.counting = PartialCounting::kTypingOnly;
    for (int i = 0; i < 20; ++i) {
        pages.Frame(true);  // the keyboard says NoteTyping for it; a switch redraws on the same screen, so needs one
    }
    Check(pages.fulls == 0 && pages.partials == 20, "twenty page switches count twenty, and no ceiling count");
    pages.Frame(true);
    Check(pages.fulls == 1 && pages.partials == 0, "the 21st page switch is drawn full");
}

static_assert(PartialsCounted(PartialCounting::kTypingOnly, false, 2) == 0 && !PartialsOverLimit(20, 0, 20),
              "the counting rules are constexpr");
static_assert(kUncountedPartialLimit == 60 && PartialsOverLimit(60, 1, kUncountedPartialLimit) && !PartialsOverLimit(59, 1, kUncountedPartialLimit),
              "the ceiling is 60 uncounted partials");
static_assert(OneBitWhite(kDitherDark, 1, 1) && !OneBitWhite(kDitherDark, 0, 0), "the rules are constexpr");

}  // namespace

int main()
{
    TestDither();
    TestSparseDither();
    TestThreshold();
    TestFillCode();
    TestFlashWait();
    TestPartialCounting();
    std::printf("glass_rules: %d of %d checks pass\n", s_checks - s_failures, s_checks);
    return s_failures == 0 ? 0 : 1;
}
