// Host test of the gaps between texts and between components, on LVGL's real layout with the real widgets and fonts, on the
// computer with no board (DESIGN → Layout → Spacing; STICKY_DESIGN_TOKENS §4): a section heading stands `section` 48 over its
// place whatever gap its parent sets, and keeps `lg` when it is the first in its parent; its words start at `edge`, or `md` in for
// the rows whose words stand there; a pair's gap is measured between the two texts' own boxes — the font's height, no leading —
// which `Glue` takes the line boxes' leading out of; the dialog's title stands `pair-xl` over its message and the message `group`
// over its buttons; a status block's title stands `pair-md` over its description.
// run.sh builds LVGL's sources from managed_components once, into build/host/lvgl, and the sources named below, against stand-ins
// for the ESP-IDF headers (tests/host/stubs).
// host-sources: components/ui/ui.cpp components/ui/line_box.cpp components/ui/fonts/bitcount/*.c components/widgets/marks/marks.c components/widgets/src/*.cpp components/widgets/src/internal/*.cpp components/router/router.cpp components/router/overlays.cpp components/router/focus.cpp components/router/touch.cpp components/router/routes.cpp components/router/screen.cpp components/router/status.cpp
// Build and run: bash tests/host/run.sh section_gaps

#include <cstdio>
#include <cstdlib>
#include <string>

#include "design_tokens.h"
#include "lvgl.h"
#include "lvgl_display.h"
#include "ui.h"
#include "widgets/widgets.h"

#if __has_include("internal/glue.h")
#include "internal/glue.h"
#define HAVE_GLUE 1
#else
#define HAVE_GLUE 0  // the code before the pair gap was measured between font boxes: the checks that need it are skipped
#endif

namespace dt = design_tokens;

