// Host tests of the scroll indicator's thumb rule (components/widgets/src/internal/scroll_rules.h): where the thumb
// stands for each page of 1 to 60, on the computer with no board — the ends kept by the first and last page only, every
// page its own place, a short list unchanged from the proportional rule (DESIGN → Components → The kit).
// Build and run: bash tests/host/run.sh

#include <algorithm>
#include <cstdio>

#include "design_tokens.h"
#include "internal/scroll_rules.h"

using namespace widgets::detail;
namespace dt = design_tokens;

namespace {

int s_failures = 0;
int s_checks = 0;

void Check(bool ok, const char *what, int pages = 0, int page = 0)
{
    ++s_checks;
    if (!ok) {
        ++s_failures;
        std::printf("FAIL  %s (pages %d, page %d)\n", what, pages, page);
    }
}

constexpr int32_t kStroke = dt::kSpacingScrollThumb;
constexpr int32_t kHold = dt::kSpacingSm + kStroke;  // a gap and a dot of track

// The room a paged view's thumb has: the lane less the view's share of it, never under a dot — as the widget works it out
// for a list (view = one page, content = `pages` pages).
int32_t RoomFor(int pages)
{
    const int32_t whole = dt::kSpacingScrollLength;
    return whole - std::max<int32_t>(kStroke, whole / pages);
}

// The rule before the spread, as it stood: proportional, held at an end until a gap and a dot of track fit behind it.
int32_t OldAt(int32_t room, int page, int pages)
{
    int32_t at = pages <= 1 ? 0 : ProportionalAt(room, page, pages - 1);
    if (at > 0 && at < kHold) {
        at = 0;
    } else if (at < room && room - at < kHold) {
        at = room;
    }
    return at;
}

void TestEveryPageItsOwnPlace()
{
    for (int pages = 2; pages <= 60; ++pages) {
        const int32_t room = RoomFor(pages);
        Check(ThumbAt(room, kHold, 0, pages) == 0, "the first page is at the top", pages, 0);
        Check(ThumbAt(room, kHold, pages - 1, pages) == room, "the last page is at the bottom", pages, pages - 1);
        for (int page = 1; page < pages; ++page) {
            Check(ThumbAt(room, kHold, page, pages) > ThumbAt(room, kHold, page - 1, pages), "each page is below the one before",
                  pages, page);
        }
        for (int page = 1; page < pages - 1; ++page) {
            const int32_t at = ThumbAt(room, kHold, page, pages);
            Check(at >= kHold && at <= room - kHold, "an interior page is a gap and a dot in from each end", pages, page);
        }
    }
}

void TestShortListsUnchanged()
{
    for (int pages = 1; pages <= 15; ++pages) {
        const int32_t room = RoomFor(pages);
        for (int page = 0; page < pages; ++page) {
            Check(ThumbAt(room, kHold, page, pages) == OldAt(room, page, pages), "15 pages or fewer are proportional as before",
                  pages, page);
        }
    }
    Check(ThumbAt(RoomFor(1), kHold, 0, 1) == 0, "one page stands at the top", 1, 0);
}

// Where the rule differs from the old one, the old one had pages on the same spot: that is the crowded list.
void TestOnlyCrowdedListsChange()
{
    for (int pages = 2; pages <= 60; ++pages) {
        const int32_t room = RoomFor(pages);
        bool differs = false;
        bool doubled = false;
        for (int page = 0; page < pages; ++page) {
            differs |= ThumbAt(room, kHold, page, pages) != OldAt(room, page, pages);
            doubled |= page > 0 && OldAt(room, page, pages) == OldAt(room, page - 1, pages);
        }
        Check(differs == doubled, "a list changes only where the old rule put two pages on one spot", pages);
    }
}

void PrintPositions()
{
    for (int pages : {2, 5, 15, 16, 20, 40}) {
        const int32_t room = RoomFor(pages);
        std::printf("pages %2d, room %3d:", pages, room);
        for (int page = 0; page < pages; ++page) {
            std::printf(" %d", ThumbAt(room, kHold, page, pages));
        }
        std::printf("\n");
    }
}

}  // namespace

int main()
{
    TestEveryPageItsOwnPlace();
    TestShortListsUnchanged();
    TestOnlyCrowdedListsChange();
    PrintPositions();
    std::printf("scroll_test: %d checks, %d failed\n", s_checks, s_failures);
    return s_failures == 0 ? 0 : 1;
}
