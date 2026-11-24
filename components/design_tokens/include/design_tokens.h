// Every token in DESIGN.md's front matter, by role, as the firmware uses it (STICKY_DESIGN_TOKENS §6). A value changes
// in DESIGN.md first, then here — never only here. A token's path is its symbol: colors.text-primary →
// kColorTextPrimary. A colour is its panel level, 0 to 3, with the hex it stands for in a comment
// (STICKY_DESIGN_TOKENS §2.1).

#pragma once

#include <cstddef>
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
// pixels. Its line is 1.2 em to 40 px, then tapers, each step on the 4 px grid and never under the font's own height:
// 56 at 50, 64 at 60, 72 at 70, 80 at 80. 100 px and up hold figures only, on the height of their digits
// (STICKY_DESIGN_TOKENS §3; tests/host/line_floor_lvgl_test.cpp holds the floor).
inline constexpr TypeRole kTypographyScale20Semibold = {20, 24, 600};
inline constexpr TypeRole kTypographyScale20Bold = {20, 24, 700};
inline constexpr TypeRole kTypographyScale30Regular = {30, 36, 400};  // the only regular weight
inline constexpr TypeRole kTypographyScale30Semibold = {30, 36, 600};
inline constexpr TypeRole kTypographyScale30Bold = {30, 36, 700};
inline constexpr TypeRole kTypographyScale40Semibold = {40, 48, 600};
inline constexpr TypeRole kTypographyScale40Bold = {40, 48, 700};
inline constexpr TypeRole kTypographyScale50Semibold = {50, 56, 600};
inline constexpr TypeRole kTypographyScale50Bold = {50, 56, 700};
inline constexpr TypeRole kTypographyScale60Semibold = {60, 64, 600};
inline constexpr TypeRole kTypographyScale60Bold = {60, 64, 700};
inline constexpr TypeRole kTypographyScale70Semibold = {70, 72, 600};
inline constexpr TypeRole kTypographyScale70Bold = {70, 72, 700};
inline constexpr TypeRole kTypographyScale80Semibold = {80, 80, 600};
inline constexpr TypeRole kTypographyScale80Bold = {80, 80, 700};
inline constexpr TypeRole kTypographyScale100Semibold = {100, 68, 600};
inline constexpr TypeRole kTypographyScale100Bold = {100, 68, 700};
inline constexpr TypeRole kTypographyScale120Semibold = {120, 80, 600};
inline constexpr TypeRole kTypographyScale120Bold = {120, 80, 700};
inline constexpr TypeRole kTypographyScale160Semibold = {160, 108, 600};
inline constexpr TypeRole kTypographyScale160Bold = {160, 108, 700};

// The roles: each one primitive (DESIGN → Typography → Scale and roles).
inline constexpr TypeRole kTypographyHero = kTypographyScale60Bold;
inline constexpr TypeRole kTypographyDisplay = kTypographyScale50Bold;
inline constexpr TypeRole kTypographyHeadline = kTypographyScale40Bold;
inline constexpr TypeRole kTypographyBody = {kTypographyScale30Regular.size_px, 40, kTypographyScale30Regular.weight};  // scale-30-regular on 40
inline constexpr TypeRole kTypographyLabel = kTypographyScale30Bold;
inline constexpr TypeRole kTypographyCaption = kTypographyScale20Semibold;
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
inline constexpr int16_t kSpacingEdge = kSpacingLg;
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
inline constexpr int16_t kSpacingStatusMarkSlot = 73;  // the widest compiled mark: every block's words stand at one x
inline constexpr int16_t kSpacingKeyGap = 12;
// The role tokens: the gaps between texts and between whole components, each one step of the scale (DESIGN → Layout → Spacing).
inline constexpr int16_t kSpacingPairSm = kSpacingSm;   // a row's label over its one line
inline constexpr int16_t kSpacingPairMd = kSpacingMd;   // a title over a wrapped description; a headline over body
inline constexpr int16_t kSpacingPairLg = kSpacingLg;   // a display or a hero over body
inline constexpr int16_t kSpacingPairXl = kSpacingXl;   // a modal's title over its message
inline constexpr int16_t kSpacingItem = kSpacingMd;     // row to row, button to button
inline constexpr int16_t kSpacingGroup = kSpacingLg;    // a dialog's message to its buttons; a group's name under a heading
inline constexpr int16_t kSpacingSection = kSpacing2xl; // above a section heading
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
// distances and the 100 ms window, held under a finger on a V1.0 and a V2.0) -------------------------------------------

