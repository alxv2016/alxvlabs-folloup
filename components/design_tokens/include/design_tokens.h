// Every token in DESIGN.md's front matter, by role, as the firmware uses it (STICKY_DESIGN_TOKENS §6). A value changes
// in DESIGN.md first, then here — never only here. A token's path is its symbol: colors.text-primary →
// kColorTextPrimary. A colour is its panel level, 0 to 3, with the hex it stands for in a comment
// (STICKY_DESIGN_TOKENS §2.1).

#pragma once

#include <cstdint>

namespace design_tokens {

// --- Colours: levels 0 (black) to 3 (white); a partial refresh shows 0 and 1 black, 2 and 3 white ------------------
// Levels 4 and 5 are no panel level: the two sparse dithers, one black pixel in sixteen and in eight, which only a
// disabled control's words are drawn in; `ui::Fill` gives their codes (STICKY_DESIGN_TOKENS §2.1, §4).

inline constexpr uint8_t kColorScalePrimary0 = 0;  // #000000
inline constexpr uint8_t kColorScalePrimary1 = 1;  // #555555
inline constexpr uint8_t kColorScalePrimary2 = 2;  // #AAAAAA
inline constexpr uint8_t kColorScalePrimary3 = 3;  // #FFFFFF
inline constexpr uint8_t kColorScaleSparseSixteenth = 4;  // #F8F8F8 — one black pixel in sixteen, in linear light
inline constexpr uint8_t kColorScaleSparseEighth = 5;     // #F0F0F0 — one black pixel in eight, in linear light
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
inline constexpr uint8_t kColorBand = kColorScalePrimary2;     // #AAAAAA
inline constexpr uint8_t kColorShadow1 = kColorScaleSparseEighth;  // #F0F0F0 — level 1's offset: one black pixel in eight
inline constexpr uint8_t kColorShadow2 = kColorScalePrimary2;      // #AAAAAA — level 2's offset: the light dither
inline constexpr uint8_t kColorShadow3 = kColorScalePrimary1;      // #555555 — level 3's offset: the dark dither
inline constexpr uint8_t kColorKeyline = kColorWhite;              // #FFFFFF — the line round an offset's outside
inline constexpr uint8_t kColorThumb = kColorBlack;            // #000000
inline constexpr uint8_t kColorThumbOn = kColorWhite;          // #FFFFFF
inline constexpr uint8_t kColorKnob = kColorWhite;             // #FFFFFF
inline constexpr uint8_t kColorTextDisabled = kColorScaleSparseSixteenth;      // #F8F8F8
inline constexpr uint8_t kColorTextDisabledPage = kColorScaleSparseEighth;     // #F0F0F0

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
// Caption in bold: a Wi-Fi network's name, a choice card's name.
inline constexpr TypeRole kTypographyCaptionStrong = kTypographyScale20Bold;
// Caption on a loose line, 1.5 em: a text view's lines, the deck's airy 24 on 16 (DESIGN → Typography → Scale and roles).
inline constexpr TypeRole kTypographyCaptionLoose = {20, 30, 600};

// --- Radius -------------------------------------------------------------------------------------------------------

inline constexpr int16_t kRoundedNone = 0;
inline constexpr int16_t kRoundedSm = 4;
inline constexpr int16_t kRoundedMd = 8;
inline constexpr int16_t kRoundedLg = 12;
inline constexpr int16_t kRoundedXl = 16;
inline constexpr int16_t kRounded2xl = 32;
inline constexpr int16_t kRoundedFull = 9999;

// --- Elevation: how high a surface stands off the page (STICKY_DESIGN_TOKENS §2.1; DESIGN → Elevation & Depth) ----------------

struct ElevationLevel {
    int16_t outline_px;  // the edge of a raised surface
    uint8_t shadow;      // the colour level of its offset's dither; kNoShadow where it has none
    int16_t offset_px;   // how far the offset stands from the box
    int16_t keyline_px;  // the white line round the offset's outer edge
};
inline constexpr uint8_t kNoShadow = 0xFF;

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
inline constexpr int16_t kSpacingFooter = 96;
inline constexpr int16_t kSpacingListRowHeight = 80;
inline constexpr int16_t kSpacingToggleHeight = 44;
inline constexpr int16_t kSpacingToggleWidth = 96;
inline constexpr int16_t kSpacingToggleThumbWidth = 52;
inline constexpr int16_t kSpacingSliderHeight = 52;
inline constexpr int16_t kSpacingButtonWidth = 212;
inline constexpr int16_t kSpacingCardInset = 20;
inline constexpr int16_t kSpacingCheckSize = 44;
inline constexpr int16_t kSpacingCheckDot = 16;
inline constexpr int16_t kSpacingIcon = 40;
inline constexpr int16_t kSpacingMark = 42;
inline constexpr int16_t kSpacingMarkDot = 26;
inline constexpr int16_t kSpacingProgress = 12;
inline constexpr int16_t kSpacingProgressLead = 16;
inline constexpr int16_t kSpacingStepGap = 12;
inline constexpr int16_t kSpacingPageDot = 8;
inline constexpr int16_t kSpacingPagePill = 28;
inline constexpr int16_t kSpacingScrollThumb = 8;
inline constexpr int16_t kSpacingScrollLength = 248;
inline constexpr int16_t kSpacingScrollX = 467;
inline constexpr int16_t kSpacingStatusMark = 85;
inline constexpr int16_t kSpacingKeyGap = 12;
inline constexpr int16_t kSpacingOverlayOffset = 6;
inline constexpr int16_t kSpacingStrokeHairline = 2;
inline constexpr int16_t kSpacingStrokeDefault = 3;
inline constexpr int16_t kSpacingStrokeFocus = 4;
inline constexpr int16_t kSpacingStrokeStrong = 4;
inline constexpr int16_t kSpacingStrokeEdit = 6;

// The scale: a level points at roles and spacing, a surface at a level. 0 flat — on the page; 1 raised — floats over the page and
// never takes it over (no surface yet); 2 anchored — a bottom sheet; 3 floating — a dialog. Denser is higher; one distance for all.
inline constexpr ElevationLevel kElevation0 = {kSpacingStrokeDefault, kNoShadow, 0, 0};
inline constexpr ElevationLevel kElevation1 = {kSpacingStrokeStrong, kColorShadow1, kSpacingOverlayOffset, kSpacingStrokeHairline};
inline constexpr ElevationLevel kElevation2 = {kSpacingStrokeStrong, kColorShadow2, kSpacingOverlayOffset, kSpacingStrokeHairline};
inline constexpr ElevationLevel kElevation3 = {kSpacingStrokeStrong, kColorShadow3, kSpacingOverlayOffset, kSpacingStrokeHairline};

// --- Touch: how a swipe turns a page (DESIGN → Focus and input → Buttons and touch; STICKY_DESIGN_TOKENS §10 owns the
// distances and the 100 ms window, `[?]`: proposed, to be judged on the glass, STICKY_DESIGN_TOKENS §8) ------------------

inline constexpr int16_t kTouchSwipeDistance = kSpacingTarget;  // px of travel that commits a page turn: one target, 64
inline constexpr int16_t kTouchFlickDistance = 24;       // px a flick must have travelled at least `[?]`
inline constexpr int16_t kTouchFlickMovement = 12;       // px moved in the last kTouchFlickWindowMs that make it a flick `[?]`
inline constexpr uint32_t kTouchFlickWindowMs = 100;     // the end of the drag a flick is read from `[?]`

// --- Motion: nothing animates; the delays are what the code waits (DESIGN → Motion) --------------------------------

inline constexpr uint32_t kMotionStateChangeMs = 0;
inline constexpr uint32_t kMotionSurfaceEnterMs = 0;
inline constexpr uint32_t kMotionSurfaceExitMs = 0;
inline constexpr uint32_t kMotionWaitIndicatorDelayMs = 1000;
inline constexpr uint32_t kMotionSelfUpdateDelayMs = 5000;
inline constexpr uint32_t kMotionClockTickDelayMs = 60000;
inline constexpr uint32_t kMotionPasswordRevealDelayMs = 1500;  // a password's last typed character shows this long, then a bullet

// --- Refresh and feedback (STICKY_DESIGN_TOKENS §5) ------------------------------------------------------------------

inline constexpr int kRefreshPartialLimit = 20;
inline constexpr uint32_t kFeedbackTickMs = 100;
inline constexpr uint32_t kWaitIndicatorDelayMs = kMotionWaitIndicatorDelayMs;
inline constexpr uint32_t kSelfUpdateMinMs = kMotionSelfUpdateDelayMs;
inline constexpr uint32_t kClockTickMs = kMotionClockTickDelayMs;

}  // namespace design_tokens
