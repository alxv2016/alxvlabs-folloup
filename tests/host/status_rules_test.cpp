// Host tests of the status line's rules (components/router/include/router/status_rules.h), on the computer with no board and no
// LVGL: the newest news replaces the one showing and the same words again draw nothing and still sound success or error; Clear
// retracts only the line it names — a stale id is a no-op; a change of screen drops the news, held news included, and ids stay
// unique across it; a screen with no band says nothing; while an overlay is open the band holds still and what changed is
// drawn, and sounded, once when it closes — and a held line retracted before then is never drawn nor heard; success and error
// sound, info is silent; the copy cap counts characters and keeps whole ones (DESIGN → Components → The kit; APP_ARCHITECTURE
// §7). Build and run: bash tests/host/run.sh

#include <cstdio>
#include <cstring>

#include "router/status_rules.h"

using namespace router;

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

bool Is(const StatusLine &line, const char *words)
{
    return std::strcmp(line.words, words) == 0;
}

StatusState WithBand()
{
    StatusState state;
    state.ScreenChanged(true);
    return state;
}

void TestSay()
{
    StatusState state = WithBand();
    const StatusStep step = state.Say(StatusKind::kSuccess, "Card read.", false);
    Check(step.id != 0, "news gets an id, never 0");
    Check(step.draw == StatusDraw::kShow && Is(state.Drawn(), "Card read."), "it is drawn at once");
    Check(step.sound == StatusSound::kSuccess, "success sounds sound.success");
    Check(state.Say(StatusKind::kError, "No card.", false).sound == StatusSound::kError, "error sounds sound.error");
    Check(state.Say(StatusKind::kInfo, "Card removed.", false).sound == StatusSound::kNone, "info is silent");
    Check(state.Say(StatusKind::kInfo, "Card removed.", false).draw == StatusDraw::kNothing, "the same info again draws nothing");
}

void TestReplace()
{
    StatusState state = WithBand();
    const StatusStep first = state.Say(StatusKind::kInfo, "Card removed.", false);
    const StatusStep second = state.Say(StatusKind::kError, "No card.", false);
    Check(second.id != first.id, "a replacement is a new line");
    Check(second.draw == StatusDraw::kShow && Is(state.Drawn(), "No card.") && Is(state.Wanted(), "No card."),
          "the newest replaces the one showing, in one step");
    Check(state.Drawn().id == second.id, "and the band names the newest");
}

void TestSameWordsAgain()
{
    StatusState state = WithBand();
    const StatusStep first = state.Say(StatusKind::kSuccess, "Card read.", false);
    const StatusStep again = state.Say(StatusKind::kSuccess, "Card read.", false);
    Check(again.draw == StatusDraw::kNothing && again.sound == StatusSound::kSuccess,
          "the same success again: no redraw, but sound.success is asked again — a press hears its outcome");
    Check(again.id == first.id, "and the same id, so the first caller's Clear still works");
    const StatusStep error = state.Say(StatusKind::kError, "No card.", false);
    const StatusStep error_again = state.Say(StatusKind::kError, "No card.", false);
    Check(error_again.draw == StatusDraw::kNothing && error_again.sound == StatusSound::kError && error_again.id == error.id,
          "the same error again: no redraw, same id, sound.error again");
    const StatusStep info = state.Say(StatusKind::kInfo, "Card removed.", false);
    Check(state.Say(StatusKind::kInfo, "Card removed.", false).sound == StatusSound::kNone && info.id != 0,
          "the same info again stays silent");
    state.Say(StatusKind::kSuccess, "Card read.", false);
    const StatusStep other_kind = state.Say(StatusKind::kError, "Card read.", false);
    Check(other_kind.draw == StatusDraw::kShow && other_kind.id != first.id, "the same words of another kind are news");
    // After a retraction the words are news again.
    state.Clear(other_kind.id, false);
    Check(state.Say(StatusKind::kError, "Card read.", false).draw == StatusDraw::kShow, "after a Clear the words are said afresh");
}

