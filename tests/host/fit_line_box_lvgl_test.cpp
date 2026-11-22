// Host test of ui::FitLineBox (components/ui/line_box.cpp) on LVGL's real label code, on the computer with no board: the
// spare pixels between a font's own height and its role's line box are split above and below (the odd pixel above); a dotted
// label is one line box tall; a wrapped label is n line boxes tall for n lines — for `label` (36), `caption` (24) and
// `caption-loose` (30) in their real Bitcount fonts (STICKY_DESIGN_TOKENS §3 gives a role's line; STICKY_DESIGN_TOKENS §9 is the line-box rule).
// run.sh builds LVGL's sources from managed_components once, into build/host/lvgl, and the sources named below.
// host-sources: components/ui/line_box.cpp components/ui/fonts/bitcount/bitcount_700_30.c components/ui/fonts/bitcount/bitcount_600_20.c
// Build and run: bash tests/host/run.sh

#include <cstdio>
#include <cstdlib>
#include <string>

#include "design_tokens.h"
#include "line_box.h"
#include "lvgl.h"

extern "C" {
LV_FONT_DECLARE(bitcount_700_30)
LV_FONT_DECLARE(bitcount_600_20)
}

namespace dt = design_tokens;

namespace {

int s_failures = 0;
int s_checks = 0;

void Check(bool ok, const std::string &what)
{
    ++s_checks;
    if (!ok) {
        ++s_failures;
        std::printf("FAIL  %s\n", what.c_str());
    }
}

void Flush(lv_display_t *display, const lv_area_t *, uint8_t *)
{
    lv_display_flush_ready(display);
}

constexpr int32_t kWidth = 432;  // the content's width (spacing.content)

struct Case {
    const char *name;
    const lv_font_t *font;
    int32_t line_px;  // from the token header's role
};

lv_obj_t *NewLabel(const Case &c, lv_label_long_mode_t mode, const char *text, int32_t width)
{
    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, c.font, 0);
    // As ui::NewLabel sets a wrapped label: LVGL adds the line box less the font between lines.
    lv_obj_set_style_text_line_space(label, c.line_px - lv_font_get_line_height(c.font), 0);
    lv_label_set_long_mode(label, mode);
    lv_obj_set_width(label, width);
    return label;
}

int32_t HeightOf(lv_obj_t *label)
{
    lv_obj_update_layout(label);
    return lv_obj_get_height(label);
}

void TestPadding(const Case &c)
{
    const int32_t font_px = lv_font_get_line_height(c.font);
    const int32_t spare = c.line_px - font_px;
    lv_obj_t *label = NewLabel(c, LV_LABEL_LONG_MODE_WRAP, "Words", kWidth);
    ui::FitLineBox(label, c.line_px);
    const int32_t top = lv_obj_get_style_pad_top(label, LV_PART_MAIN);
    const int32_t bottom = lv_obj_get_style_pad_bottom(label, LV_PART_MAIN);
    const std::string n = std::string(c.name) + " (" + std::to_string(c.line_px) + " px, font " + std::to_string(font_px) +
                          ", spare " + std::to_string(spare) + "): ";
    Check(spare > 0, n + "the font is shorter than its line box");
    Check(top == (spare + 1) / 2, n + "pad_top is (spare + 1) / 2, the odd pixel above");
    Check(bottom == spare / 2, n + "pad_bottom is spare / 2");
    Check(top + bottom == spare, n + "the two pads sum to the spare");
    Check(top - bottom == (spare % 2), n + "above is the larger by the odd pixel, never by more");
    Check(HeightOf(label) == c.line_px, n + "one line is one line box tall");
    lv_obj_delete(label);
}

// The same arithmetic at both parities, whatever the font: line boxes of the font's height plus 4 and plus 5.
void TestBothParities(const Case &c)
{
    const int32_t font_px = lv_font_get_line_height(c.font);
    for (int32_t spare : {4, 5}) {
        Case synthetic = {c.name, c.font, font_px + spare};
        lv_obj_t *label = NewLabel(synthetic, LV_LABEL_LONG_MODE_WRAP, "Words", kWidth);
        ui::FitLineBox(label, synthetic.line_px);
        const std::string n = std::string(c.name) + " at spare " + std::to_string(spare) + ": ";
        Check(lv_obj_get_style_pad_top(label, LV_PART_MAIN) == (spare + 1) / 2, n + "pad_top");
        Check(lv_obj_get_style_pad_bottom(label, LV_PART_MAIN) == spare / 2, n + "pad_bottom");
        Check(HeightOf(label) == synthetic.line_px, n + "one line is the line box");
        lv_obj_delete(label);
    }
}

