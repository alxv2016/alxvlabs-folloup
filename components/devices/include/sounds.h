// FolloUp's sound words — tick, at end, error, success, recording start and recording stop — each one of DESIGN.md's
// sound tokens (design_tokens.h `kSound*`), chosen by the person by ear on the board from the candidates in the agent
// deck's 8-bit style. devices::Play(Word) plays one; the hardware test's audition and its `sound <name>` list them
// (DESIGN → Motion → Sound; STICKY_DESIGN_TOKENS §5).

#pragma once

#include <cstddef>
#include <cstdint>

#include "design_tokens.h"
#include "sound_rules.h"

namespace devices::sound {

enum class Word : uint8_t { kTick, kAtEnd, kError, kSuccess, kRecordingStart, kRecordingStop };

struct NamedSound {
    Word word;
    const char *id;    // the console's name, `sound <id>`, and the token's: sound.<id>
    const char *name;  // the word, as the audition row says it
    const char *what;  // the chosen sound, as the row's value says it
    Sound sound;
};

// In Word's order, so kSounds[Word] is that word's.
inline constexpr NamedSound kSounds[] = {
    {Word::kTick, "tick", "Tick", "Click", Of(design_tokens::kSoundTick)},
    {Word::kAtEnd, "at-end", "At end", "Double tick", Of(design_tokens::kSoundAtEnd)},
    {Word::kError, "error", "Error", "Womp womp", Of(design_tokens::kSoundError)},
    {Word::kSuccess, "success", "Success", "Rise", Of(design_tokens::kSoundSuccess)},
    {Word::kRecordingStart, "recording-start", "Recording start", "Nudge up", Of(design_tokens::kSoundRecordingStart)},
    {Word::kRecordingStop, "recording-stop", "Recording stop", "Nudge down", Of(design_tokens::kSoundRecordingStop)},
};
inline constexpr size_t kSoundCount = sizeof(kSounds) / sizeof(kSounds[0]);

constexpr const Sound &SoundOf(Word word)
{
    return kSounds[static_cast<size_t>(word)].sound;
}

}  // namespace devices::sound