void TestNothingSaid()
{
    StatusState state;  // never given a screen
    Check(state.Say(StatusKind::kError, "No card.", false).id == 0, "no screen, no band: nothing said");
    state.ScreenChanged(false);
    const StatusStep none = state.Say(StatusKind::kError, "No card.", false);
    Check(none.id == 0 && none.draw == StatusDraw::kNothing && none.sound == StatusSound::kNone,
          "a screen with no band: no id, no draw, no sound for a line never seen");
    state.ScreenChanged(true);
    Check(state.Say(StatusKind::kInfo, "", false).id == 0 && state.Say(StatusKind::kInfo, nullptr, false).id == 0,
          "empty words are not news");
    Check(!state.Say(StatusKind::kError, "No card.", false).over_cap, "(a short line is under the cap)");
}

void TestClearById()
{
    StatusState state = WithBand();
    const StatusId first = state.Say(StatusKind::kInfo, "Card removed.", false).id;
    const StatusId second = state.Say(StatusKind::kSuccess, "Card read.", false).id;
    StatusStep step = state.Clear(first, false);
    Check(step.draw == StatusDraw::kNothing && Is(state.Drawn(), "Card read."), "a stale id is a no-op: the newer line stays");
    step = state.Clear(0, false);
    Check(step.draw == StatusDraw::kNothing && state.Drawn().id == second, "id 0 is a no-op");
    step = state.Clear(second + 100, false);
    Check(step.draw == StatusDraw::kNothing && state.Drawn().id == second, "an id never given is a no-op");
    step = state.Clear(second, false);
    Check(step.draw == StatusDraw::kBlank && step.sound == StatusSound::kNone && state.Drawn().id == 0 && Is(state.Drawn(), ""),
          "its own id retracts it: the band goes blank, silently");
    step = state.Clear(second, false);
    Check(step.draw == StatusDraw::kNothing, "a second Clear of the same id is a no-op");
}

void TestScreenChange()
{
    StatusState state = WithBand();
    const StatusId old_id = state.Say(StatusKind::kError, "No card.", false).id;
    state.ScreenChanged(true);
    Check(state.Drawn().id == 0 && state.Wanted().id == 0, "a change of screen clears the line, drawn and wanted");
    Check(state.Clear(old_id, false).draw == StatusDraw::kNothing, "the old screen's caller retracts nothing");
    const StatusId fresh = state.Say(StatusKind::kInfo, "Card removed.", false).id;
    Check(fresh != old_id, "ids stay unique across a change of screen");
    Check(state.Clear(old_id, false).draw == StatusDraw::kNothing && state.Drawn().id == fresh,
          "so the old id never retracts the new screen's line");
    state.ScreenChanged(false);
    Check(!state.HasBand() && state.Say(StatusKind::kError, "No card.", false).id == 0, "the next screen may have no band");
    // Held news goes with the screen too.
    state.ScreenChanged(true);
    state.Say(StatusKind::kError, "No card.", true);
    state.ScreenChanged(true);
    const StatusStep closed = state.OverlaysClosed();
    Check(closed.draw == StatusDraw::kNothing && closed.sound == StatusSound::kNone,
          "news held under an overlay is dropped when the screen changes: never drawn, never heard");
}

void TestHoldUnderOverlay()
{
    StatusState state = WithBand();
    const StatusStep shown = state.Say(StatusKind::kInfo, "Card removed.", false);
    const StatusStep held = state.Say(StatusKind::kError, "No card.", true);
    Check(held.id != 0 && held.id != shown.id, "news under an overlay still gets its id");
    Check(held.draw == StatusDraw::kNothing && held.sound == StatusSound::kNone, "but is not drawn and not heard yet");
    Check(Is(state.Drawn(), "Card removed.") && Is(state.Wanted(), "No card."), "the band holds still; the news is wanted");
    const StatusStep closed = state.OverlaysClosed();
    Check(closed.draw == StatusDraw::kShow && closed.sound == StatusSound::kError && Is(state.Drawn(), "No card."),
          "when the overlay closes it is drawn, and sounded, then");
    Check(state.OverlaysClosed().draw == StatusDraw::kNothing, "and only once");
}