inline constexpr int16_t kTouchSwipeDistance = kSpacingTarget;  // px of travel that commits a page turn: one target, 64
inline constexpr int16_t kTouchFlickDistance = 24;       // px a flick must have travelled at least
inline constexpr int16_t kTouchFlickMovement = 12;       // px moved in the last kTouchFlickWindowMs that make it a flick
inline constexpr uint32_t kTouchFlickWindowMs = 100;     // the end of the drag a flick is read from

// --- Motion: nothing animates; the delays are what the code waits (DESIGN → Motion) --------------------------------

inline constexpr uint32_t kMotionStateChangeMs = 0;
inline constexpr uint32_t kMotionSurfaceEnterMs = 0;
inline constexpr uint32_t kMotionSurfaceExitMs = 0;
inline constexpr uint32_t kMotionWaitIndicatorDelayMs = 1000;
inline constexpr uint32_t kMotionSelfUpdateDelayMs = 5000;
inline constexpr uint32_t kMotionClockTickDelayMs = 60000;
inline constexpr uint32_t kMotionPasswordRevealDelayMs = 1500;  // a password's last typed character shows this long, then a bullet
inline constexpr uint32_t kMotionKeyRepeatDelayMs = 600;   // a held UP or DOWN steps again once held this long
inline constexpr uint32_t kMotionKeyRepeatPeriodMs = 500;  // then this often while held, each step's frame landed first

// --- Refresh and feedback (STICKY_DESIGN_TOKENS §5) ------------------------------------------------------------------

inline constexpr int kRefreshPartialLimit = 20;
inline constexpr uint32_t kFeedbackTickMs = 100;
inline constexpr uint32_t kWaitIndicatorDelayMs = kMotionWaitIndicatorDelayMs;
inline constexpr uint32_t kSelfUpdateMinMs = kMotionSelfUpdateDelayMs;
inline constexpr uint32_t kClockTickMs = kMotionClockTickDelayMs;

// --- Sound: the buzzer's words, in the agent deck's note format (DESIGN → Accessibility; STICKY_DESIGN_TOKENS §5) ------
// A note is {wave, hz, hz_end, ms, volume}: where it starts (0 a rest), where it slides to by its end (0 for none), its
// length, and on (255) or off (0) — the buzzer has no level but its duty. devices' note player plays them
// (STICKY_HARDWARE_SPECS §4.2).

enum class SoundWave : uint8_t {
    kPulse50,   // a square: the loudest at a pitch
    kPulse25,   // brighter, about 3 dB quieter
    kPulse12,   // the thinnest, about 8 dB quieter
    kTriangle,  // played as kPulse50: a switched coil has no triangle
    kNoise,     // the pitch hopping at random about `hz`
};

struct SoundNote {
    SoundWave wave;
    uint16_t hz;
    uint16_t hz_end;
    uint16_t ms;
    uint8_t volume;
};

inline constexpr uint32_t kSoundBandLowHz = 2000;   // sound.band-low: every sound is written at or above it
inline constexpr uint32_t kSoundBandHighHz = 6000;  // sound.band-high: and at or below it — round the 4 kHz resonance
inline constexpr uint32_t kSoundTickMaxMs = 40;     // sound.tick-max: a tick, on every press and every repeated step
inline constexpr uint32_t kSoundMaxMs = 600;        // sound.max: any other sound

// sound.tick: every press, and every repeated step of a held key — the full square at the resonance, 30 ms.
inline constexpr SoundNote kSoundTick[] = {{SoundWave::kPulse50, 4000, 0, 30, 255}};
// sound.at-end: a step that can go no further — two ticks, lower than the press's (E7).
inline constexpr SoundNote kSoundAtEnd[] = {
    {SoundWave::kPulse50, 2637, 0, 25, 255}, {SoundWave::kPulse50, 0, 0, 45, 0}, {SoundWave::kPulse50, 2637, 0, 25, 255}};
// sound.error: that did not work — the agent deck's "womp womp" in C (C8, A♭7, F7 sliding to C7).
inline constexpr SoundNote kSoundError[] = {
    {SoundWave::kPulse50, 4186, 0, 150, 255}, {SoundWave::kPulse50, 0, 0, 50, 0},
    {SoundWave::kPulse50, 3322, 0, 150, 255}, {SoundWave::kPulse50, 0, 0, 50, 0},
    {SoundWave::kPulse50, 2794, 2093, 200, 255}};
