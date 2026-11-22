// Host tests of the select sheet's arithmetic (components/widgets/src/internal/sheet_rules.h), on the computer with no board: the
// height for three and four rows, and which a field gets — four when the sheet leaves the field clear, else three
// (STICKY_DESIGN_TOKENS §12).
// Build and run: bash tests/host/run.sh

#include <cstdio>

#include "design_tokens.h"
#include "internal/sheet_rules.h"

using namespace widgets::detail::sheet;
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

constexpr int32_t kGlass = 800;

void TestHeights()
{
    Check(Band(4) == 304 && Band(3) == 224 && Band(2) == 144, "four rows and three gaps are 304 px, three rows 224, two 144");
    Check(Height(4) == 488 && Height(3) == 408 && Height(2) == 328, "the sheet is 488 px for four rows, 408 for three, 328 for two");
    Check(Height(4) == 80 * 4 + 168 && Height(3) == 80 * 3 + 168, "a sheet is 80 px for each row and 168 for the rest");
    Check(kGlass - Height(4) == 312 && kGlass - Height(3) == 392, "the sheet's top is at y 312 for four rows and 392 for three");
    Check(Clearance() == 22, "the ring, white and the offset above the sheet are 22 px");
}

void TestRowsPerPage()
{
    // The draft's page: Remind me ends at y 182, Repeat at 262, the disabled Alert at 342.
    Check(RowsPerPage(182, kGlass) == 4 && RowsPerPage(262, kGlass) == 4, "the first two fields leave room for four rows");
    Check(RowsPerPage(290, kGlass) == 4, "a field ending at y 290 is the last to take four");
    Check(RowsPerPage(291, kGlass) == 3, "a field ending at y 291 takes three");
    Check(RowsPerPage(342, kGlass) == 3 && RowsPerPage(370, kGlass) == 3, "a field between 291 and 370 takes three");
    Check(RowsPerPage(371, kGlass) == 3 && RowsPerPage(700, kGlass) == 3, "a field below 370 is covered even by three: never fewer");
    Check(Clearance() + 290 <= kGlass - Height(4) && Clearance() + 291 > kGlass - Height(4), "290 is the edge of the clearance's arithmetic");
    Check(Clearance() + 370 <= kGlass - Height(3) && Clearance() + 371 > kGlass - Height(3), "370 is the edge of three rows' clearance");
}

static_assert(Height(4) == 488 && RowsPerPage(290, 800) == 4 && RowsPerPage(291, 800) == 3, "the rules are constexpr");

}  // namespace

int main()
{
    TestHeights();
    TestRowsPerPage();
    std::printf("sheet_rules: %d of %d checks pass\n", s_checks - s_failures, s_checks);
    return s_failures == 0 ? 0 : 1;
}
