// The status line's rules that need no LVGL: which news is wanted, which is drawn, and what the band must do next. Pure, as
// key_rules.h and sound_rules.h are, so tests/host checks them on the computer with no board (DESIGN → Components → The kit;
// APP_ARCHITECTURE §7). status.cpp is the one caller: it draws what a step says into the screen's band.
//
// TWO LINES, WANTED AND DRAWN. The caller's latest news is the wanted line; the band's text is the drawn one. They differ only
// while an overlay is open — the band holds still under a dialog, a sheet or the keyboard, and what changed is drawn, and
// sounded, once the last one closes. So a held line that is retracted before then is never drawn and never heard.
//
// THE SOUND FOLLOWS THE SAYING, THE DRAWING FOLLOWS THE WORDS. Every Say of success or error asks its sound — a press always
// hears its outcome — even when the same words and kind are already up, which draw nothing and keep their id. Said under an
// overlay it is owed: it sounds once, as the overlay closes, however many times it was said. Info is silent.

#pragma once

#include <cstddef>
#include <cstdint>

namespace router {

// What the news is: it chooses the sound — success and error sound, info is silent — and nothing else. The words say it too,
// and the three are drawn alike (DESIGN → Components → The kit).
enum class StatusKind : uint8_t { kInfo, kSuccess, kError };

// Names one line, so a caller retracts only its own. Never 0; 0 is "no line" — what Say returns when nothing was said.
using StatusId = uint32_t;

// The copy cap: 60 characters at most, so two lines of `caption-strong` at 36 characters a line never fill edge to edge. Past
// it the line is still drawn — LVGL ends the second line in dots — and the log says so for the developer.
inline constexpr size_t kStatusMaxChars = 60;
// The bytes kept of a line's words, a terminator included. 60 characters of ASCII and of the font's two-byte marks (· ° × — U+00B7,
// U+00B0, U+00D7) fit; its three-byte marks (– — ’ • …) fit only 42, and words beyond the bytes are cut at a character's
// edge, which the log says, as it does for the character cap.
inline constexpr size_t kStatusBytes = 128;

// Bytes in `text`, without the terminator.
constexpr size_t StatusLength(const char *text)
{
    size_t n = 0;
    while (text != nullptr && text[n] != '\0') {
        ++n;
    }
    return n;
}

// Characters in UTF-8 `text`: every byte that is not a continuation.
constexpr size_t StatusChars(const char *text)
{
    size_t chars = 0;
    for (; text != nullptr && *text != '\0'; ++text) {
        chars += (static_cast<uint8_t>(*text) & 0xC0) != 0x80 ? 1 : 0;
    }
    return chars;
}

// What a line asks of the buzzer.
enum class StatusSound : uint8_t { kNone, kSuccess, kError };
constexpr StatusSound SoundOf(StatusKind kind)
{
    return kind == StatusKind::kSuccess ? StatusSound::kSuccess
           : kind == StatusKind::kError ? StatusSound::kError
                                        : StatusSound::kNone;
}

struct StatusLine {
    StatusId id = 0;  // 0: no line
    StatusKind kind = StatusKind::kInfo;
    char words[kStatusBytes] = {};
};

// What the caller must do to the band after a step.
enum class StatusDraw : uint8_t {
    kNothing,  // the band stays as it is
    kShow,     // set the band's words to `line` and show its rule
    kBlank,    // empty the band's words and hide its rule
};

struct StatusStep {
    StatusId id = 0;                      // Say's answer: the line's id; 0 when nothing was said
    StatusDraw draw = StatusDraw::kNothing;
    StatusSound sound = StatusSound::kNone;  // to play, before the draw: for each Say of success or error, never for a Clear
    bool over_cap = false;                // the words had more than kStatusMaxChars characters: log it
    bool cut = false;                     // the words had more bytes than kStatusBytes keeps and were cut: log it
    size_t chars = 0;                     // the words' characters, for the log
};

class StatusState {
public:
    // The test seam for the ids: they continue from `last_id`, so a test can start a step from the wrap at UINT32_MAX.
    constexpr explicit StatusState(StatusId last_id = 0) : last_id_(last_id) {}

    // The screen on top has changed, or been entered the first time: the news goes with the old one, held news included
    // — nothing of it is drawn or heard — and the band is the new screen's, if it has one (`has_band`). Ids stay unique
    // across it, so the old screen's caller who retracts later retracts nothing.
    constexpr void ScreenChanged(bool has_band)
    {
        band_ = has_band;
        wanted_ = StatusLine{};
        drawn_ = StatusLine{};
        owed_ = false;
    }

