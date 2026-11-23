// The buttons' rules that need no hardware: when a held UP or DOWN steps again, and when a held OK goes back instead of
// pressing. Pure and constexpr, as glass_rules.h is, so tests/host checks them on the computer with no board (DESIGN → Focus
// and input; DESIGN → Motion → Press; STICKY_DESIGN_TOKENS §5). input.cpp's keypad read is their one caller.

#pragma once

#include <cstdint>

#include "design_tokens.h"

namespace lvgl_display {

// The three buttons, as the rules name them.
enum class Key : uint8_t { kUp, kDown, kOk };

// Whether a held key steps again: UP and DOWN do, OK never — a held OK is the escape (OkHold).
constexpr bool KeyRepeats(Key key)
{
    return key != Key::kOk;
}

// A held UP or DOWN steps again once held this long, then this often (`motion.key-repeat-delay`, `motion.key-repeat-period`).
inline constexpr int64_t kKeyRepeatDelayUs = int64_t{design_tokens::kMotionKeyRepeatDelayMs} * 1000;
inline constexpr int64_t kKeyRepeatPeriodUs = int64_t{design_tokens::kMotionKeyRepeatPeriodMs} * 1000;
// Holding OK this long goes back instead of pressing (DESIGN → Focus and input).
inline constexpr int64_t kBackHoldUs = 1000 * 1000;

// A held UP or DOWN, as the keypad read replays it. `Press` when the press reaches LVGL — `since_us` when the watcher saw the
// key go down, `at_us` when LVGL acted on it — and `Release` when its release does: a release stops the stepping at once,
// and nothing is queued, so a step is never sent after the key came up. OK is never given to it: OK never repeats.
struct KeyRepeat {
    bool held = false;
    int64_t since_us = 0;      // when the key went down: the hold's length is measured from here
    int64_t last_step_us = 0;  // when LVGL acted on the last step — the press, then each repeat

    constexpr void Press(int64_t since, int64_t at)
    {
        held = true;
        since_us = since;
        last_step_us = at;
    }
    constexpr void Release() { held = false; }

    // Whether the next step is due at `now_us`, and if it is, takes it (the step is now the last). Due once the key has been
    // held kKeyRepeatDelayUs, kKeyRepeatPeriodUs after the last step, with the glass not still driving a frame
    // (`refresh_busy`: the SSD1677's scans), and the last step's frame on the glass — a refresh ended after it
    // (`last_frame_end_us`) — or, for a step that drew nothing, once kKeyRepeatDelayUs has passed since it. So the walk never
    // runs ahead of the glass.
    constexpr bool Step(int64_t now_us, int64_t last_frame_end_us, bool refresh_busy)
    {
        if (!held || refresh_busy) {
            return false;
        }
        if (now_us - since_us < kKeyRepeatDelayUs || now_us - last_step_us < kKeyRepeatPeriodUs) {
            return false;
        }
        const bool landed = last_frame_end_us > last_step_us;
        if (!landed && now_us - last_step_us < kKeyRepeatDelayUs) {
            return false;
        }
        last_step_us = now_us;
        return true;
    }
};

// A held OK: it goes back once, when the hold has lasted kBackHoldUs, whether LVGL saw it begin or only its release.
// A press is named by when it began (`since_us`), so one hold is used once however many reads see it.
struct OkHold {
    int64_t used_since_us = -1;  // the press a hold has already used to go back

    constexpr bool Used(int64_t since_us) const { return since_us == used_since_us; }

    // While OK is still down: true once, the read the hold reaches kBackHoldUs.
    constexpr bool WhileDown(bool held, int64_t since_us, int64_t now_us)
    {
        if (!held || Used(since_us) || now_us - since_us < kBackHoldUs) {
            return false;
        }
        used_since_us = since_us;
        return true;
    }

    // At OK's release (`at_us`): true when the press lasted kBackHoldUs and has not gone back already — a hold that ended
    // before LVGL could see it. False is a press, which acts.
    constexpr bool AtRelease(int64_t since_us, int64_t at_us)
    {
        if (Used(since_us) || at_us - since_us < kBackHoldUs) {
            return false;
        }
        used_since_us = since_us;
        return true;
    }
};

}  // namespace lvgl_display
