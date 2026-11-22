// Host test of a disabled control's words on LVGL's real renderer, in the glass's own frame — 8-bit grey, as lvgl_display keeps
// it — on the computer with no board: words and a mark painted in the sparse dithers keep the code in every ink pixel, with
// no grey edge between it and its ground, and the one-bit packing then draws exactly the pattern (DESIGN → Components →
// States → Disabled; STICKY_DESIGN_TOKENS §4). A one-bit Bitcount stroke is the font's solid pixels, so a glyph pixel is the
// text colour's code itself; an A1 mark recoloured is the same.
// host-sources: components/ui/fonts/bitcount/bitcount_700_30.c components/ui/fonts/bitcount/bitcount_600_20.c components/widgets/marks/marks.c
// Build and run: bash tests/host/run.sh

#include <cstdint>
#include <cstdio>
#include <set>
#include <string>
#include <vector>

#include "design_tokens.h"
#include "glass_rules.h"
#include "lvgl.h"

extern "C" {
LV_FONT_DECLARE(bitcount_700_30)
LV_FONT_DECLARE(bitcount_600_20)
extern const lv_image_dsc_t widgets_mark_glyph_check;
extern const lv_image_dsc_t widgets_mark_glyph_speaker_off;
}

namespace dt = design_tokens;
using namespace lvgl_display;

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

constexpr int kW = 480;
constexpr int kH = 160;
std::vector<uint8_t> s_frame(kW * kH);

void Flush(lv_display_t *display, const lv_area_t *area, uint8_t *px)
{
    const int w = lv_area_get_width(area);
    for (int y = area->y1; y <= area->y2; ++y) {
        for (int x = area->x1; x <= area->x2; ++x) {
            s_frame[static_cast<size_t>(y) * kW + x] = px[(y - area->y1) * w + (x - area->x1)];
        }
    }
    lv_display_flush_ready(display);
}

lv_color_t Code(uint8_t level)
{
    const uint8_t v = FillCode(level);
    return lv_color_make(v, v, v);
}

void Render(lv_obj_t *screen)
{
    lv_screen_load(screen);
    lv_refr_now(nullptr);  // draws the whole screen now, with no tick to wait for
}

lv_obj_t *Box(lv_obj_t *parent, int x, int y, int w, int h, uint8_t level, int radius)
{
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_remove_style_all(box);
    lv_obj_set_pos(box, x, y);
    lv_obj_set_size(box, w, h);
    lv_obj_set_style_radius(box, radius, 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(box, Code(level), 0);
    return box;
}

lv_obj_t *Words(lv_obj_t *parent, const lv_font_t *font, const char *text, uint8_t level, int x, int y)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, Code(level), 0);
    lv_obj_set_pos(label, x, y);
    return label;
}

lv_obj_t *NewScreen()
{
    lv_obj_t *screen = lv_obj_create(nullptr);
    lv_obj_remove_style_all(screen);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(screen, lv_color_white(), 0);
    return screen;
}

struct Tally {
    std::set<int> values;       // every distinct value in the area
    int code_pixels = 0;        // pixels that hold the sparse code
    int black_code_pixels = 0;  // of them, those the one-bit frame draws black
};

// Tallies the values in an area, and how the one-bit frame draws the pixels holding `code`.
Tally Count(int x0, int y0, int x1, int y1, uint8_t code)
{
    Tally t;
    for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
            const uint8_t v = s_frame[static_cast<size_t>(y) * kW + x];
            t.values.insert(v);
            if (v == code) {
                ++t.code_pixels;
                if (!OneBitWhite(v, x, y)) {
                    ++t.black_code_pixels;
                }
            }
        }
    }
    return t;
}

bool OnlyValues(const Tally &t, std::initializer_list<int> allowed)
{
    for (int v : t.values) {
        bool ok = false;
        for (int a : allowed) {
            ok = ok || v == a;
        }
        if (!ok) {
            return false;
        }
    }
    return true;
}

