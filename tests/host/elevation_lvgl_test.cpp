// Host test of the elevation scale's offsets (ui::AddOffset in components/ui/ui.cpp) on LVGL's real renderer, on the computer with
// no board: each raised level draws its own dither in its band beside the box, a 2 px white keyline rings the band's outer
// edge — shown here over a black ground, where white is visible — and the box itself is untouched (STICKY_DESIGN_TOKENS §2.1;
// DESIGN → Elevation & Depth). The frame is read the way the glass reads it: each L8 pixel through `OneBitWhite` at its column
// and row, one bit a pixel.
// run.sh builds LVGL's sources from managed_components once, into build/host/lvgl, and the sources named below.
// host-sources: components/ui/ui.cpp components/ui/line_box.cpp components/ui/fonts/bitcount/*.c
// Build and run: bash tests/host/run.sh

#include <cstdio>
#include <string>

#include "design_tokens.h"
#include "lvgl.h"
#include "lvgl_display.h"
#include "ui.h"

namespace dt = design_tokens;

namespace lvgl_display {
lv_group_t *Group() { return nullptr; }  // ui.cpp adds rows and buttons to it; no row or button is built here
}  // namespace lvgl_display

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

constexpr int kW = 480, kH = 800;
uint8_t s_buffer[kW * kH];

void Flush(lv_display_t *display, const lv_area_t *, uint8_t *)
{
    lv_display_flush_ready(display);
}

uint32_t Ticks()
{
    return 0;
}

// The glass's pixel: 1 white, 0 black.
int Pixel(int x, int y)
{
    return lvgl_display::OneBitWhite(s_buffer[y * kW + x], x, y) ? 1 : 0;
}

// The box the offsets stand beside: a white box with a black outline and `radius`, on a black ground, at (x, y).
constexpr int kBoxX = 100, kBoxY = 300, kBoxW = 240, kBoxH = 160;

void Draw(ui::Elevation level, ui::OffsetSide side, int32_t radius)
{
    lv_obj_t *screen = lv_obj_create(nullptr);
    lv_obj_remove_style_all(screen);
    lv_obj_set_scrollable(screen, false);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);  // white is visible against it: the keyline
    lv_obj_t *box = lv_obj_create(screen);
    lv_obj_remove_style_all(box);
    lv_obj_set_pos(box, kBoxX, kBoxY);
    lv_obj_set_size(box, kBoxW, kBoxH);
    lv_obj_set_style_radius(box, radius, 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(box, lv_color_white(), 0);
    ui::AddOffset(box, level, side);
    lv_screen_load(screen);
    lv_obj_update_layout(screen);
    lv_obj_invalidate(screen);
    lv_refr_now(nullptr);
    lv_screen_load(lv_obj_create(nullptr));
    lv_obj_delete(screen);
}

// The share of white pixels in a rectangle, in sixteenths — every pattern's period divides 4 x 4, so a whole number of periods is exact.
int WhiteSixteenths(int x0, int y0, int w, int h)
{
    int white = 0;
    for (int y = y0; y < y0 + h; ++y) {
        for (int x = x0; x < x0 + w; ++x) {
            white += Pixel(x, y);
        }
    }
    return white * 16 / (w * h);
}

constexpr int kOffset = dt::kSpacingOverlayOffset;  // 6
constexpr int kKeyline = dt::kSpacingStrokeHairline;  // 2

// What a level's band looks like to the right of the box (a floating box: down and right), middle of the box's height, the corner
// stretches left out.
struct Level {
    ui::Elevation level;
    const char *name;
    int white_sixteenths;  // the pattern's share of white, in sixteenths
};
constexpr Level kLevelsDrawn[] = {
    {ui::Elevation::kRaised, "level 1, raised (the eighth)", 14},      // one black pixel in eight: seven in eight white
    {ui::Elevation::kAnchored, "level 2, anchored (the light dither)", 12},  // three in four white
    {ui::Elevation::kFloating, "level 3, floating (the dark dither)", 4},    // one in four white
};

