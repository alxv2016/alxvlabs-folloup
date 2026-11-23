// Host tests of the roller's rules (components/widgets/src/internal/roller_rules.h), on the computer with no board: a step, the
// wrap of a circular list and the stop of any other, the blank neighbour, what a tap does, and what keeping and cancelling an edit
// leave (STICKY_DESIGN_TOKENS §9).
// Build and run: bash tests/host/run.sh

#include <cstdio>

#include "internal/roller_rules.h"

using namespace widgets::detail::roller;

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

void TestStep()
{
    // A list of 12 that stops: UP steps to the value above, the earlier index.
    Check(Step(5, 12, -1, false).index == 4 && Step(5, 12, -1, false).moved, "UP steps to the value above");
    Check(Step(5, 12, +1, false).index == 6 && Step(5, 12, +1, false).moved, "DOWN steps to the value below");
    Check(!Step(0, 12, -1, false).moved && Step(0, 12, -1, false).index == 0, "UP at the first value stops, and says it did not move");
    Check(!Step(11, 12, +1, false).moved && Step(11, 12, +1, false).index == 11, "DOWN at the last value stops, and says it did not move");
    Check(Step(0, 12, +1, false).moved && Step(11, 12, -1, false).moved, "a step away from an end moves");
    // A circular list: hours 0 to 23 wrap with a move each time.
    Check(Step(0, 24, -1, true).index == 23 && Step(0, 24, -1, true).moved, "UP at 00 wraps to 23");
    Check(Step(23, 24, +1, true).index == 0 && Step(23, 24, +1, true).moved, "DOWN at 23 wraps to 00");
    Check(Step(59, 60, +1, true).index == 0, "minutes wrap from 59 to 00");
    // Held: sixty steps down from any minute are back where they began.
    int at = 17;
    for (int i = 0; i < 60; ++i) {
        at = Step(at, 60, +1, true).index;
    }
    Check(at == 17, "a held key on a circular roller goes round and arrives where it began");
    // A held key on a list that stops reaches the end and stays, every step answering not moved.
    at = 3;
    int stops = 0;
    for (int i = 0; i < 30; ++i) {
        const Move m = Step(at, 12, +1, false);
        at = m.index;
        stops += m.moved ? 0 : 1;
    }
    Check(at == 11 && stops == 30 - 8, "a held key on a list that stops reaches the last value and then every step is an end");
    Check(Wrap(-1, 5) == 4 && Wrap(5, 5) == 0 && Wrap(-6, 5) == 4 && Wrap(7, 5) == 2, "Wrap goes round either way");
}

void TestNeighbour()
{
    Check(Neighbour(0, 12, -1, false) == -1, "above the first value of a list that stops is a blank row");
    Check(Neighbour(11, 12, +1, false) == -1, "below the last value of a list that stops is a blank row");
    Check(Neighbour(0, 12, +1, false) == 1 && Neighbour(5, 12, -1, false) == 4, "inside a list the neighbours are the next values");
    Check(Neighbour(0, 24, -1, true) == 23 && Neighbour(23, 24, +1, true) == 0, "a circular roller has no blank row: 23 above 00, 00 below 23");
    Check(Neighbour(5, 12, 0, false) == 5, "the middle row is the value");
}

void TestTaps()
{
    Check(TapOn(false, 0) == Tap::kOpen && TapOn(false, 1) == Tap::kOpen && TapOn(false, 2) == Tap::kOpen, "a tap on a closed roller opens it, whichever row");
    Check(TapOn(true, 0) == Tap::kStepUp, "open: the row above steps up");
    Check(TapOn(true, 2) == Tap::kStepDown, "open: the row below steps down");
    Check(TapOn(true, 1) == Tap::kKeep, "open: the middle row keeps");
    Check(RowAt(0, 64) == 0 && RowAt(63, 64) == 0 && RowAt(64, 64) == 1 && RowAt(128, 64) == 2 && RowAt(191, 64) == 2, "the rows are 64 px each");
    Check(RowAt(-3, 64) == 0 && RowAt(500, 64) == 2, "a touch past an end lands in the end row");
}

void TestEdit()
{
    Edit edit;
    Open(edit, 8);
    Check(edit.open && edit.opened == 8, "OK opens it on the value it has");
    Check(!Keep(edit, 8) && !edit.open, "keeping the value it opened on reports no change and closes");
    Open(edit, 8);
    Check(Keep(edit, 9) && !edit.open, "keeping another value reports one change and closes");
    Check(!Keep(edit, 12), "closed, keeping again reports nothing: one change, once");
    Open(edit, 8);
    Check(Cancel(edit) == 8 && !edit.open, "cancelling puts back the value it opened on and closes");
    Check(!Keep(edit, 8), "and closing after a cancel reports nothing");
    Open(edit, 0);
    Check(Keep(edit, 23) && !Keep(edit, 23), "a wrapped value is a change, reported once");
}

}  // namespace

int main()
{
    TestStep();
    TestNeighbour();
    TestTaps();
    TestEdit();
    std::printf("roller_rules: %d of %d checks pass\n", s_checks - s_failures, s_checks);
    return s_failures == 0 ? 0 : 1;
}