// Words of `font` in `code` over `ground` (level 3 white, or 2 the face's light dither): the area holds only the ground's
// value and the code — no grey edge — and the one-bit frame draws the pattern's share of the strokes black.
void TestWords(const char *what, const lv_font_t *font, uint8_t code_level, uint8_t ground_level, int black_per)
{
    lv_obj_t *screen = NewScreen();
    if (ground_level != dt::kColorWhite) {
        Box(screen, 24, 20, 400, 100, ground_level, 12);
    }
    Words(screen, font, "Record Later 0123", code_level, 40, 40);
    Render(screen);
    const uint8_t code = FillCode(code_level);
    const int ground = FillCode(ground_level);
    const Tally t = Count(30, 30, 420, 110, code);
    const std::string n = std::string(what) + ": ";
    Check(t.code_pixels > 300, n + "the words draw ink pixels (" + std::to_string(t.code_pixels) + ")");
    Check(OnlyValues(t, {ground, code}), n + "every pixel is the ground or the code, no grey edge (" +
                                             std::to_string(t.values.size()) + " distinct values)");
    // The pattern's share: about one black pixel in black_per of the strokes — a stroke of one font cell, 2 px at 20 px,
    // meets the pattern's 4 px period unevenly, so within half of that share either way.
    const double share = static_cast<double>(t.black_code_pixels) / t.code_pixels;
    Check(share > 0.5 / black_per && share < 1.5 / black_per,
          n + "one pixel in " + std::to_string(black_per) + " of the strokes draws black (" + std::to_string(share) + ")");
    lv_obj_delete(screen);
}

// An A1 mark, recoloured: its ink pixels hold the code exactly — as many as the mark has set bits — and nothing else is added.
void TestMark(const char *what, const lv_image_dsc_t &mark, uint8_t code_level)
{
    lv_obj_t *screen = NewScreen();
    Box(screen, 24, 20, 100, 100, dt::kColorBand, 12);
    lv_obj_t *image = lv_image_create(screen);
    lv_image_set_src(image, &mark);
    lv_obj_set_style_image_recolor(image, Code(code_level), 0);
    lv_obj_set_style_image_recolor_opa(image, LV_OPA_COVER, 0);
    lv_obj_set_pos(image, 50, 50);
    Render(screen);
    int ink = 0;
    const int w = mark.header.w;
    for (uint32_t y = 0; y < mark.header.h; ++y) {
        for (int x = 0; x < w; ++x) {
            ink += (mark.data[y * mark.header.stride + x / 8] >> (7 - x % 8)) & 1;
        }
    }
    const uint8_t code = FillCode(code_level);
    const Tally t = Count(50, 50, 50 + w, 50 + static_cast<int>(mark.header.h), code);
    const std::string n = std::string(what) + ": ";
    Check(t.code_pixels == ink, n + "the code is on exactly the mark's " + std::to_string(ink) + " ink pixels (" +
                                    std::to_string(t.code_pixels) + ")");
    Check(OnlyValues(t, {FillCode(dt::kColorBand), code}), n + "no grey edge round the mark");
    lv_obj_delete(screen);
}

}  // namespace

int main()
{
    lv_init();
    lv_display_t *display = lv_display_create(kW, kH);
    static uint8_t buf[kW * kH];
    lv_display_set_color_format(display, LV_COLOR_FORMAT_L8);
    lv_display_set_buffers(display, buf, nullptr, sizeof buf, LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(display, Flush);

    Check(dt::kColorTextDisabled == kLevelSparseSixteenth && dt::kColorTextDisabledPage == kLevelSparseEighth,
          "text-disabled is the sixteenth dither and text-disabled-page the eighth");
    // Words on a disabled control's face: the sixteenth on the light dither; a row's name on the page: the eighth on white.
    TestWords("label on the face", &bitcount_700_30, dt::kColorTextDisabled, dt::kColorBand, 16);
    TestWords("caption on the face", &bitcount_600_20, dt::kColorTextDisabled, dt::kColorBand, 16);
    TestWords("label on the page", &bitcount_700_30, dt::kColorTextDisabledPage, dt::kColorWhite, 8);
    TestWords("caption on the page", &bitcount_600_20, dt::kColorTextDisabledPage, dt::kColorWhite, 8);
    TestMark("check on the face", widgets_mark_glyph_check, dt::kColorTextDisabled);
    TestMark("speaker off on the face", widgets_mark_glyph_speaker_off, dt::kColorTextDisabled);

    std::printf("disabled words on LVGL: %d checks, %d failed\n", s_checks, s_failures);
    return s_failures == 0 ? 0 : 1;
}
