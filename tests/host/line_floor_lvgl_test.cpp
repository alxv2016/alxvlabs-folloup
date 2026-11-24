// Host test of the type scale's line heights against the compiled fonts, on the computer with no board: every primitive's
// line is on the 4 px grid and never under its font's own height (the line_height lv_font_conv wrote), so ui.cpp's
// LineSpace never clamps a line box to the font's own height in silence (STICKY_DESIGN_TOKENS §3).
// host-sources: components/ui/fonts/bitcount/bitcount_400_30.c components/ui/fonts/bitcount/bitcount_600_100.c components/ui/fonts/bitcount/bitcount_600_120.c components/ui/fonts/bitcount/bitcount_600_160.c components/ui/fonts/bitcount/bitcount_600_20.c components/ui/fonts/bitcount/bitcount_600_30.c components/ui/fonts/bitcount/bitcount_600_40.c components/ui/fonts/bitcount/bitcount_600_50.c components/ui/fonts/bitcount/bitcount_600_60.c components/ui/fonts/bitcount/bitcount_600_70.c components/ui/fonts/bitcount/bitcount_600_80.c components/ui/fonts/bitcount/bitcount_700_100.c components/ui/fonts/bitcount/bitcount_700_120.c components/ui/fonts/bitcount/bitcount_700_160.c components/ui/fonts/bitcount/bitcount_700_20.c components/ui/fonts/bitcount/bitcount_700_30.c components/ui/fonts/bitcount/bitcount_700_40.c components/ui/fonts/bitcount/bitcount_700_50.c components/ui/fonts/bitcount/bitcount_700_60.c components/ui/fonts/bitcount/bitcount_700_70.c components/ui/fonts/bitcount/bitcount_700_80.c
// Build and run: bash tests/host/run.sh line_floor

#include <cstdio>
#include <string>

#include "design_tokens.h"
#include "lvgl.h"

extern "C" {
LV_FONT_DECLARE(bitcount_600_20)
LV_FONT_DECLARE(bitcount_700_20)
LV_FONT_DECLARE(bitcount_400_30)
LV_FONT_DECLARE(bitcount_600_30)
LV_FONT_DECLARE(bitcount_700_30)
LV_FONT_DECLARE(bitcount_600_40)
LV_FONT_DECLARE(bitcount_700_40)
LV_FONT_DECLARE(bitcount_600_50)
LV_FONT_DECLARE(bitcount_700_50)
LV_FONT_DECLARE(bitcount_600_60)
LV_FONT_DECLARE(bitcount_700_60)
LV_FONT_DECLARE(bitcount_600_70)
LV_FONT_DECLARE(bitcount_700_70)
LV_FONT_DECLARE(bitcount_600_80)
LV_FONT_DECLARE(bitcount_700_80)
LV_FONT_DECLARE(bitcount_600_100)
LV_FONT_DECLARE(bitcount_700_100)
LV_FONT_DECLARE(bitcount_600_120)
LV_FONT_DECLARE(bitcount_700_120)
LV_FONT_DECLARE(bitcount_600_160)
LV_FONT_DECLARE(bitcount_700_160)
}

namespace dt = design_tokens;

namespace {

struct Primitive {
    const char *name;
    dt::TypeRole role;
    const lv_font_t *font;
};

const Primitive kScale[] = {
    {"scale-20-semibold", dt::kTypographyScale20Semibold, &bitcount_600_20},
    {"scale-20-bold", dt::kTypographyScale20Bold, &bitcount_700_20},
    {"scale-30-regular", dt::kTypographyScale30Regular, &bitcount_400_30},
    {"scale-30-semibold", dt::kTypographyScale30Semibold, &bitcount_600_30},
    {"scale-30-bold", dt::kTypographyScale30Bold, &bitcount_700_30},
    {"scale-40-semibold", dt::kTypographyScale40Semibold, &bitcount_600_40},
    {"scale-40-bold", dt::kTypographyScale40Bold, &bitcount_700_40},
    {"scale-50-semibold", dt::kTypographyScale50Semibold, &bitcount_600_50},
    {"scale-50-bold", dt::kTypographyScale50Bold, &bitcount_700_50},
    {"scale-60-semibold", dt::kTypographyScale60Semibold, &bitcount_600_60},
    {"scale-60-bold", dt::kTypographyScale60Bold, &bitcount_700_60},
    {"scale-70-semibold", dt::kTypographyScale70Semibold, &bitcount_600_70},
    {"scale-70-bold", dt::kTypographyScale70Bold, &bitcount_700_70},
    {"scale-80-semibold", dt::kTypographyScale80Semibold, &bitcount_600_80},
    {"scale-80-bold", dt::kTypographyScale80Bold, &bitcount_700_80},
    {"scale-100-semibold", dt::kTypographyScale100Semibold, &bitcount_600_100},
    {"scale-100-bold", dt::kTypographyScale100Bold, &bitcount_700_100},
    {"scale-120-semibold", dt::kTypographyScale120Semibold, &bitcount_600_120},
    {"scale-120-bold", dt::kTypographyScale120Bold, &bitcount_700_120},
    {"scale-160-semibold", dt::kTypographyScale160Semibold, &bitcount_600_160},
    {"scale-160-bold", dt::kTypographyScale160Bold, &bitcount_700_160},
};

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

}  // namespace

int main()
{
    int32_t last_ratio_milli = 1000000;
    int32_t last_size = 0;
    for (const Primitive &p : kScale) {
        const int32_t font_px = p.font->line_height;
        const std::string at = std::string(p.name) + " line " + std::to_string(p.role.line_px) + ", font " +
                               std::to_string(font_px);
        Check(p.role.line_px >= font_px, at + ": the line is never under the font's own height");
        Check(p.role.line_px % 4 == 0, at + ": the line is on the 4 px grid");
        // Text sizes: the line over the size never grows as the size grows (TYPOGRAPHY §6: large text gets less).
        // 100 px and up hold figures, one line on the height of their digits, so no ratio holds them.
        if (p.role.size_px <= 80 && p.role.size_px != last_size) {
            const int32_t ratio_milli = p.role.line_px * 1000 / p.role.size_px;
            Check(ratio_milli <= last_ratio_milli, at + ": the line ratio does not grow with the size");
            last_ratio_milli = ratio_milli;
            last_size = p.role.size_px;
        }
    }
    std::printf("line_floor: %d checks, %d failed\n", s_checks, s_failures);
    return s_failures == 0 ? 0 : 1;
}
