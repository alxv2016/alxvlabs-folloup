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

// --- Type: Bitcount Single at one bit a pixel (STICKY_DESIGN_TOKENS §3) -----------------------------------------------

struct TypeRole {
    int16_t size_px;
    int16_t line_px;
    int16_t weight;
};

// The primitives: the scale, each size a multiple of 10 px so the font's cells — a tenth of the em — fall on whole
// pixels, its line 1.2 em; 100 px and up hold figures only.
inline constexpr TypeRole kTypographyScale20Semibold = {20, 24, 600};
inline constexpr TypeRole kTypographyScale20Bold = {20, 24, 700};
inline constexpr TypeRole kTypographyScale30Semibold = {30, 36, 600};
inline constexpr TypeRole kTypographyScale30Bold = {30, 36, 700};
inline constexpr TypeRole kTypographyScale40Semibold = {40, 48, 600};
inline constexpr TypeRole kTypographyScale40Bold = {40, 48, 700};
inline constexpr TypeRole kTypographyScale50Semibold = {50, 60, 600};
inline constexpr TypeRole kTypographyScale50Bold = {50, 60, 700};
inline constexpr TypeRole kTypographyScale60Semibold = {60, 72, 600};
inline constexpr TypeRole kTypographyScale60Bold = {60, 72, 700};
inline constexpr TypeRole kTypographyScale70Semibold = {70, 84, 600};
inline constexpr TypeRole kTypographyScale70Bold = {70, 84, 700};
inline constexpr TypeRole kTypographyScale80Semibold = {80, 96, 600};
inline constexpr TypeRole kTypographyScale80Bold = {80, 96, 700};
inline constexpr TypeRole kTypographyScale100Semibold = {100, 120, 600};
inline constexpr TypeRole kTypographyScale100Bold = {100, 120, 700};
inline constexpr TypeRole kTypographyScale120Semibold = {120, 144, 600};
inline constexpr TypeRole kTypographyScale120Bold = {120, 144, 700};
inline constexpr TypeRole kTypographyScale160Semibold = {160, 192, 600};
inline constexpr TypeRole kTypographyScale160Bold = {160, 192, 700};

// The roles: each one primitive (DESIGN → Typography → Scale and roles).
inline constexpr TypeRole kTypographyHero = kTypographyScale60Bold;
inline constexpr TypeRole kTypographyDisplay = kTypographyScale50Bold;
inline constexpr TypeRole kTypographyHeadline = kTypographyScale40Bold;
inline constexpr TypeRole kTypographyBody = kTypographyScale30Semibold;
inline constexpr TypeRole kTypographyLabel = kTypographyScale30Bold;
inline constexpr TypeRole kTypographyCaption = kTypographyScale20Semibold;

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
