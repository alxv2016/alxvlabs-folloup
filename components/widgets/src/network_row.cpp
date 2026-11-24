#include "widgets/network_row.h"

#include "design_tokens.h"
#include "internal/base.h"
#include "internal/marks.h"

namespace widgets {
namespace dt = design_tokens;

namespace {
constexpr int32_t kSignalGlyphWidth = 40;
constexpr int32_t kSignalGlyphHeight = 30;
// The name's and the facts' room: the row's 400 inside its padding, less the signal glyph, which stands alone, and a gap: 344 px,
// 28 characters of `caption` at 12 px a character.
constexpr int32_t kWords = dt::kSpacingContent - 2 * dt::kSpacingMd - kSignalGlyphWidth - dt::kSpacingMd;

lv_obj_t *Mark(lv_obj_t *parent, const lv_image_dsc_t *glyph, lv_color_t colour)
{
    lv_obj_t *image = lv_image_create(parent);
    lv_image_set_src(image, glyph);
    lv_obj_set_style_image_recolor(image, colour, 0);
    lv_obj_set_style_image_recolor_opa(image, LV_OPA_COVER, 0);
    lv_obj_remove_flag(image, LV_OBJ_FLAG_CLICKABLE);
    return image;
}

// A `caption` word, `text-secondary` at rest and `text-on-fill` on the inverted row. Its colour is its own: the row's states
// do not reach a label that has one.
lv_obj_t *Word(lv_obj_t *parent, const char *text, bool inverted)
{
    lv_obj_t *word = detail::Line(parent, ui::Type::kCaption, text, !inverted);
    if (inverted) {
        lv_obj_set_style_text_color(word, ui::Level(dt::kColorTextOnFill), 0);
    }
    return word;
}

// One fact: an optional glyph, then its word, `spacing.sm` apart. A second fact stands `spacing.lg` after the first one's word
// (the draft's 8 px gap between facts, with the 8 px either side of it).
void Fact(lv_obj_t *facts, const lv_image_dsc_t *glyph, const char *text, bool inverted, bool second = false)
{
    lv_obj_t *first = nullptr;
    if (glyph != nullptr) {
        first = Mark(facts, glyph, ui::Level(inverted ? dt::kColorTextOnFill : dt::kColorTextPrimary));
    }
    lv_obj_t *word = Word(facts, text, inverted);
    if (second) {
        lv_obj_set_style_margin_left(first != nullptr ? first : word, dt::kSpacingMd, 0);
    }
}

const lv_image_dsc_t *SignalLit(int signal)
{
    return signal >= 3 ? &widgets_mark_glyph_signal_strong
           : signal == 2 ? &widgets_mark_glyph_signal_fair
           : signal == 1 ? &widgets_mark_glyph_signal_weak
                         : nullptr;
}

// Everything under the name: the facts line and the signal glyph, drawn for the state.
void Fill(lv_obj_t *row, bool locked, int signal, NetworkState state)
{
    const bool inverted = state != NetworkState::kRest;
    // The name is the row's first child, kept; the rest is made again.
    while (lv_obj_get_child_count(row) > 1) {
        lv_obj_delete(lv_obj_get_child(row, 1));
    }
    ui::SetChosen(row, inverted);

    lv_obj_t *facts = detail::Bare(row);
    lv_obj_set_size(facts, kWords, ui::LineBox(ui::Type::kCaption));
    lv_obj_set_flex_flow(facts, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(facts, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(facts, dt::kSpacingSm, 0);
    switch (state) {
    case NetworkState::kConnected:
        Fact(facts, &widgets_mark_glyph_fact_check, "Connected", inverted);
        if (locked) {
            Fact(facts, &widgets_mark_glyph_lock, "Locked", inverted, true);
        } else {
            Fact(facts, nullptr, "Open", inverted, true);
        }
        break;
    case NetworkState::kConnecting:
        Fact(facts, nullptr, "Connecting…", inverted);
        break;
    case NetworkState::kRest:
        if (locked) {
            Fact(facts, &widgets_mark_glyph_lock, "Locked", inverted);
        } else {
            Fact(facts, nullptr, "Open", inverted);
        }
        break;
    }

    // The signal glyph, alone, at the row's right end whatever the name's length and centred on the row's height: no word under it (DESIGN →
    // Exceptions → The signal glyph without a word). The unlit bars are the strongest glyph in the light dither (on the black row the
    // dark dither: as faint, mirrored), the lit bars over them on the same canvas, black (white on black); the count of lit bars and the
    // faint rest are the reading.
    lv_obj_t *canvas = detail::Bare(row);
    lv_obj_add_flag(canvas, LV_OBJ_FLAG_FLOATING);
    lv_obj_set_size(canvas, kSignalGlyphWidth, kSignalGlyphHeight);
    lv_obj_align(canvas, LV_ALIGN_RIGHT_MID, 0, 0);
    if (signal < 3) {
        lv_obj_t *unlit = Mark(canvas, &widgets_mark_glyph_signal_strong,
                               inverted ? ui::Fill(dt::kColorScalePrimary1) : ui::Fill(dt::kColorBand));
        lv_obj_set_pos(unlit, 0, 0);
    }
    if (const lv_image_dsc_t *lit = SignalLit(signal)) {
        lv_obj_t *bars = Mark(canvas, lit, ui::Level(inverted ? dt::kColorTextOnFill : dt::kColorTextPrimary));
        lv_obj_set_pos(bars, 0, 0);
    }
}
}  // namespace

lv_obj_t *MakeNetworkRow(lv_obj_t *parent, const char *name, bool locked, int signal, NetworkState state)
{
    // The home row's shell: 80 px, `rounded.lg`, its ring outside, the chosen inversion.
    lv_obj_t *row = ui::NewRow(parent, name, nullptr, nullptr, nullptr);
    lv_obj_t *title = lv_obj_get_child(row, 0);
    lv_obj_set_style_text_font(title, ui::Font(ui::Type::kCaption), 0);
    ui::FitLineBox(title, ui::Type::kCaption);
    lv_obj_set_width(title, kWords);
    Fill(row, locked, signal, state);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    detail::JoinGroup(row);
    return row;
}

void SetNetworkRow(lv_obj_t *row, bool locked, int signal, NetworkState state)
{
    Fill(row, locked, signal, state);
}

}  // namespace widgets