void TestFloatingOffsets()
{
    std::printf("down and to the right\n");
    for (const Level &l : kLevelsDrawn) {
        Draw(l.level, ui::OffsetSide::kBottomRight, dt::kRoundedMd);
        const int x2 = kBoxX + kBoxW;  // first column right of the box
        const int mid = kBoxY + 40;
        // The band: 6 px wide beside the box, a multiple of nothing — sample the 4 x 4 periods inside it (columns x2 to x2 + 3 are
        // the first whole period and rows run in whole periods).
        const int band = WhiteSixteenths(x2, mid, 4, 80);
        std::printf("  %-40s the band is %2d/16 white (the pattern: %d/16)\n", l.name, band, l.white_sixteenths);
        Check(band == l.white_sixteenths, std::string(l.name) + ": the band right of the box is its dither");
        // The keyline: the 2 px past the band's outer edge are white, and the ground beyond them is black.
        const int lx = x2 + kOffset;
        Check(WhiteSixteenths(lx, mid, kKeyline, 80) == 16, std::string(l.name) + ": a 2 px white keyline rings the band's outer edge");
        Check(WhiteSixteenths(lx + kKeyline, mid, 4, 80) == 0, std::string(l.name) + ": black ground beyond the keyline");
        // Below the box, the same band and keyline (a stretch clear of the corners).
        const int y2 = kBoxY + kBoxH;
        Check(WhiteSixteenths(kBoxX + 40, y2, 80, 4) == l.white_sixteenths && WhiteSixteenths(kBoxX + 40, y2 + kOffset, 80, kKeyline) == 16,
              std::string(l.name) + ": the bottom band and its keyline match");
        // The box is untouched, and nothing stands up and to the left of it.
        Check(WhiteSixteenths(kBoxX + 20, kBoxY + 20, 100, 60) == 16, std::string(l.name) + ": the box is white inside");
        Check(WhiteSixteenths(kBoxX - 8, kBoxY + 20, 6, 80) == 0 && WhiteSixteenths(kBoxX + 20, kBoxY - 8, 80, 6) == 0,
              std::string(l.name) + ": no offset on the box's top and left");
    }
}

void TestSheetOffset()
{
    std::printf("up, from a sheet's top edge\n");
    for (const Level &l : kLevelsDrawn) {
        Draw(l.level, ui::OffsetSide::kTop, dt::kRounded2xl);
        const int top = kBoxY;
        const int mid = kBoxX + 100;  // clear of the 32 px corners
        const int band = WhiteSixteenths(mid, top - kOffset, 80, 4);  // rows top - 6 to top - 3: a whole period
        Check(WhiteSixteenths(mid, top - kOffset + 2, 80, 4) >= 0, "the band can be read");
        Check(band >= l.white_sixteenths - 4 && band <= l.white_sixteenths + 4, std::string(l.name) + ": the band above the sheet is its dither");
        Check(WhiteSixteenths(mid, top - kOffset - kKeyline, 80, kKeyline) == 16, std::string(l.name) + ": a 2 px white keyline above the band");
        Check(WhiteSixteenths(mid, top - kOffset - kKeyline - 4, 80, 4) == 0, std::string(l.name) + ": black ground above the keyline");
        Check(WhiteSixteenths(mid, top - kOffset, 80, kOffset) != 16 && WhiteSixteenths(mid, top + 20, 80, 40) == 16,
              std::string(l.name) + ": the band is a dither and the sheet below it is white");
        std::printf("  %-40s the band is %2d/16 white\n", l.name, band);
    }
}

void TestFlatAndDistinct()
{
    std::printf("the scale\n");
    Draw(ui::Elevation::kFlat, ui::OffsetSide::kBottomRight, dt::kRoundedMd);
    Check(WhiteSixteenths(kBoxX + kBoxW, kBoxY + 40, 12, 80) == 0 && WhiteSixteenths(kBoxX + 40, kBoxY + kBoxH, 80, 12) == 0,
          "a flat surface draws no offset and no keyline");
    Check(dt::kColorShadow1 != dt::kColorShadow2 && dt::kColorShadow2 != dt::kColorShadow3 && dt::kColorShadow1 != dt::kColorShadow3,
          "the three levels' roles are three different dithers");
    Check(lvgl_display::FillCode(dt::kColorShadow1) != lvgl_display::FillCode(dt::kColorShadow2) &&
              lvgl_display::FillCode(dt::kColorShadow2) != lvgl_display::FillCode(dt::kColorShadow3),
          "and three different codes on the glass");
    Check(dt::kColorKeyline == dt::kColorWhite && dt::kElevation3.keyline_px == dt::kSpacingStrokeHairline, "the keyline is white and hairline-wide");
    Check(dt::kElevation1.offset_px == dt::kElevation2.offset_px && dt::kElevation2.offset_px == dt::kElevation3.offset_px &&
              dt::kElevation3.offset_px == dt::kSpacingOverlayOffset,
          "one distance, spacing.overlay-offset, for every level");
    Check(dt::kElevation1.outline_px == dt::kSpacingStrokeStrong && dt::kElevation2.outline_px == dt::kSpacingStrokeStrong &&
              dt::kElevation3.outline_px == dt::kSpacingStrokeStrong && dt::kElevation0.outline_px == dt::kSpacingStrokeDefault,
          "the strong outline on every raised level, the default one where flat");
    Check(dt::kElevation0.shadow == dt::kNoShadow, "level 0 has no offset");
}

}  // namespace

int main()
{
    lv_init();
    lv_tick_set_cb(Ticks);
    lv_display_t *display = lv_display_create(kW, kH);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_L8);
    lv_display_set_buffers(display, s_buffer, nullptr, sizeof s_buffer, LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(display, Flush);
    lv_display_set_theme(display, nullptr);
    TestFloatingOffsets();
    TestSheetOffset();
    TestFlatAndDistinct();
    std::printf("elevation on LVGL: %d checks, %d failed\n", s_checks, s_failures);
    return s_failures == 0 ? 0 : 1;
}
