// Host tests of the keyboard's pages (components/widgets/src/internal/keyboard_pages.h), on the computer with no board: six rows of
// six slots on four pages, every letter, digit and printable ASCII symbol reachable, no two keys on one slot, the bottom row's
// three keys equal and in the same slots on every page so a page switch keeps the place, and the one row with no keys the
// symbols page's fourth (STICKY_DESIGN_TOKENS §11).
// Build and run: bash tests/host/run.sh

#include <cstdio>
#include <cstring>

#include "design_tokens.h"
#include "internal/keyboard_pages.h"

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

constexpr KeyPage kAll[] = {KeyPage::kLower, KeyPage::kUpper, KeyPage::kNumbers, KeyPage::kSymbols};

constexpr int SlotsWidth(int slots)
{
    return slots * dt::kSpacingTarget + (slots - 1) * dt::kSpacingKeyGap;
}

void TestGeometry()
{
    bool in_range = true, ascending = true, sized = true;
    int empty_rows = 0;
    for (KeyPage page : kAll) {
        const RowDef *rows = PageRows(page);
        for (int r = 0; r < kKeyRows; ++r) {
            const RowDef &row = rows[r];
            if (row.count == 0) {
                ++empty_rows;
                continue;
            }
            int end = 0;
            for (int i = 0; i < row.count; ++i) {
                const KeyDef &key = row.keys[i];
                sized = sized && key.slots >= 1 && key.slots <= 2;
                in_range = in_range && key.slot < kKeySlots && key.slot + key.slots <= kKeySlots;
                ascending = ascending && key.slot >= end;  // no two keys on one slot
                end = key.slot + key.slots;
            }
        }
    }
    Check(in_range, "every key stands inside the six slots of its row");
    Check(ascending, "no two keys of a row share a slot, and they run left to right");
    Check(sized, "a key takes one slot or two");
    Check(empty_rows == 1 && PageRows(KeyPage::kSymbols)[3].count == 0, "the one row with no keys is the symbols page's fourth");
    Check(kKeyRows * dt::kSpacingTarget + (kKeyRows - 1) * dt::kSpacingKeyGap == 444, "six rows of 64 px and five 12 px gaps are 444 px tall");
    Check(kKeySlots * dt::kSpacingTarget + (kKeySlots - 1) * dt::kSpacingKeyGap == 444 && (480 - 444) / 2 == 18,
          "six slots across are 444 px, 18 px from each side of the glass");
}

void TestBottomRow()
{
    bool same_slots = true, equal = true;
    for (KeyPage page : kAll) {
        const RowDef &row = PageRows(page)[kKeyRows - 1];
        same_slots = same_slots && row.count == 3 && row.keys[0].slot == 0 && row.keys[1].slot == 2 && row.keys[2].slot == 4;
        equal = equal && row.keys[0].slots == 2 && row.keys[1].slots == 2 && row.keys[2].slots == 2;
    }
    Check(same_slots, "the bottom row's three keys stand in slots 0, 2 and 4 on every page, so a switch keeps the place");
    Check(equal, "the bottom row's three keys are two slots each, equal");
    Check(SlotsWidth(2) == 140 && 3 * SlotsWidth(2) + 2 * dt::kSpacingKeyGap == 444, "three equal keys of 140 px and two 12 px gaps fill the 444 px grid");
    // Each label is centred: the widest, "Shift" or "Space", is five characters of `label`, 18 px each.
    Check((SlotsWidth(2) - 5 * 18) / 2 == 25 && (SlotsWidth(2) - 3 * 18) / 2 == 43, "Shift and Space keep 25 px each side, 123, ABC and #+= 43");
    const RowDef &lower = PageRows(KeyPage::kLower)[kKeyRows - 1];
    const RowDef &upper = PageRows(KeyPage::kUpper)[kKeyRows - 1];
    const RowDef &numbers = PageRows(KeyPage::kNumbers)[kKeyRows - 1];
    const RowDef &symbols = PageRows(KeyPage::kSymbols)[kKeyRows - 1];
    Check(lower.keys[0].kind == KeyKind::kShift && upper.keys[0].kind == KeyKind::kShift, "Shift is in the first slot of both letter pages");
    Check(numbers.keys[0].kind == KeyKind::kToSymbols && symbols.keys[0].kind == KeyKind::kToNumbers,
          "the first slot reads #+= on the digits page and 123 on the symbols page");
    Check(lower.keys[1].kind == KeyKind::kToNumbers && numbers.keys[1].kind == KeyKind::kToLetters && symbols.keys[1].kind == KeyKind::kToLetters,
          "the second slot is 123 on the letters and ABC beside them");
    Check(lower.keys[2].kind == KeyKind::kSpace && numbers.keys[2].kind == KeyKind::kSpace && symbols.keys[2].kind == KeyKind::kSpace,
          "Space is the last key of every page");
    Check(std::strcmp(lower.keys[0].label, "Shift") == 0, "Shift is the word alone, no glyph in its label");
}