void TestNoSpare(const Case &c)
{
    // A line box no taller than the font adds nothing: the label stays the font's height.
    const int32_t font_px = lv_font_get_line_height(c.font);
    lv_obj_t *label = NewLabel(c, LV_LABEL_LONG_MODE_WRAP, "Words", kWidth);
    ui::FitLineBox(label, font_px);
    Check(lv_obj_get_style_pad_top(label, LV_PART_MAIN) == 0 && lv_obj_get_style_pad_bottom(label, LV_PART_MAIN) == 0,
          std::string(c.name) + ": no spare, no padding");
    Check(HeightOf(label) == font_px, std::string(c.name) + ": no spare, the font's own height");
    lv_obj_delete(label);
}

void TestDotted(const Case &c)
{
    // A dotted label is one line box tall, padding included, whether its words fit or run past the width.
    const char *short_text = "Short";
    const char *long_text =
        "A name far too long for one line of the content column, so that LVGL ends it in three full stops instead";
    for (const char *text : {short_text, long_text}) {
        lv_obj_t *label = NewLabel(c, LV_LABEL_LONG_MODE_DOTS, text, kWidth);
        ui::FitLineBox(label, c.line_px);
        Check(HeightOf(label) == c.line_px,
              std::string(c.name) + ": a dotted label (" + (text == short_text ? "short" : "long") + ") is line_px tall");
        lv_obj_delete(label);
    }
}

void TestWrapped(const Case &c)
{
    // n explicit lines are n line boxes tall.
    for (int n = 1; n <= 5; ++n) {
        std::string text;
        for (int i = 0; i < n; ++i) {
            text += i == 0 ? "Line" : "\nLine";
        }
        lv_obj_t *label = NewLabel(c, LV_LABEL_LONG_MODE_WRAP, text.c_str(), kWidth);
        ui::FitLineBox(label, c.line_px);
        Check(HeightOf(label) == n * c.line_px,
              std::string(c.name) + ": " + std::to_string(n) + " lines are " + std::to_string(n) + " x line_px tall");
        lv_obj_delete(label);
    }
    // Words that wrap by themselves, in a label too narrow for them: a whole number of line boxes, more than one.
    lv_obj_t *label = NewLabel(c, LV_LABEL_LONG_MODE_WRAP,
                               "These words are long enough to wrap onto a second line and then on to a third", 200);
    ui::FitLineBox(label, c.line_px);
    const int32_t h = HeightOf(label);
    Check(h % c.line_px == 0 && h / c.line_px >= 3,
          std::string(c.name) + ": wrapped by width, " + std::to_string(h) + " px is a whole number (>= 3) of line boxes");
    lv_obj_delete(label);
}

}  // namespace

int main()
{
    lv_init();
    static uint8_t buf[480 * 40 * 4];
    lv_display_t *display = lv_display_create(480, 800);
    lv_display_set_buffers(display, buf, nullptr, sizeof buf, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, Flush);

    // The roles' fonts and line boxes are the token header's: a change there changes what is tested, and says so.
    Check(dt::kTypographyLabel.size_px == 30 && dt::kTypographyLabel.weight == 700, "label is Bitcount 30 at 700");
    Check(dt::kTypographyCaption.size_px == 20 && dt::kTypographyCaption.weight == 600, "caption is Bitcount 20 at 600");
    Check(dt::kTypographyCaptionLoose.size_px == 20 && dt::kTypographyCaptionLoose.weight == 600,
          "caption-loose is Bitcount 20 at 600");
    const Case cases[] = {
        {"label", &bitcount_700_30, dt::kTypographyLabel.line_px},
        {"caption", &bitcount_600_20, dt::kTypographyCaption.line_px},
        {"caption-loose", &bitcount_600_20, dt::kTypographyCaptionLoose.line_px},
    };
    Check(cases[0].line_px == 36 && cases[1].line_px == 24 && cases[2].line_px == 30, "the line boxes are 36, 24 and 30");

    for (const Case &c : cases) {
        TestPadding(c);
        TestBothParities(c);
        TestNoSpare(c);
        TestDotted(c);
        TestWrapped(c);
    }

    std::printf("FitLineBox on LVGL: %d checks, %d failed\n", s_checks, s_failures);
    return s_failures == 0 ? 0 : 1;
}
