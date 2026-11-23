// Host tests of the buzzer's note player (components/devices/include/sound_rules.h) and FolloUp's sound words
// (sounds.h, DESIGN.md's sound tokens in design_tokens.h), on the computer with no board: a pulse voice is its duty, 50, 25 and 12.5 % of the 10-bit channel,
// the triangle and noise the full square; a note's pitch is the LEDC timer's frequency, clamped to what the timer can
// divide to; a slide steps every kSlideStepMs from its start to its end; noise hops at random within 40 % of its centre,
// the same way every time; a rest and the end are silent; the tick is the 30 ms at 4 kHz it has been since bring-up; and
// each of the six words has its token, in the buzzer's band, a tick at most 40 ms and any other sound at most 600
// (STICKY_HARDWARE_SPECS §4.2; STICKY_DESIGN_TOKENS §5).
// Build and run: bash tests/host/run.sh

#include <cstdio>
#include <cstring>

#include "design_tokens.h"
#include "sounds.h"
#include "sound_rules.h"

using namespace devices::sound;

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

constexpr uint32_t kHalf = 512;  // board_config's kBuzzerDuty: half of 10 bits

void TestDuty()
{
    Check(DutyFor(Wave::kPulse50, kHalf) == 512, "the full square is half the 10-bit duty");
    Check(DutyFor(Wave::kPulse25, kHalf) == 256, "the 25 % pulse is a quarter");
    Check(DutyFor(Wave::kPulse12, kHalf) == 128, "the 12.5 % pulse is an eighth");
    Check(DutyFor(Wave::kTriangle, kHalf) == 512, "the triangle plays as the full square");
    Check(DutyFor(Wave::kNoise, kHalf) == 512, "noise plays at the full square's duty");
}

void TestNotes()
{
    constexpr Note notes[] = {{Wave::kPulse25, 3000, 0, 20, 200}, {Wave::kPulse25, 0, 0, 10, 0}, {Wave::kPulse12, 4000, 0, 5, 9}};
    const Sound s = Of(notes);
    Check(LengthMs(s) == 35, "a sound's length is its notes'");
    Check(FrameAt(s, 0, kHalf).hz == 3000 && FrameAt(s, 0, kHalf).duty == 256, "the first note sounds at once");
    Check(FrameAt(s, 19, kHalf).hz == 3000, "a note holds to its last millisecond");
    Check(FrameAt(s, 20, kHalf).duty == 0 && FrameAt(s, 20, kHalf).hz == 0 && !FrameAt(s, 20, kHalf).done,
          "a rest is silent and leaves the timer, and the sound is not done");
    Check(FrameAt(s, 30, kHalf).hz == 4000 && FrameAt(s, 30, kHalf).duty == 128, "the next note after the rest");
    Check(FrameAt(s, 34, kHalf).duty == 128, "a volume above 0 sounds: the buzzer has no level but the duty");
    Check(FrameAt(s, 35, kHalf).done && FrameAt(s, 35, kHalf).duty == 0, "past the last note, silent and done");
    Check(FrameAt(Sound{}, 0, kHalf).done && FrameAt(Sound{}, 0, kHalf).duty == 0, "no sound is silence");

    constexpr Note quiet[] = {{Wave::kPulse50, 3000, 0, 10, 0}};
    Check(FrameAt(Of(quiet), 0, kHalf).duty == 0, "a volume of 0 is a rest");

    constexpr Note edges[] = {{Wave::kPulse50, 40, 0, 10, 255}, {Wave::kPulse50, 30000, 0, 10, 255}};
    Check(FrameAt(Of(edges), 0, kHalf).hz == kLowestHz, "a pitch below the timer's reach plays at the lowest");
    Check(FrameAt(Of(edges), 10, kHalf).hz == kHighestHz, "and one above at the highest");
}

void TestSlide()
{
    constexpr Note up = {Wave::kPulse25, 2000, 4000, 80, 255};  // 20 steps of 4 ms
    Check(SlideHz(up, 0) == 2000, "a slide starts at its start");
    Check(SlideHz(up, kSlideStepMs - 1) == 2000, "and holds there for a step");
    Check(SlideHz(up, kSlideStepMs) > 2000, "then steps");
    Check(SlideHz(up, 79) == 4000, "its last step is its end");
    bool rising = true;
    for (uint32_t t = 1; t < up.ms; ++t) {
        rising = rising && SlideHz(up, t) >= SlideHz(up, t - 1);
    }
    Check(rising, "a slide up never falls");
    int changes = 0;
    for (uint32_t t = 1; t < up.ms; ++t) {
        changes += SlideHz(up, t) != SlideHz(up, t - 1);
    }
    Check(changes == static_cast<int>(up.ms / kSlideStepMs) - 1, "the pitch changes once a step, never each millisecond");

    constexpr Note down = {Wave::kPulse25, 4186, 2093, 120, 255};
    Check(SlideHz(down, 0) == 4186 && SlideHz(down, 119) == 2093, "a slide down reaches its end");
    constexpr Note flat = {Wave::kPulse50, 3000, 0, 50, 255};
    Check(SlideHz(flat, 25) == 3000, "no end, no slide");
    constexpr Note short_slide = {Wave::kPulse50, 3000, 4000, 5, 255};
    Check(SlideHz(short_slide, 4) == 3000, "a note too short for two steps holds its start");
}