void TestHeldRepeatSoundsOnce()
{
    StatusState state = WithBand();
    state.Say(StatusKind::kError, "No card.", true);
    const StatusStep repeat = state.Say(StatusKind::kError, "No card.", true);
    Check(repeat.draw == StatusDraw::kNothing && repeat.sound == StatusSound::kNone, "a repeat under an overlay sounds nothing yet");
    const StatusStep closed = state.OverlaysClosed();
    Check(closed.draw == StatusDraw::kShow && closed.sound == StatusSound::kError, "it is drawn and sounded as the overlay closes");
    Check(state.OverlaysClosed().sound == StatusSound::kNone, "once, however many times it was said");
    // A repeat of a line already showing, said under an overlay, sounds once as it closes.
    state.Say(StatusKind::kError, "No card.", true);
    Check(state.OverlaysClosed().sound == StatusSound::kError, "a repeat of what shows, said under an overlay, sounds as it closes");
}

void TestHeldNewestWins()
{
    StatusState state = WithBand();
    state.Say(StatusKind::kError, "No card.", true);
    state.Say(StatusKind::kSuccess, "Card read.", true);
    const StatusStep closed = state.OverlaysClosed();
    Check(closed.draw == StatusDraw::kShow && Is(state.Drawn(), "Card read.") && closed.sound == StatusSound::kSuccess,
          "of several held, the newest is drawn and heard, the others never");
}

void TestHeldThenRetracted()
{
    StatusState state = WithBand();
    const StatusId held = state.Say(StatusKind::kError, "No card.", true).id;
    Check(state.Clear(held, true).draw == StatusDraw::kNothing, "a held line retracted under the overlay draws nothing");
    const StatusStep closed = state.OverlaysClosed();
    Check(closed.draw == StatusDraw::kNothing && closed.sound == StatusSound::kNone && state.Drawn().id == 0,
          "and when the overlay closes the band stays white and silent");
}

void TestClearShownUnderOverlay()
{
    StatusState state = WithBand();
    const StatusId shown = state.Say(StatusKind::kInfo, "Card removed.", false).id;
    const StatusStep step = state.Clear(shown, true);
    Check(step.draw == StatusDraw::kNothing && Is(state.Drawn(), "Card removed."), "a line showing, retracted under an overlay, stays until it closes");
    const StatusStep closed = state.OverlaysClosed();
    Check(closed.draw == StatusDraw::kBlank && closed.sound == StatusSound::kNone, "then the band goes blank, silent");
}

void TestHeldSameAsDrawn()
{
    StatusState state = WithBand();
    state.Say(StatusKind::kSuccess, "Card read.", false);
    state.Say(StatusKind::kError, "No card.", true);
    const StatusStep again = state.Say(StatusKind::kSuccess, "Card read.", true);
    Check(again.id != 0, "held news that matches what the band shows is still news");
    const StatusStep closed = state.OverlaysClosed();
    Check(closed.draw == StatusDraw::kNothing && closed.sound == StatusSound::kSuccess,
          "the band already shows it, so it is not drawn again, but the press that said it hears its sound, once");
}