// sound.success: only once the action succeeded — a rise of a fourth (G7, C8).
inline constexpr SoundNote kSoundSuccess[] = {{SoundWave::kPulse25, 3136, 0, 80, 255}, {SoundWave::kPulse25, 4186, 0, 180, 255}};
// sound.recording-start: BYTE-90's nudge moved up — a bip and a higher bip a fifth up (A7, E8).
inline constexpr SoundNote kSoundRecordingStart[] = {
    {SoundWave::kPulse25, 3520, 0, 70, 255}, {SoundWave::kPulse25, 0, 0, 45, 0}, {SoundWave::kPulse25, 5274, 0, 120, 255}};
// sound.recording-stop: the same pair falling (E8, A7).
inline constexpr SoundNote kSoundRecordingStop[] = {
    {SoundWave::kPulse25, 5274, 0, 70, 255}, {SoundWave::kPulse25, 0, 0, 45, 0}, {SoundWave::kPulse25, 3520, 0, 120, 255}};

// --- What the tokens' arithmetic keeps true, checked as the header compiles (STICKY_DESIGN_TOKENS §6) -------------
// A value changed alone that breaks one of these fails the build, not the glass.

inline constexpr int16_t kFocusRingReach = kSpacingFocusOffset + kSpacingStrokeFocus;  // how far a focus ring stands out: 8
static_assert(kSpacingItem >= 2 * kFocusRingReach, "two neighbours' rings never touch: the item gap holds both reaches");
static_assert((kSpacingFooter - kSpacingTarget) / 2 >= kFocusRingReach, "the footer holds its buttons' rings");
static_assert(kSpacingListRowHeight >= kSpacingTarget, "a list row is at least one target tall");
// A flick's last movement is part of its distance, so the movement threshold is never past it.
static_assert(kTouchFlickMovement <= kTouchFlickDistance, "a flick's last movement is part of its distance");

namespace detail {
// Every primitive above, kept by hand: a new one joins this list, or the grid check below does not see it.
inline constexpr TypeRole kScale[] = {
    kTypographyScale20Semibold,  kTypographyScale20Bold,     kTypographyScale30Regular, kTypographyScale30Semibold,
    kTypographyScale30Bold,      kTypographyScale40Semibold, kTypographyScale40Bold,    kTypographyScale50Semibold,
    kTypographyScale50Bold,      kTypographyScale60Semibold, kTypographyScale60Bold,    kTypographyScale70Semibold,
    kTypographyScale70Bold,      kTypographyScale80Semibold, kTypographyScale80Bold,    kTypographyScale100Semibold,
    kTypographyScale100Bold,     kTypographyScale120Semibold, kTypographyScale120Bold,  kTypographyScale160Semibold,
    kTypographyScale160Bold,
};
constexpr bool ScaleOnGrid()
{
    for (const TypeRole &r : kScale) {
        // Lines sit on the 4 px grid up to 80 px; figures (100 px and up) sit on their digits' height instead.
        if (r.size_px % 10 != 0 || (r.size_px <= 80 && r.line_px % 4 != 0)) {
            return false;
        }
    }
    return true;
}
template <size_t N> constexpr bool InBand(const SoundNote (&notes)[N], uint32_t max_ms)
{
    uint32_t ms = 0;
    for (const SoundNote &n : notes) {
        const bool rest = n.hz == 0 || n.volume == 0;  // silent, as the player takes it (sound_rules.h)
        const bool in_band = n.hz >= kSoundBandLowHz && n.hz <= kSoundBandHighHz &&
                             (n.hz_end == 0 || (n.hz_end >= kSoundBandLowHz && n.hz_end <= kSoundBandHighHz));
        if (!rest && !in_band) {
            return false;
        }
        ms += n.ms;
    }
    return ms <= max_ms;
}
}  // namespace detail

static_assert(detail::ScaleOnGrid(), "every type size is a multiple of 10 px and every line to 80 px a multiple of 4");
static_assert(detail::InBand(kSoundTick, kSoundTickMaxMs), "the tick is in the buzzer's band and at most tick-max");
static_assert(detail::InBand(kSoundAtEnd, kSoundMaxMs) && detail::InBand(kSoundError, kSoundMaxMs) &&
                  detail::InBand(kSoundSuccess, kSoundMaxMs) && detail::InBand(kSoundRecordingStart, kSoundMaxMs) &&
                  detail::InBand(kSoundRecordingStop, kSoundMaxMs),
              "every sound is in the buzzer's band and at most sound.max long");

}  // namespace design_tokens