    // News: replaces the one showing. With no band on the screen, or empty words, nothing is said (id 0). The same words and
    // kind again draw nothing and give the same id back, but still ask their sound. With an overlay open the band holds still
    // and the news, and its sound, wait for OverlaysClosed.
    constexpr StatusStep Say(StatusKind kind, const char *words, bool overlay_open)
    {
        StatusStep step;
        if (!band_ || words == nullptr || *words == '\0') {
            return step;
        }
        step.chars = StatusChars(words);
        step.over_cap = step.chars > kStatusMaxChars;
        step.cut = StatusLength(words) >= kStatusBytes;
        if (wanted_.id != 0 && wanted_.kind == kind && SameWords(wanted_.words, words)) {
            step.id = wanted_.id;
            if (overlay_open) {
                owed_ = true;
            } else {
                step.sound = SoundOf(kind);
            }
            return step;
        }
        wanted_.id = NextId();
        wanted_.kind = kind;
        Copy(wanted_.words, words);
        step.id = wanted_.id;
        if (overlay_open) {
            owed_ = true;
        } else {
            Draw(&step, true);
        }
        return step;
    }

    // The caller retracts its line: only if it is still the line wanted — a stale id, a replaced line, or one from another
    // screen does nothing. With an overlay open the band is emptied when it closes.
    constexpr StatusStep Clear(StatusId id, bool overlay_open)
    {
        StatusStep step;
        if (id == 0 || wanted_.id != id) {
            return step;
        }
        wanted_ = StatusLine{};
        owed_ = false;
        if (!overlay_open) {
            Draw(&step, false);
        }
        return step;
    }

    // The last overlay has closed: draw what changed while it stood, and sound what was said, once.
    constexpr StatusStep OverlaysClosed()
    {
        StatusStep step;
        Draw(&step, owed_);
        owed_ = false;
        return step;
    }

    constexpr bool HasBand() const { return band_; }
    constexpr const StatusLine &Drawn() const { return drawn_; }
    constexpr const StatusLine &Wanted() const { return wanted_; }

private:
    // Brings the drawn line to the wanted one, saying what that takes, and asks the wanted line's sound when `sound_due`. The
    // same news under another id is only renamed: the band already shows it, so it is not drawn again.
    constexpr void Draw(StatusStep *step, bool sound_due)
    {
        if (wanted_.id != 0 && sound_due) {
            step->sound = SoundOf(wanted_.kind);
        }
        if (wanted_.id == drawn_.id) {
            return;
        }
        if (wanted_.id == 0) {
            drawn_ = StatusLine{};
            step->draw = StatusDraw::kBlank;
            return;
        }
        if (drawn_.id != 0 && drawn_.kind == wanted_.kind && SameWords(drawn_.words, wanted_.words)) {
            drawn_.id = wanted_.id;
            return;
        }
        drawn_ = wanted_;
        step->draw = StatusDraw::kShow;
    }

    constexpr StatusId NextId()
    {
        last_id_ = last_id_ == UINT32_MAX ? 1 : last_id_ + 1;
        return last_id_;
    }

    static constexpr bool SameWords(const char *a, const char *b)
    {
        // `b` may be longer than what was kept of it: compare as kept, so the same long words again are still the same.
        StatusLine kept;
        Copy(kept.words, b);
        size_t i = 0;
        for (; a[i] != '\0' && a[i] == kept.words[i]; ++i) {
        }
        return a[i] == kept.words[i];
    }

    // Copies up to kStatusBytes - 1 bytes, stopping at a character's edge, and terminates.
    static constexpr void Copy(char *to, const char *from)
    {
        size_t n = 0;
        while (from[n] != '\0' && n + 1 < kStatusBytes) {
            ++n;
        }
        // Stopped inside a character: back to its first byte.
        if (from[n] != '\0') {
            while (n > 0 && (static_cast<uint8_t>(from[n]) & 0xC0) == 0x80) {
                --n;
            }
        }
        for (size_t i = 0; i < n; ++i) {
            to[i] = from[i];
        }
        to[n] = '\0';
    }

    bool band_ = false;
    StatusId last_id_;
    StatusLine wanted_;
    StatusLine drawn_;
    bool owed_ = false;  // the wanted line was said under an overlay: its sound is due as the overlay closes
};

}  // namespace router
