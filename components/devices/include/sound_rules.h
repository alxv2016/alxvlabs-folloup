// The buzzer's sounds as data, and the arithmetic that plays them: the agent deck's chiptune note format (its Wave, Note
// and Sound) adapted to a magnetic buzzer that LEDC switches on and off — a wave is a duty, a pitch is the LEDC timer's
// frequency, a slide is that frequency stepped, noise is the pitch hopping at random. Pure and constexpr, so tests/host
// checks it on the computer with no board; devices.cpp's player task applies each Frame to the LEDC channel
// (STICKY_HARDWARE_SPECS §4.2).

#pragma once

#include <cstddef>
#include <cstdint>

#include "design_tokens.h"

namespace devices::sound {

// The deck's five voices and its note, as DESIGN.md's sound tokens write them (design_tokens.h). A pulse voice is its duty,
// so the buzzer plays it as the deck's synth does; the triangle has no shape a switched coil can make and plays as the
// full square, its nearest; noise plays as random pitch hops. A note's volume is kept so the deck's tables port as they
// are, though the buzzer has no level but its duty, which is the wave already: only 0 means silence.
using Wave = design_tokens::SoundWave;
using Note = design_tokens::SoundNote;

// Notes played in order, once. The deck's `loops` is left out: a buzzer that repeats while something waits is the
// feedback reference's "repeating while waiting", which FolloUp does not do.
struct Sound {
    const Note *notes;
    size_t count;
};

template <size_t N>
constexpr Sound Of(const Note (&notes)[N])
{
    return {notes, N};
}

// --- The player's steps ------------------------------------------------------------------------------------------

// How often the player task wakes while a sound plays, and the finest time a note's edge can land on: FreeRTOS's 1 ms
// tick (sdkconfig.defaults).
inline constexpr uint32_t kStepMs = 1;
// A slide is held for this long at each pitch, then steps: fine enough that a 60 ms slide is fifteen steps and heard as a
// glide, coarse enough that LEDC's timer is not reprogrammed every millisecond for it.
inline constexpr uint32_t kSlideStepMs = 4;
// A noise note's pitch is chosen afresh this often — at 2 to 6 kHz, two to six cycles each, which the ear hears as a rasp
// rather than as notes.
inline constexpr uint32_t kNoiseHopMs = 1;
// And within this share of its centre, either way.
inline constexpr uint32_t kNoiseSpreadPct = 40;

// The pitches the player sets: LEDC's 10-bit timer on the 80 MHz APB clock divides by 1 to 1023 (80 MHz / 1024 / divider),
// so 77 Hz to 78 kHz are reachable; the clamp keeps every pitch well inside, and a note asking for more or less plays at
// the bound.
inline constexpr uint32_t kLowestHz = 100;
inline constexpr uint32_t kHighestHz = 10000;

// The FUET-5018's resonance, 4 kHz (LCSC's listing for the part), where Seeed drives it and where a V1.0 measured it
// loudest; the band a sound is written in, `sound.band-low` to `sound.band-high`, within which the buzzer measured 11 to
// 22 dB down from the peak between 2 and 3.5 kHz and 4 to 17 dB down between 4.5 and 6 kHz (STICKY_HARDWARE_SPECS §4.2).
// Below 2 kHz a note is heard mostly by its harmonics near 4 kHz, quieter and buzzier.
inline constexpr uint32_t kResonanceHz = 4000;
inline constexpr uint32_t kBandLowHz = design_tokens::kSoundBandLowHz;
inline constexpr uint32_t kBandHighHz = design_tokens::kSoundBandHighHz;

// The lengths a sound may take: a tick, which sounds on every press and every repeated step of a held key, at most
// `sound.tick-max`, 40 ms, so a 500 ms repeat never runs ticks together; any other sound at most `sound.max`, 600 ms, so
// an outcome is told and gone before the person's next press is likely.
inline constexpr uint32_t kTickMaxMs = design_tokens::kSoundTickMaxMs;
inline constexpr uint32_t kSoundMaxMs = design_tokens::kSoundMaxMs;

// --- The arithmetic ----------------------------------------------------------------------------------------------

// What the LEDC channel is set to at a moment: the timer's frequency and the channel's duty. A duty of 0 is silence, and
// `hz` is 0 there, meaning "leave the timer as it is".
struct Frame {
    uint32_t hz;
    uint32_t duty;
    bool done;  // past the sound's last note: silent, and the player may sleep
};

constexpr uint32_t LengthMs(const Sound &sound)
{
    uint32_t ms = 0;
    for (size_t i = 0; i < sound.count; ++i) {
        ms += sound.notes[i].ms;
    }
    return ms;
}

// The duty a voice plays at, from `half`, the channel's half duty (board_config's kBuzzerDuty, 512 of 10-bit): 50, 25
// and 12.5 % for the pulses, 50 % for the triangle and for noise, which is loudest so.
constexpr uint32_t DutyFor(Wave wave, uint32_t half)
{
    switch (wave) {
    case Wave::kPulse25: return half / 2;
    case Wave::kPulse12: return half / 4;
    case Wave::kPulse50:
    case Wave::kTriangle:
    case Wave::kNoise: return half;
    }
    return half;
}

constexpr uint32_t ClampHz(uint32_t hz)
{
    return hz < kLowestHz ? kLowestHz : hz > kHighestHz ? kHighestHz : hz;
}

// A note's pitch `at_ms` into it: its start, or, with a slide, a straight line from `hz` to `hz_end` held kSlideStepMs at
// each step, the first step at `hz` and the last at `hz_end`.
constexpr uint32_t SlideHz(const Note &note, uint32_t at_ms)
{
    if (note.hz_end == 0 || note.hz_end == note.hz) {
        return note.hz;
    }
    const uint32_t steps = note.ms / kSlideStepMs;
    if (steps < 2) {
        return note.hz;
    }
    uint32_t step = at_ms / kSlideStepMs;
    if (step > steps - 1) {
        step = steps - 1;
    }
    const int32_t from = note.hz;
    const int32_t to = note.hz_end;
    return static_cast<uint32_t>(from + (to - from) * static_cast<int32_t>(step) / static_cast<int32_t>(steps - 1));
}

// A hop's draw, 0 to 65535: a 32-bit integer hash (Chris Wellons' "lowbias32") of the note's place and the hop's number, so
// the same sound hops the same way every time and a host test can hear it.
constexpr uint32_t HopDraw(uint32_t note_index, uint32_t hop)
{
    uint32_t x = note_index * 0x9E3779B9u + hop + 1u;
    x ^= x >> 16;
    x *= 0x7FEB352Du;
    x ^= x >> 15;
    x *= 0x846CA68Bu;
    x ^= x >> 16;
    return x & 0xFFFFu;
}

// A noise note's pitch `at_ms` into it: its centre (sliding, as any note's) moved by up to kNoiseSpreadPct either way,
// a fresh draw every kNoiseHopMs.
constexpr uint32_t NoiseHz(const Note &note, size_t note_index, uint32_t at_ms)
{
    const int32_t centre = static_cast<int32_t>(SlideHz(note, at_ms));
    const int32_t spread = centre * static_cast<int32_t>(kNoiseSpreadPct) / 100;
    const int32_t draw = static_cast<int32_t>(HopDraw(static_cast<uint32_t>(note_index), at_ms / kNoiseHopMs));
    return static_cast<uint32_t>(centre - spread + 2 * spread * draw / 65535);
}

// The LEDC setting `elapsed_ms` into a sound, `half` the channel's half duty.
constexpr Frame FrameAt(const Sound &sound, uint32_t elapsed_ms, uint32_t half)
{
    uint32_t start = 0;
    for (size_t i = 0; i < sound.count; ++i) {
        const Note &note = sound.notes[i];
        if (elapsed_ms < start + note.ms) {
            if (note.hz == 0 || note.volume == 0) {
                return {0, 0, false};  // a rest
            }
            const uint32_t at = elapsed_ms - start;
            const uint32_t hz = note.wave == Wave::kNoise ? NoiseHz(note, i, at) : SlideHz(note, at);
            return {ClampHz(hz), DutyFor(note.wave, half), false};
        }
        start += note.ms;
    }
    return {0, 0, true};
}

}  // namespace devices::sound
