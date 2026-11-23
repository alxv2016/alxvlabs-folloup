// Host tests of the buttons' rules (components/lvgl_display/include/key_rules.h), on the computer with no board: a held UP or
// DOWN steps again after `motion.key-repeat-delay` (600 ms), then every `motion.key-repeat-period` (500 ms), only with no refresh
// running and the last step's frame on the glass — or, for a step that drew nothing, once 600 ms have passed since it; a release
// stops it at once and nothing is queued; OK never repeats, and a held OK goes back once however long it is held (DESIGN → Focus
// and input; STICKY_DESIGN_TOKENS §5).
// Build and run: bash tests/host/run.sh

#include <cstdio>

#include "design_tokens.h"
#include "key_rules.h"

using namespace lvgl_display;

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

constexpr int64_t Ms(int64_t ms)
{
    return ms * 1000;
}

void TestTokens()
{
    Check(design_tokens::kMotionKeyRepeatDelayMs == 600 && kKeyRepeatDelayUs == Ms(600), "the delay is the token's 600 ms");
    Check(design_tokens::kMotionKeyRepeatPeriodMs == 500 && kKeyRepeatPeriodUs == Ms(500), "the period is the token's 500 ms");
}

void TestDelay()
{
    KeyRepeat r;
    r.Press(0, 0);
    Check(!r.Step(Ms(599), Ms(100), false), "no step before the key has been held 600 ms");
    Check(r.Step(Ms(600), Ms(100), false), "the first step at 600 ms");
    Check(r.last_step_us == Ms(600), "the step taken is the last one");

    // The press reached LVGL late — queued behind a refresh — and the refresh that ended after it is not its frame.
    KeyRepeat late;
    late.Press(0, Ms(900));
    Check(!late.Step(Ms(1000), Ms(950), false), "a press LVGL acted on late waits a period from when it acted");
    Check(late.Step(Ms(1400), Ms(1300), false), "and steps once its frame has landed and the period passed");
}

void TestPeriod()
{
    KeyRepeat r;
    r.Press(0, 0);
    Check(r.Step(Ms(600), Ms(100), false), "the first step");
    Check(!r.Step(Ms(1099), Ms(700), false), "no second step before 500 ms after the first");
    Check(r.Step(Ms(1100), Ms(700), false), "the second step 500 ms after the first, its frame landed");
    Check(!r.Step(Ms(1100), Ms(1200), false), "never two steps in one read");
    Check(r.Step(Ms(1600), Ms(1200), false), "the third 500 ms after the second");
}

void TestFrameGate()
{
    KeyRepeat r;
    r.Press(0, 0);
    Check(r.Step(Ms(600), Ms(100), false), "the first step");
    // The step's frame has not landed: the last refresh ended before it.
    Check(!r.Step(Ms(1100), Ms(500), false), "no step while the last step's frame is not on the glass");
    Check(!r.Step(Ms(1199), Ms(500), false), "still none before the no-frame wait");
    Check(r.Step(Ms(1200), Ms(500), false), "a step that drew nothing lets the next go 600 ms after it");
}

void TestBusyGate()
{
    KeyRepeat r;
    r.Press(0, 0);
    Check(!r.Step(Ms(600), Ms(100), true), "no step while a refresh runs");
    Check(!r.Step(Ms(5000), Ms(100), true), "however long the refresh runs");
    Check(r.Step(Ms(5010), Ms(5005), false), "the step comes once the refresh ends");
    Check(!r.Step(Ms(5510), Ms(5400), true), "and the next waits on the next refresh too");
}

void TestRelease()
{
    KeyRepeat r;
    r.Press(0, 0);
    Check(r.Step(Ms(600), Ms(100), false), "the first step");
    r.Release();
    Check(!r.Step(Ms(1100), Ms(700), false), "a release stops the stepping at once");
    Check(!r.Step(Ms(9000), Ms(700), false), "and nothing is queued for later");
    r.Press(Ms(9000), Ms(9000));
    Check(!r.Step(Ms(9100), Ms(9050), false), "a new press starts the delay again: no catching up");
    Check(r.Step(Ms(9600), Ms(9050), false), "its first step at 600 ms");
}

void TestOkNeverRepeats()
{
    Check(KeyRepeats(Key::kUp) && KeyRepeats(Key::kDown), "UP and DOWN step when held");
    Check(!KeyRepeats(Key::kOk), "OK never does");

    // A held OK, read every 10 ms for 2 s, as LVGL's keypad read sees it: one escape, at 1 s, and none at the release.
    OkHold hold;
    int escapes = 0;
    int64_t first = -1;
    for (int64_t t = 0; t <= Ms(2000); t += Ms(10)) {
        if (hold.WhileDown(true, 0, t)) {
            ++escapes;
            if (first < 0) {
                first = t;
            }
        }
    }
    Check(escapes == 1, "a held OK goes back once, however long it is held");
    Check(first == kBackHoldUs, "at 1 s");
    Check(!hold.AtRelease(0, Ms(2000)), "its release neither goes back again nor presses");

    // A hold LVGL saw only at its release — it ended during a refresh — goes back then, once.
    OkHold unseen;
    Check(unseen.AtRelease(Ms(100), Ms(1200)), "a hold seen only at its release goes back");
    Check(!unseen.AtRelease(Ms(100), Ms(1200)), "once");
    Check(!unseen.AtRelease(Ms(3000), Ms(3300)), "a short OK is a press");
    Check(!unseen.WhileDown(false, Ms(3000), Ms(4500)), "nothing goes back once OK is up");
}

}  // namespace

int main()
{
    TestTokens();
    TestDelay();
    TestPeriod();
    TestFrameGate();
    TestBusyGate();
    TestRelease();
    TestOkNeverRepeats();
    std::printf("key rules: %d checks, %d failed\n", s_checks, s_failures);
    return s_failures == 0 ? 0 : 1;
}
