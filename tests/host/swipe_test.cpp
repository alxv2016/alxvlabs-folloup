// Host tests of the page swipe's decision (components/widgets/src/internal/swipe_rules.h): which drags turn a page and
// which place, on the computer with no board — a slow drag, a flick, a long drag, a pull-back, a rested finger, both
// directions and the thresholds' edges (STICKY_DESIGN_TOKENS §10).
// Build and run: bash tests/host/run.sh

#include <cstdio>

#include "design_tokens.h"
#include "internal/swipe_rules.h"

using namespace widgets::detail;
namespace dt = design_tokens;

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

// A drag is a run of moves: `count` moves of `each` px, `every_ms` apart, the first at `start_ms`. Returns the time of the
// last move.
uint32_t Run(Drag &drag, uint32_t start_ms, int count, int32_t each, uint32_t every_ms)
{
    uint32_t at = start_ms;
    for (int i = 0; i < count; ++i) {
        LogMove(drag, at, each);
        at += every_ms;
    }
    return at - every_ms;
}

void TestDistance()
{
    Drag slow;
    uint32_t last = Run(slow, 1000, 30, 1, 40);  // 30 px in 1.2 s: 2.5 px in the last 100 ms
    Check(PageTurn(slow, last + 20) == 0, "a slow 30 px drag places and turns nothing");

    Drag far;
    last = Run(far, 1000, 64, 1, 30);  // 64 px, slowly
    Check(PageTurn(far, last + 20) == 1, "64 px turns a page however slowly");
    Drag just_short;
    last = Run(just_short, 1000, 63, 1, 30);
    Check(PageTurn(just_short, last + 20) == 0, "63 px slowly turns nothing");

    Drag down;
    last = Run(down, 1000, 64, -1, 30);
    Check(PageTurn(down, last + 20) == -1, "64 px down turns to the previous page");
}

void TestFlick()
{
    Drag flick;
    uint32_t last = Run(flick, 1000, 3, 10, 20);  // 30 px in 40 ms
    Check(PageTurn(flick, last + 20) == 1, "a 30 px flick up turns a page");

    Drag flick_down;
    last = Run(flick_down, 1000, 3, -10, 20);
    Check(PageTurn(flick_down, last + 20) == -1, "a 30 px flick down turns to the previous page");

    Drag short_flick;
    last = Run(short_flick, 1000, 2, 11, 20);  // 22 px, under kTouchFlickDistance
    Check(PageTurn(short_flick, last + 20) == 0, "a fast drag shorter than a flick's distance turns nothing");

    Drag least;
    last = Run(least, 1000, 1, dt::kTouchFlickDistance, 20);  // 24 px in one move
    Check(PageTurn(least, last + 20) == 1, "a flick of exactly the flick distance turns a page");

    Drag long_flick;
    last = Run(long_flick, 1000, 4, 20, 10);  // 80 px fast: past the swipe distance either way
    Check(PageTurn(long_flick, last + 20) == 1, "a flick past 64 px turns one page");
}

void TestSlidingWindow()
{
    // 40 px dragged slowly over a second, then a short last push: only the last 100 ms is the window, so a drag that is
    // still moving at lift is a flick only if its latest movement is.
    Drag still_slow;
    uint32_t last = Run(still_slow, 1000, 40, 1, 25);  // 4 px in the last 100 ms
    Check(PageTurn(still_slow, last + 10) == 0, "40 px still moving slowly at the lift is not a flick");

    Drag speeds_up;
    last = Run(speeds_up, 1000, 20, 1, 40);            // 20 px slowly
    last = Run(speeds_up, last + 10, 2, 10, 10);       // then 20 px in 10 ms: a flick at the end
    Check(PageTurn(speeds_up, last + 10) == 1, "a drag that ends in a quick push is a flick");

    Drag fast_then_slow;
    last = Run(fast_then_slow, 1000, 3, 10, 10);       // 30 px fast, then it crawls
    last = Run(fast_then_slow, last + 50, 5, 1, 50);   // 5 px more over 200 ms
    Check(PageTurn(fast_then_slow, last + 10) == 0, "a fast start that slowed is not a flick: the window is the end");

    // The window's own edge: a move exactly kTouchFlickWindowMs before the lift is inside it, one a millisecond older is out.
    Drag edge;
    LogMove(edge, 1000, 20);
    LogMove(edge, 1100, 12);  // 12 px at 1100
    Check(MovedWithin(edge, 1100 + dt::kTouchFlickWindowMs, dt::kTouchFlickWindowMs) == 12,
          "a move exactly a window before the lift counts");
    Check(MovedWithin(edge, 1100 + dt::kTouchFlickWindowMs + 1, dt::kTouchFlickWindowMs) == 0,
          "a move a millisecond older than the window does not");
}