void TestCap()
{
    char sixty[kStatusMaxChars + 1];
    std::memset(sixty, 'a', kStatusMaxChars);
    sixty[kStatusMaxChars] = '\0';
    StatusState state = WithBand();
    Check(StatusChars(sixty) == 60 && !state.Say(StatusKind::kInfo, sixty, false).over_cap, "60 characters are under the cap");
    char sixty_one[kStatusMaxChars + 2];
    std::memset(sixty_one, 'a', kStatusMaxChars + 1);
    sixty_one[kStatusMaxChars + 1] = '\0';
    Check(state.Say(StatusKind::kInfo, sixty_one, false).over_cap, "61 are over it, for the log");
    Check(StatusChars("Card \xC2\xB7 read") == 11, "a two-byte mark is one character");
    // Words past kStatusBytes are cut at a character's edge, never inside one.
    char long_words[kStatusBytes + 8];
    std::memset(long_words, 'a', sizeof long_words);
    long_words[kStatusBytes - 3] = '\xC2';  // a two-byte mark that straddles the last byte kept
    long_words[kStatusBytes - 2] = '\xB7';
    long_words[sizeof long_words - 1] = '\0';
    state.ScreenChanged(true);
    state.Say(StatusKind::kInfo, long_words, false);
    const size_t kept = std::strlen(state.Drawn().words);
    Check(kept <= kStatusBytes - 1, "the copy is bounded");
    bool whole = true;
    for (size_t i = 0; i < kept; ++i) {
        const uint8_t byte = static_cast<uint8_t>(state.Drawn().words[i]);
        if (byte == 0xC2 && (i + 1 >= kept || static_cast<uint8_t>(state.Drawn().words[i + 1]) != 0xB7)) {
            whole = false;
        }
    }
    Check(whole, "and ends on a whole character");
    Check(state.Say(StatusKind::kInfo, long_words, false).draw == StatusDraw::kNothing, "the same long words again are the same words");
}

void TestCutByBytes()
{
    // 60 three-byte marks: at the 60-character cap, but 180 bytes, more than the 128 kept.
    char marks[60 * 3 + 1];
    for (size_t i = 0; i < 60; ++i) {
        std::memcpy(marks + i * 3, "\xE2\x80\xA6", 3);  // …
    }
    marks[sizeof marks - 1] = '\0';
    StatusState state = WithBand();
    const StatusStep step = state.Say(StatusKind::kInfo, marks, false);
    Check(step.chars == 60 && !step.over_cap, "60 three-byte marks are 60 characters: under the character cap");
    Check(step.cut, "but their bytes are over what is kept: the step says it was cut, for the log");
    const size_t kept = std::strlen(state.Drawn().words);
    Check(kept <= kStatusBytes - 1 && kept % 3 == 0, "and the cut falls on a character's edge");
    Check(!state.Say(StatusKind::kInfo, "Card read.", false).cut, "a short line is not cut");
}

void TestIdWrap()
{
    StatusState state(UINT32_MAX - 1);
    state.ScreenChanged(true);
    const StatusId before = state.Say(StatusKind::kInfo, "a", false).id;
    const StatusId at_max = state.Say(StatusKind::kInfo, "b", false).id;
    const StatusId wrapped = state.Say(StatusKind::kInfo, "c", false).id;
    const StatusId after = state.Say(StatusKind::kInfo, "d", false).id;
    Check(before == UINT32_MAX && at_max == 1 && wrapped == 2 && after == 3,
          "ids wrap past UINT32_MAX to 1, never to 0");
    Check(state.Clear(wrapped, false).draw == StatusDraw::kNothing && state.Drawn().id == after,
          "an id from before the wrap retracts nothing after it");
}

void TestIdsNeverZero()
{
    StatusState state = WithBand();
    StatusId last = 0;
    for (int i = 0; i < 1000; ++i) {
        const StatusId id = state.Say(StatusKind::kInfo, i % 2 ? "a" : "b", false).id;
        if (id == 0 || id == last) {
            Check(false, "ids are never 0 and never repeat in a row");
            return;
        }
        last = id;
    }
    Check(true, "ids are never 0 and never repeat in a row");
}

}  // namespace

int main()
{
    TestSay();
    TestReplace();
    TestSameWordsAgain();
    TestNothingSaid();
    TestClearById();
    TestScreenChange();
    TestHoldUnderOverlay();
    TestHeldRepeatSoundsOnce();
    TestHeldNewestWins();
    TestHeldThenRetracted();
    TestClearShownUnderOverlay();
    TestHeldSameAsDrawn();
    TestCap();
    TestCutByBytes();
    TestIdWrap();
    TestIdsNeverZero();
    std::printf("status_rules_test: %d checks, %d failures\n", s_checks, s_failures);
    return s_failures == 0 ? 0 : 1;
}