// What the widgets take from lvgl_display, which is not linked: the focus group, and the few calls the keyboard and the sheet make.
namespace {
lv_group_t *s_group = nullptr;
void (*s_back)() = nullptr;
}  // namespace
namespace lvgl_display {
lv_group_t *Group() { return s_group; }
void ClearGroup()
{
    lv_group_set_editing(s_group, false);
    lv_group_remove_all_objs(s_group);
}
// The router binds a screen's or an overlay's group here: the buttons and LVGL's default group follow it, as on the board.
void SetGroup(lv_group_t *group)
{
    if (group == nullptr) {
        return;
    }
    s_group = group;
    for (lv_indev_t *indev = lv_indev_get_next(nullptr); indev != nullptr; indev = lv_indev_get_next(indev)) {
        lv_indev_set_group(indev, group);  // a keypad's only: LVGL ignores it for a pointer
    }
    lv_group_set_default(group);
}
void SetBackHandler(void (*back)()) { s_back = back; }
// The router's read hooks, kept so a test can run a key read through the focus rules as LVGL's read would.
void (*s_key_hook)(lv_indev_t *, lv_indev_data_t *) = nullptr;
void (*s_touch_hook)(lv_indev_t *, lv_indev_data_t *) = nullptr;
void SetKeyReadHook(void (*hook)(lv_indev_t *, lv_indev_data_t *)) { s_key_hook = hook; }
void SetTouchReadHook(void (*hook)(lv_indev_t *, lv_indev_data_t *)) { s_touch_hook = hook; }
void SetPartialCounting(PartialCounting) {}
void NoteTyping() {}
void RequestFull() {}
void PlayCue(Cue) {}
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

void Flush(lv_display_t *display, const lv_area_t *, uint8_t *)
{
    lv_display_flush_ready(display);
}

uint32_t Ticks()
{
    return 0;
}

// --- Measuring -----------------------------------------------------------------------------------------------------------

lv_obj_t *s_screen = nullptr;

// A page: a screen with a column of `gap` between its children, `spacing.edge` margins, as the kit page is.
lv_obj_t *NewColumn(int32_t gap)
{
    if (s_screen != nullptr) {
        lvgl_display::ClearGroup();
        lv_obj_t *blank = lv_obj_create(nullptr);
        lv_screen_load(blank);
        lv_obj_delete(s_screen);  // LVGL's pool is small: a page goes before the next is built
    }
    s_screen = ui::NewScreen();
    lv_screen_load(s_screen);
    lv_obj_t *column = lv_obj_create(s_screen);
    lv_obj_remove_style_all(column);
    lv_obj_set_size(column, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(column, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_hor(column, dt::kSpacingEdge, 0);
    lv_obj_set_style_pad_row(column, gap, 0);
    lv_obj_set_scrollable(column, false);
    return column;
}

lv_area_t Area(lv_obj_t *obj)
{
    lv_obj_update_layout(obj);
    lv_area_t a;
    lv_obj_get_coords(obj, &a);
    return a;
}

int32_t PadTop(lv_obj_t *obj) { return lv_obj_get_style_pad_top(obj, LV_PART_MAIN); }
int32_t PadBottom(lv_obj_t *obj) { return lv_obj_get_style_pad_bottom(obj, LV_PART_MAIN); }

// The white between two stacked texts' own boxes: the lower text's font box top less the upper one's bottom, the line boxes'
// leading (a label's padding) taken off each.
int32_t FontBoxGap(lv_obj_t *upper, lv_obj_t *lower)
{
    return (Area(lower).y1 + PadTop(lower)) - (Area(upper).y2 + 1 - PadBottom(upper));
}

lv_obj_t *Block(lv_obj_t *parent, int32_t height)
{
    lv_obj_t *block = lv_obj_create(parent);
    lv_obj_remove_style_all(block);
    lv_obj_set_size(block, lv_pct(100), height);
    return block;
}

// --- Tests ---------------------------------------------------------------------------------------------------------------

// A heading that follows other content stands `section`, 48, over its place — its own padding and the parent's gap together,
// whatever that gap is; the first in its parent has the page's padding above it and keeps `lg`.
void TestSectionHeading()
{
    std::printf("the section heading's gap over it\n");
    for (int32_t gap : {0, 16, 24}) {
        lv_obj_t *column = NewColumn(gap);
        lv_obj_t *first = widgets::MakeSectionHeading(column, "First");
        lv_obj_t *above = Block(column, 40);
        lv_obj_t *second = widgets::MakeSectionHeading(column, "Second");
        const std::string at = " with the parent's gap " + std::to_string(gap);
        Check(PadTop(first) == dt::kSpacingLg, "the first heading keeps lg over it" + at);
        Check(Area(second).y1 + PadTop(second) - (Area(above).y2 + 1) == dt::kSpacingSection,
              "a heading after other content stands section, 48, from it" + at);
    }
}

#if HAVE_GLUE
// Where a heading's words start: at the edge, over boxes and the rows whose words stand there, or `md` in, over the rows whose
// words stand 16 in.
void TestSectionHeadingX()
{
    std::printf("the section heading's x\n");
    lv_obj_t *column = NewColumn(dt::kSpacingMd);
    lv_obj_t *edge = widgets::MakeSectionHeading(column, "Boxes");
    lv_obj_t *rows = widgets::MakeSectionHeading(column, "Rows", widgets::HeadingX::kRowWords);
    Check(Area(edge).x1 + lv_obj_get_style_pad_left(edge, LV_PART_MAIN) == dt::kSpacingEdge,
          "a heading over boxes starts its words at spacing.edge, 24");
    Check(Area(rows).x1 + lv_obj_get_style_pad_left(rows, LV_PART_MAIN) == dt::kSpacingEdge + dt::kSpacingMd,
          "a heading over rows whose words stand in starts them 16 in, at 40");
}

// Glue takes the leading a line box adds out of the pair: the column's gap is then the gap between the two texts' own boxes.
void TestGlue()
{
    std::printf("the pair gap between font boxes\n");
    lv_obj_t *column = NewColumn(dt::kSpacingPairMd);
    lv_obj_t *upper = ui::NewLabel(column, ui::Type::kLabel, "Title");
    ui::FitLineBox(upper, ui::Type::kLabel);
    lv_obj_t *lower = ui::NewLabel(column, ui::Type::kBody, "Under it");
    ui::FitLineBox(lower, ui::Type::kBody);
    const int32_t loose = FontBoxGap(upper, lower);
    Check(loose > dt::kSpacingPairMd, "without Glue the line boxes' leading widens the gap");
    widgets::detail::Glue(upper, lower);
    Check(FontBoxGap(upper, lower) == dt::kSpacingPairMd, "with Glue the gap between the font boxes is the column's, 16");
}
#endif

// The dialog's title stands `pair-xl`, 32, over its message between their font boxes; the message stands `group`, 24, over the
// buttons, box to box.
void TestDialog()
{
    std::printf("the dialog's gaps\n");
    lv_obj_t *column = NewColumn(dt::kSpacingMd);
    const widgets::Dialog dialog =
        widgets::MakeDialog(column, "Office", "Wrong password. Check it and try again.", "Retry", "Cancel");
    lv_obj_t *title = lv_obj_get_child(dialog.box, 0);
    lv_obj_t *message = lv_obj_get_child(dialog.box, 1);
    lv_obj_t *buttons = lv_obj_get_child(dialog.box, 2);
    Check(FontBoxGap(title, message) == dt::kSpacingPairXl, "the title stands pair-xl, 32, over its message (font boxes)");
    Check(Area(buttons).y1 - (Area(message).y2 + 1) == dt::kSpacingGroup, "the message stands group, 24, over the buttons (box to box)");
}

// A status block's title stands `pair-md`, 16, over its description between their font boxes.
void TestStatusBlock()
{
    std::printf("the status block's pair\n");
    lv_obj_t *column = NewColumn(dt::kSpacingMd);
    lv_obj_t *block = widgets::MakeStatusBlock(column, "Allowed", "Your answer reached the agent.", widgets::StatusMark::kCheck);
    lv_obj_t *words = lv_obj_get_child(block, 1);
    lv_obj_t *title = lv_obj_get_child(words, 0);
    lv_obj_t *description = lv_obj_get_child(words, 1);
    Check(FontBoxGap(title, description) == dt::kSpacingPairMd, "the title stands pair-md, 16, over its description (font boxes)");
}

}  // namespace

int main()
{
    lv_init();
    lv_tick_set_cb(Ticks);
    lv_display_t *display = lv_display_create(480, 800);
    static uint8_t buffer[480 * 800];
    lv_display_set_color_format(display, LV_COLOR_FORMAT_L8);
    lv_display_set_buffers(display, buffer, nullptr, sizeof buffer, LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(display, Flush);
    lv_display_set_theme(display, nullptr);  // as the firmware: no LVGL theme is compiled in
    s_group = lv_group_create();
    lv_group_set_default(s_group);
    if (ui::Start() != ESP_OK) {
        std::printf("ui::Start failed\n");
        return 1;
    }
    TestSectionHeading();
#if HAVE_GLUE
    TestSectionHeadingX();
    TestGlue();
#endif
    TestDialog();
    TestStatusBlock();
    std::printf("section gaps on LVGL: %d checks, %d failed\n", s_checks, s_failures);
    return s_failures == 0 ? 0 : 1;
}
