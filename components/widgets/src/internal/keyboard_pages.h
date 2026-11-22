// The keyboard's pages, with no LVGL in it: six rows of six slots on four pages — letters, capitals, digits with common
// punctuation (123) and the rest (#+=) — as the person-approved draft draws them, so tests/host checks that every page
// holds what it should and that the page keys never move (STICKY_DESIGN_TOKENS §11).
//
// A row is six slots, each a 64 px key and the `spacing.key-gap` after it; a key takes one slot, or two joined with
// the gap between them (Delete, and the bottom row's three equal keys: 140 px each across the 444 px grid). A row with no keys draws nothing and holds no stop.

#pragma once

#include <cstdint>

namespace widgets::detail {

inline constexpr int kKeyRows = 6;
inline constexpr int kKeySlots = 6;
inline constexpr int kMaxKeysInRow = 6;

enum class KeyPage : uint8_t { kLower, kUpper, kNumbers, kSymbols };

// What a key does when pressed: types its first character, or one of the specials.
enum class KeyKind : uint8_t {
    kChar,        // types its label's character
    kSpace,       // types a space
    kDelete,      // removes the last character
    kShift,       // toggles the capitals page; on, it is the chosen inversion
    kToLetters,   // ABC: back to letters (the capitals page while Shift is on)
    kToNumbers,   // 123
    kToSymbols,   // #+=
};

struct KeyDef {
    const char *label;
    KeyKind kind;
    uint8_t slot;   // the first slot it stands in, 0 to 5
    uint8_t slots;  // how many it joins, 1 or 2
};

struct RowDef {
    const KeyDef *keys;
    uint8_t count;
};

namespace pages {

// The letters in the order of the alphabet: a person who has never typed on a phone can find them.
inline constexpr KeyDef kLowerRow0[] = {{"a", KeyKind::kChar, 0, 1}, {"b", KeyKind::kChar, 1, 1}, {"c", KeyKind::kChar, 2, 1},
                                        {"d", KeyKind::kChar, 3, 1}, {"e", KeyKind::kChar, 4, 1}, {"f", KeyKind::kChar, 5, 1}};
inline constexpr KeyDef kLowerRow1[] = {{"g", KeyKind::kChar, 0, 1}, {"h", KeyKind::kChar, 1, 1}, {"i", KeyKind::kChar, 2, 1},
                                        {"j", KeyKind::kChar, 3, 1}, {"k", KeyKind::kChar, 4, 1}, {"l", KeyKind::kChar, 5, 1}};
inline constexpr KeyDef kLowerRow2[] = {{"m", KeyKind::kChar, 0, 1}, {"n", KeyKind::kChar, 1, 1}, {"o", KeyKind::kChar, 2, 1},
                                        {"p", KeyKind::kChar, 3, 1}, {"q", KeyKind::kChar, 4, 1}, {"r", KeyKind::kChar, 5, 1}};
inline constexpr KeyDef kLowerRow3[] = {{"s", KeyKind::kChar, 0, 1}, {"t", KeyKind::kChar, 1, 1}, {"u", KeyKind::kChar, 2, 1},
                                        {"v", KeyKind::kChar, 3, 1}, {"w", KeyKind::kChar, 4, 1}, {"x", KeyKind::kChar, 5, 1}};
inline constexpr KeyDef kLowerRow4[] = {{"y", KeyKind::kChar, 0, 1}, {"z", KeyKind::kChar, 1, 1}, {".", KeyKind::kChar, 2, 1},
                                        {"@", KeyKind::kChar, 3, 1}, {"Delete", KeyKind::kDelete, 4, 2}};
inline constexpr KeyDef kLowerRow5[] = {{"Shift", KeyKind::kShift, 0, 2}, {"123", KeyKind::kToNumbers, 2, 2},
                                        {"Space", KeyKind::kSpace, 4, 2}};

inline constexpr KeyDef kUpperRow0[] = {{"A", KeyKind::kChar, 0, 1}, {"B", KeyKind::kChar, 1, 1}, {"C", KeyKind::kChar, 2, 1},
                                        {"D", KeyKind::kChar, 3, 1}, {"E", KeyKind::kChar, 4, 1}, {"F", KeyKind::kChar, 5, 1}};
inline constexpr KeyDef kUpperRow1[] = {{"G", KeyKind::kChar, 0, 1}, {"H", KeyKind::kChar, 1, 1}, {"I", KeyKind::kChar, 2, 1},
                                        {"J", KeyKind::kChar, 3, 1}, {"K", KeyKind::kChar, 4, 1}, {"L", KeyKind::kChar, 5, 1}};
inline constexpr KeyDef kUpperRow2[] = {{"M", KeyKind::kChar, 0, 1}, {"N", KeyKind::kChar, 1, 1}, {"O", KeyKind::kChar, 2, 1},
                                        {"P", KeyKind::kChar, 3, 1}, {"Q", KeyKind::kChar, 4, 1}, {"R", KeyKind::kChar, 5, 1}};
inline constexpr KeyDef kUpperRow3[] = {{"S", KeyKind::kChar, 0, 1}, {"T", KeyKind::kChar, 1, 1}, {"U", KeyKind::kChar, 2, 1},
                                        {"V", KeyKind::kChar, 3, 1}, {"W", KeyKind::kChar, 4, 1}, {"X", KeyKind::kChar, 5, 1}};
inline constexpr KeyDef kUpperRow4[] = {{"Y", KeyKind::kChar, 0, 1}, {"Z", KeyKind::kChar, 1, 1}, {".", KeyKind::kChar, 2, 1},
                                        {"@", KeyKind::kChar, 3, 1}, {"Delete", KeyKind::kDelete, 4, 2}};

inline constexpr KeyDef kNumbersRow0[] = {{"1", KeyKind::kChar, 0, 1}, {"2", KeyKind::kChar, 1, 1}, {"3", KeyKind::kChar, 2, 1},
                                          {"4", KeyKind::kChar, 3, 1}, {"5", KeyKind::kChar, 4, 1}, {"6", KeyKind::kChar, 5, 1}};
inline constexpr KeyDef kNumbersRow1[] = {{"7", KeyKind::kChar, 0, 1}, {"8", KeyKind::kChar, 1, 1}, {"9", KeyKind::kChar, 2, 1},
                                          {"0", KeyKind::kChar, 3, 1}, {"-", KeyKind::kChar, 4, 1}, {"/", KeyKind::kChar, 5, 1}};
inline constexpr KeyDef kNumbersRow2[] = {{":", KeyKind::kChar, 0, 1}, {";", KeyKind::kChar, 1, 1}, {"(", KeyKind::kChar, 2, 1},
                                          {")", KeyKind::kChar, 3, 1}, {"$", KeyKind::kChar, 4, 1}, {"&", KeyKind::kChar, 5, 1}};
inline constexpr KeyDef kNumbersRow3[] = {{"@", KeyKind::kChar, 0, 1}, {"\"", KeyKind::kChar, 1, 1}, {".", KeyKind::kChar, 2, 1},
                                          {",", KeyKind::kChar, 3, 1}, {"?", KeyKind::kChar, 4, 1}, {"!", KeyKind::kChar, 5, 1}};
inline constexpr KeyDef kNumbersRow4[] = {{"'", KeyKind::kChar, 0, 1}, {"+", KeyKind::kChar, 1, 1}, {"_", KeyKind::kChar, 2, 1},
                                          {"#", KeyKind::kChar, 3, 1}, {"Delete", KeyKind::kDelete, 4, 2}};
inline constexpr KeyDef kNumbersRow5[] = {{"#+=", KeyKind::kToSymbols, 0, 2}, {"ABC", KeyKind::kToLetters, 2, 2},
                                          {"Space", KeyKind::kSpace, 4, 2}};

inline constexpr KeyDef kSymbolsRow0[] = {{"=", KeyKind::kChar, 0, 1}, {"*", KeyKind::kChar, 1, 1}, {"%", KeyKind::kChar, 2, 1},
                                          {"^", KeyKind::kChar, 3, 1}, {"~", KeyKind::kChar, 4, 1}, {"|", KeyKind::kChar, 5, 1}};
inline constexpr KeyDef kSymbolsRow1[] = {{"\\", KeyKind::kChar, 0, 1}, {"{", KeyKind::kChar, 1, 1}, {"}", KeyKind::kChar, 2, 1},
                                          {"[", KeyKind::kChar, 3, 1}, {"]", KeyKind::kChar, 4, 1}, {"`", KeyKind::kChar, 5, 1}};
inline constexpr KeyDef kSymbolsRow2[] = {{"<", KeyKind::kChar, 0, 1}, {">", KeyKind::kChar, 1, 1}};
inline constexpr KeyDef kSymbolsRow4[] = {{"Delete", KeyKind::kDelete, 4, 2}};
inline constexpr KeyDef kSymbolsRow5[] = {{"123", KeyKind::kToNumbers, 0, 2}, {"ABC", KeyKind::kToLetters, 2, 2},
                                          {"Space", KeyKind::kSpace, 4, 2}};

#define WIDGETS_KEY_ROW(array) RowDef{array, static_cast<uint8_t>(sizeof(array) / sizeof(array[0]))}

inline constexpr RowDef kLower[kKeyRows] = {WIDGETS_KEY_ROW(kLowerRow0), WIDGETS_KEY_ROW(kLowerRow1), WIDGETS_KEY_ROW(kLowerRow2),
                                            WIDGETS_KEY_ROW(kLowerRow3), WIDGETS_KEY_ROW(kLowerRow4), WIDGETS_KEY_ROW(kLowerRow5)};
inline constexpr RowDef kUpper[kKeyRows] = {WIDGETS_KEY_ROW(kUpperRow0), WIDGETS_KEY_ROW(kUpperRow1), WIDGETS_KEY_ROW(kUpperRow2),
                                            WIDGETS_KEY_ROW(kUpperRow3), WIDGETS_KEY_ROW(kUpperRow4), WIDGETS_KEY_ROW(kLowerRow5)};
inline constexpr RowDef kNumbers[kKeyRows] = {WIDGETS_KEY_ROW(kNumbersRow0), WIDGETS_KEY_ROW(kNumbersRow1),
                                              WIDGETS_KEY_ROW(kNumbersRow2), WIDGETS_KEY_ROW(kNumbersRow3),
                                              WIDGETS_KEY_ROW(kNumbersRow4), WIDGETS_KEY_ROW(kNumbersRow5)};
inline constexpr RowDef kSymbols[kKeyRows] = {WIDGETS_KEY_ROW(kSymbolsRow0), WIDGETS_KEY_ROW(kSymbolsRow1),
                                              WIDGETS_KEY_ROW(kSymbolsRow2), RowDef{nullptr, 0}, WIDGETS_KEY_ROW(kSymbolsRow4),
                                              WIDGETS_KEY_ROW(kSymbolsRow5)};

#undef WIDGETS_KEY_ROW

}  // namespace pages

// The six rows of a page.
constexpr const RowDef *PageRows(KeyPage page)
{
    return page == KeyPage::kLower ? pages::kLower
           : page == KeyPage::kUpper ? pages::kUpper
           : page == KeyPage::kNumbers ? pages::kNumbers
                                       : pages::kSymbols;
}

// The first slot of the first key and the end slot of the last key of a row: the extent its focus ring wraps. A row with no
// keys has none.
constexpr int RowFirstSlot(const RowDef &row)
{
    return row.count == 0 ? 0 : row.keys[0].slot;
}
constexpr int RowEndSlot(const RowDef &row)
{
    return row.count == 0 ? 0 : row.keys[row.count - 1].slot + row.keys[row.count - 1].slots;
}

}  // namespace widgets::detail