void TestNoise()
{
    constexpr Note noise = {Wave::kNoise, 3500, 0, 40, 255};
    bool within = true;
    int changes = 0;
    for (uint32_t t = 0; t < noise.ms; ++t) {
        const uint32_t hz = NoiseHz(noise, 0, t);
        within = within && hz >= 3500 * (100 - kNoiseSpreadPct) / 100 && hz <= 3500 * (100 + kNoiseSpreadPct) / 100;
        if (t > 0) {
            changes += hz != NoiseHz(noise, 0, t - 1);
        }
    }
    Check(within, "noise hops within 40 % of its centre");
    Check(changes >= 30, "noise takes a new pitch nearly every hop");
    Check(NoiseHz(noise, 0, 7) == NoiseHz(noise, 0, 7), "the same hop draws the same pitch, every time");
    Check(NoiseHz(noise, 0, 7) != NoiseHz(noise, 1, 7), "another note hops another way");
    constexpr Note notes[] = {noise};
    Check(FrameAt(Of(notes), 3, kHalf).hz == NoiseHz(noise, 0, 3), "a noise note plays its hops");
}

void TestTick()
{
    const Sound &tick = SoundOf(Word::kTick);
    Check(tick.notes == design_tokens::kSoundTick, "the tick is sound.tick");
    Check(LengthMs(tick) == 30, "the tick is 30 ms, as it has been since bring-up");
    Check(FrameAt(tick, 0, kHalf).hz == kResonanceHz && FrameAt(tick, 0, kHalf).duty == 512, "at 4 kHz on the full square");
    Check(FrameAt(tick, 30, kHalf).done, "and then silent");
}

bool InBand(uint16_t hz)
{
    return hz == 0 || (hz >= kBandLowHz && hz <= kBandHighHz);
}

void TestWords()
{
    Check(kBandLowHz == 2000 && kBandHighHz == 6000, "the band is sound.band-low to sound.band-high, 2 to 6 kHz");
    Check(kTickMaxMs == 40 && kSoundMaxMs == 600, "sound.tick-max is 40 ms and sound.max 600 ms");
    Check(kSoundCount == 6, "six words: tick, at end, error, success, recording start and stop");
    const design_tokens::SoundNote *tokens[] = {design_tokens::kSoundTick,    design_tokens::kSoundAtEnd,
                                                design_tokens::kSoundError,   design_tokens::kSoundSuccess,
                                                design_tokens::kSoundRecordingStart, design_tokens::kSoundRecordingStop};
    char what[96];
    for (size_t i = 0; i < kSoundCount; ++i) {
        const NamedSound &n = kSounds[i];
        std::snprintf(what, sizeof what, "%s is in its word's place and plays its token", n.id);
        Check(static_cast<size_t>(n.word) == i && SoundOf(n.word).notes == tokens[i] && n.sound.notes == tokens[i], what);
        const uint32_t limit = n.word == Word::kTick ? kTickMaxMs : kSoundMaxMs;
        std::snprintf(what, sizeof what, "%s is %u ms, within %u", n.id, LengthMs(n.sound), limit);
        Check(n.sound.count > 0 && LengthMs(n.sound) <= limit, what);
        bool band = true;
        bool steps = true;
        for (size_t k = 0; k < n.sound.count; ++k) {
            const Note &note = n.sound.notes[k];
            band = band && InBand(note.hz) && InBand(note.hz_end);
            steps = steps && note.ms >= kStepMs && (note.hz_end == 0 || note.ms / kSlideStepMs >= 2);
        }
        std::snprintf(what, sizeof what, "%s is written in the buzzer's band", n.id);
        Check(band, what);
        std::snprintf(what, sizeof what, "%s: every note at least a step, every slide two", n.id);
        Check(steps, what);
        std::snprintf(what, sizeof what, "%s ends on a note, not a rest", n.id);
        Check(n.sound.notes[n.sound.count - 1].hz != 0, what);
        for (size_t j = 0; j < i; ++j) {
            std::snprintf(what, sizeof what, "%s is named once", n.id);
            Check(std::strcmp(kSounds[j].id, n.id) != 0, what);
        }
        std::snprintf(what, sizeof what, "%s fits the console's 32-character line", n.id);
        Check(std::strlen("sound ") + std::strlen(n.id) < 32, what);
    }
    // The person's choices, by ear on the board: the double tick, the womp, the rise, the nudge up and down.
    Check(LengthMs(SoundOf(Word::kAtEnd)) == 95 && SoundOf(Word::kAtEnd).count == 3, "at end is the double tick");
    Check(LengthMs(SoundOf(Word::kError)) == 600 && SoundOf(Word::kError).notes[4].hz_end == 2093, "error is the womp, sliding down");
    Check(SoundOf(Word::kSuccess).notes[1].hz > SoundOf(Word::kSuccess).notes[0].hz, "success rises");
    Check(SoundOf(Word::kRecordingStart).notes[2].hz > SoundOf(Word::kRecordingStart).notes[0].hz &&
              SoundOf(Word::kRecordingStop).notes[2].hz < SoundOf(Word::kRecordingStop).notes[0].hz,
          "recording start rises and stop falls");
    Check(SoundOf(Word::kRecordingStart).notes[0].hz == SoundOf(Word::kRecordingStop).notes[2].hz &&
              SoundOf(Word::kRecordingStart).notes[2].hz == SoundOf(Word::kRecordingStop).notes[0].hz,
          "and the stop is the start's pair turned round");
}

}  // namespace

int main()
{
    TestDuty();
    TestNotes();
    TestSlide();
    TestNoise();
    TestTick();
    TestWords();
    std::printf("sound rules: %d checks, %d failed\n", s_checks, s_failures);
    return s_failures == 0 ? 0 : 1;
}
