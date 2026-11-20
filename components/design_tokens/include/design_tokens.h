// Every token in DESIGN.md's front matter, by role, as the firmware uses it (STICKY_DESIGN_TOKENS §6). A value changes
// in DESIGN.md first, then here — never only here. A token's path is its symbol: colors.text-primary →
// kColorTextPrimary. A colour is its panel level, 0 to 3, with the hex it stands for in a comment
// (STICKY_DESIGN_TOKENS §2.1).

#pragma once

#include <cstdint>

namespace design_tokens {

// --- Colours: levels 0 (black) to 3 (white); a partial refresh shows 0 and 1 black, 2 and 3 white ------------------

inline constexpr uint8_t kColorScalePrimary0 = 0;  // #000000
inline constexpr uint8_t kColorScalePrimary1 = 1;  // #555555
inline constexpr uint8_t kColorScalePrimary2 = 2;  // #AAAAAA
inline constexpr uint8_t kColorScalePrimary3 = 3;  // #FFFFFF
inline constexpr uint8_t kColorBlack = kColorScalePrimary0;
inline constexpr uint8_t kColorWhite = kColorScalePrimary3;

inline constexpr uint8_t kColorBackground = kColorWhite;       // #FFFFFF
inline constexpr uint8_t kColorSurfaceAction = kColorBlack;    // #000000
inline constexpr uint8_t kColorSurfaceChosen = kColorBlack;    // #000000
inline constexpr uint8_t kColorSurfaceOn = kColorBlack;        // #000000
inline constexpr uint8_t kColorTextPrimary = kColorBlack;      // #000000
inline constexpr uint8_t kColorTextSecondary = kColorScalePrimary1;  // #555555
inline constexpr uint8_t kColorTextOnFill = kColorWhite;       // #FFFFFF
inline constexpr uint8_t kColorOutline = kColorBlack;          // #000000
inline constexpr uint8_t kColorDivider = kColorScalePrimary1;  // #555555
inline constexpr uint8_t kColorFocusRing = kColorBlack;        // #000000
inline constexpr uint8_t kColorFocusRingOnFill = kColorWhite;  // #FFFFFF
inline constexpr uint8_t kColorBand = kColorScalePrimary2;     // #AAAAAA

// --- Type: Inter, each role's size, line box and weight (STICKY_DESIGN_TOKENS §3) ------------------------------------

struct TypeRole {
    int16_t size_px;
    int16_t line_px;
    int16_t weight;
};

inline constexpr TypeRole kTypographyHero = {165, 168, 700};
inline constexpr TypeRole kTypographyDisplay = {55, 64, 700};
inline constexpr TypeRole kTypographyHeadline = {46, 54, 700};
inline constexpr TypeRole kTypographyBody = {32, 42, 600};
inline constexpr TypeRole kTypographyLabel = {26, 34, 700};
inline constexpr TypeRole kTypographyCaption = {22, 30, 600};

// --- Radius -------------------------------------------------------------------------------------------------------

inline constexpr int16_t kRoundedNone = 0;
inline constexpr int16_t kRoundedSm = 4;
inline constexpr int16_t kRoundedMd = 8;
inline constexpr int16_t kRoundedFull = 9999;

// --- Spacing, in panel pixels ------------------------------------------------------------------------------------------

inline constexpr int16_t kSpacingXs = 4;
inline constexpr int16_t kSpacingSm = 8;
inline constexpr int16_t kSpacingMd = 16;
inline constexpr int16_t kSpacingLg = 24;
inline constexpr int16_t kSpacingXl = 32;
inline constexpr int16_t kSpacing2xl = 48;
inline constexpr int16_t kSpacingEdge = 24;
inline constexpr int16_t kSpacingFocusOffset = 4;
inline constexpr int16_t kSpacingTarget = 64;
inline constexpr int16_t kSpacingContent = 432;
inline constexpr int16_t kSpacingStatusBar = 44;
inline constexpr int16_t kSpacingFooter = 80;
inline constexpr int16_t kSpacingListRowHeight = 80;
inline constexpr int16_t kSpacingToggleHeight = 44;
inline constexpr int16_t kSpacingOverlayOffset = 6;
inline constexpr int16_t kSpacingStrokeHairline = 2;
inline constexpr int16_t kSpacingStrokeDefault = 3;
inline constexpr int16_t kSpacingStrokeFocus = 4;
inline constexpr int16_t kSpacingStrokeStrong = 4;

// --- Motion: nothing animates; the delays are what the code waits (DESIGN → Motion) --------------------------------

inline constexpr uint32_t kMotionStateChangeMs = 0;
inline constexpr uint32_t kMotionSurfaceEnterMs = 0;
inline constexpr uint32_t kMotionSurfaceExitMs = 0;
inline constexpr uint32_t kMotionWaitIndicatorDelayMs = 1000;
inline constexpr uint32_t kMotionSelfUpdateDelayMs = 5000;
inline constexpr uint32_t kMotionClockTickDelayMs = 60000;

// --- Refresh and feedback (STICKY_DESIGN_TOKENS §5) ------------------------------------------------------------------

inline constexpr int kRefreshPartialLimit = 20;
inline constexpr uint32_t kFeedbackTickMs = 100;
inline constexpr uint32_t kWaitIndicatorDelayMs = kMotionWaitIndicatorDelayMs;
inline constexpr uint32_t kSelfUpdateMinMs = kMotionSelfUpdateDelayMs;
inline constexpr uint32_t kClockTickMs = kMotionClockTickDelayMs;

}  // namespace design_tokens