void TestRefused()
{
    Drag pull_back;
    uint32_t last = Run(pull_back, 1000, 8, 10, 10);   // up 80 px: past the swipe distance...
    last = Run(pull_back, last + 10, 6, -10, 10);      // ...and back 60: net 20, under every limit
    Check(PageTurn(pull_back, last + 10) == 0, "a pull-back to a short net travel turns nothing");

    Drag pulled_to_flickable;
    last = Run(pulled_to_flickable, 1000, 10, 10, 10);  // up 100
    last = Run(pulled_to_flickable, last + 10, 5, -10, 10);  // quickly back 50: net 50, over the flick distance
    Check(PageTurn(pulled_to_flickable, last + 10) == 0, "a fast pull-back is a flick the other way and turns nothing");

    Drag pulled_below;
    last = Run(pulled_below, 1000, 10, 10, 100);       // up 100 px over a second
    last = Run(pulled_below, last + 10, 4, -10, 10);   // net 60: just under 64, the last movement against it
    Check(PageTurn(pulled_below, last + 10) == 0, "net travel under 64 px with the last movement back turns nothing");

    Drag rested;
    last = Run(rested, 1000, 3, 10, 20);               // a 30 px flick...
    Check(PageTurn(rested, last + 300) == 0, "a finger that rested before lifting places, never throws");
    Check(PageTurn(rested, last + 50) == 1, "a lift soon after the last move is still the flick");

    Drag none;
    Check(PageTurn(none, 5000) == 0, "a touch that never dragged turns nothing");
}

void TestRingAndWrap()
{
    // More moves than the ring holds: the window is still the newest moves.
    Drag many;
    uint32_t last = Run(many, 0, 100, 1, 5);  // 100 px in 495 ms, 1 px a 5 ms: 20 px in the last 100 ms
    Check(MovedWithin(many, last + 1, dt::kTouchFlickWindowMs) == 20, "the ring keeps the window's moves");

    // A faster read than the ring holds still reads a fast drag as one.
    Drag flood;
    last = Run(flood, 0, 300, 1, 1);
    Check(PageTurn(flood, last + 1) == 1, "a drag read every millisecond is read as a long drag");
    Drag flood_slow;
    last = Run(flood_slow, 0, 40, 1, 1);  // 40 px in 40 ms, then still
    Check(PageTurn(flood_slow, last + 400) == 0, "a flood of moves that stopped is a rested finger");

    // A tick that wraps past 2^32 is read as the time between.
    Drag wrapped;
    const uint32_t near_end = 0xFFFFFFF0u;
    last = Run(wrapped, near_end, 3, 10, 20);  // the ticks cross zero
    Check(PageTurn(wrapped, last + 20) == 1, "a flick across the tick counter's wrap still turns a page");
    Check(PageTurn(wrapped, last + 400) == 0, "a rest across the wrap still places");
}

}  // namespace

int main()
{
    TestDistance();
    TestFlick();
    TestSlidingWindow();
    TestRefused();
    TestRingAndWrap();
    std::printf("swipe_test: %d checks, %d failed\n", s_checks, s_failures);
    return s_failures == 0 ? 0 : 1;
}