// How many keys on `page` type `c`.
int Count(KeyPage page, char c)
{
    int n = 0;
    for (int r = 0; r < kKeyRows; ++r) {
        const RowDef &row = PageRows(page)[r];
        for (int i = 0; i < row.count; ++i) {
            n += row.keys[i].kind == KeyKind::kChar && row.keys[i].label[0] == c;
        }
    }
    return n;
}

void TestCoverage()
{
    bool lower = true, upper = true, digits = true, once = true;
    for (char c = 'a'; c <= 'z'; ++c) {
        lower = lower && Count(KeyPage::kLower, c) == 1;
        upper = upper && Count(KeyPage::kUpper, static_cast<char>(c - 'a' + 'A')) == 1;
    }
    for (char c = '0'; c <= '9'; ++c) {
        digits = digits && Count(KeyPage::kNumbers, c) == 1;
    }
    for (KeyPage page : kAll) {
        for (char c = 0x21; c <= 0x7E; ++c) {
            once = once && Count(page, c) <= 1;
        }
    }
    Check(lower, "every lower-case letter is on the letters page once, in the order of the alphabet");
    Check(upper, "every capital is on the capitals page once");
    Check(digits, "every digit is on the digits page once");
    Check(once, "no page types one character on two keys");
    bool reachable = true;
    for (char c = 0x21; c <= 0x7E; ++c) {
        int n = 0;
        for (KeyPage page : kAll) {
            n += Count(page, c);
        }
        if (n == 0) {
            reachable = false;
            std::printf("      '%c' is on no page\n", c);
        }
    }
    Check(reachable, "every printable ASCII character is on some page (a Wi-Fi password may hold any of them)");
    // Order: a..f on the first row, then g..l, m..r, s..x.
    Check(PageRows(KeyPage::kLower)[0].keys[0].label[0] == 'a' && PageRows(KeyPage::kLower)[3].keys[5].label[0] == 'x' &&
              PageRows(KeyPage::kLower)[4].keys[0].label[0] == 'y',
          "the letters run a to x over four rows, then y and z");
}

void TestEditKeys()
{
    bool delete_found = true;
    for (KeyPage page : kAll) {
        const RowDef &row = PageRows(page)[4];
        delete_found = delete_found && row.keys[row.count - 1].kind == KeyKind::kDelete && row.keys[row.count - 1].slot == 4 &&
                       row.keys[row.count - 1].slots == 2;
    }
    Check(delete_found, "Delete ends the fifth row on every page, two slots wide at the right");
    Check(RowFirstSlot(PageRows(KeyPage::kSymbols)[4]) == 4 && RowEndSlot(PageRows(KeyPage::kSymbols)[4]) == 6,
          "a row with only Delete has a ring round that key alone");
    Check(RowFirstSlot(PageRows(KeyPage::kLower)[0]) == 0 && RowEndSlot(PageRows(KeyPage::kLower)[0]) == 6, "a full row's ring wraps all six slots");
}

}  // namespace

int main()
{
    TestGeometry();
    TestBottomRow();
    TestCoverage();
    TestEditKeys();
    std::printf("keyboard_pages: %d of %d checks pass\n", s_checks - s_failures, s_checks);
    return s_failures == 0 ? 0 : 1;
}
