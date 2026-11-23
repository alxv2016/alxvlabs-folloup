// Host test of the page swipe on LVGL's real scroll events (components/widgets/src/internal/swipe.cpp), on the computer
// with no board: a scripted finger drags over a paged band, and LVGL's own input and scroll code answers — the begin, the
// scrolls, the lift. swipe_test.cpp checks the decision alone; this checks what it was blind to: that the put-back and the
// turn, which LVGL answers with the same scroll events a finger's makes, do not call the swipe again without end; that a
// 64 px drag turns exactly one page and the page stays turned; that a drag from a disabled child still turns it, once. On the
// horizontal axis, the horizontal pager is built for real: UpdateHorizontalPager's pages, a horizontal drag on its strip turning
// one page, and a vertical drag on it chaining to the page behind.
// run.sh builds LVGL's sources from managed_components (a firmware build fetches them) into build/host/lvgl, once, and the
// sources named below, against stand-ins for the ESP-IDF headers (tests/host/stubs).
// host-sources: components/ui/ui.cpp components/ui/line_box.cpp components/ui/fonts/bitcount/*.c components/widgets/marks/marks.c components/widgets/src/*.cpp components/widgets/src/internal/*.cpp components/router/router.cpp components/router/overlays.cpp components/router/focus.cpp components/router/touch.cpp components/router/routes.cpp components/router/screen.cpp components/router/status.cpp
// Build and run: bash tests/host/run.sh

#include <cstdio>
#include <cstdlib>

#include <string>
#include <vector>

#include "design_tokens.h"
#include "internal/swipe.h"
#include "lvgl.h"
#include "lvgl_display.h"
#include "ui.h"
#include "widgets/chip.h"
#include "widgets/horizontal_pager.h"

using namespace widgets::detail;
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
// The at-end cue a widget asks for, counted: a swipe past the last or the first page asks it, a turn never.
int s_at_ends = 0;
void PlayCue(Cue cue) { s_at_ends += cue == Cue::kAtEnd; }
}  // namespace lvgl_display

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

// --- LVGL, driven by hand: a display that draws nowhere and a pointer the test moves -------------------------------------

constexpr int32_t kPageH = 300;      // a page: three rows
constexpr int32_t kRowH = 100;
constexpr int kRows = 9;             // three pages
constexpr uint32_t kReadMs = 33;     // this test's input read period (LV_DEF_REFR_PERIOD); the firmware reads every 10 ms

int s_x, s_y;
bool s_down;

void Flush(lv_display_t *display, const lv_area_t *, uint8_t *)
{
    lv_display_flush_ready(display);
}

void ReadPointer(lv_indev_t *, lv_indev_data_t *data)
{
    data->point.x = s_x;
    data->point.y = s_y;
    data->state = s_down ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

void Tick()
{
    lv_tick_inc(kReadMs);
    lv_timer_handler();
}

// --- The band under test, and a spy on its scroll events --------------------------------------------------------------

int s_depth;           // scroll events being handled at once: a swipe that answers its own scroll deepens it without end
int s_deepest;
int s_scroll_events;   // every LV_EVENT_SCROLL the band got
int s_page;            // the page the band's owner keeps
int s_turns;           // how many times the swipe asked for a turn

void SpyEnter(lv_event_t *e)
{
    const lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_SCROLL_BEGIN && code != LV_EVENT_SCROLL && code != LV_EVENT_SCROLL_END) {
        return;
    }
    if (code == LV_EVENT_SCROLL) {
        ++s_scroll_events;
    }
    ++s_depth;
    if (s_depth > s_deepest) {
        s_deepest = s_depth;
    }
    if (s_depth > 8) {  // the swipe is calling itself: stop here with the answer rather than overflow the stack
        std::printf("FAIL  the swipe answers its own scroll without end (%d deep)\n", s_depth);
        std::exit(2);
    }
}

void SpyLeave(lv_event_t *e)
{
    const lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_SCROLL_BEGIN || code == LV_EVENT_SCROLL || code == LV_EVENT_SCROLL_END) {
        --s_depth;
    }
}

// What a list view's turn does: the page counted, the band scrolled to its first row, at once (list_view.cpp ShowPage).
void Turn(lv_obj_t *band, int32_t dir)
{
    ++s_turns;
    const int page = s_page + static_cast<int>(dir);
    if (page < 0 || page > kRows * kRowH / kPageH - 1) {
        return;
    }
    s_page = page;
    lv_obj_scroll_to_y(band, page * kPageH, LV_ANIM_OFF);
}

lv_obj_t *NewBand(int disabled_row = -1)
{
    lv_obj_t *band = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(band);
    lv_obj_set_pos(band, 40, 100);
    lv_obj_set_size(band, 400, kPageH);
    lv_obj_set_flex_flow(band, LV_FLEX_FLOW_COLUMN);
    for (int i = 0; i < kRows; ++i) {
        lv_obj_t *row = lv_obj_create(band);
        lv_obj_remove_style_all(row);
        lv_obj_set_size(row, 400, kRowH);
        lv_obj_set_scrollable(row, false);
        if (i == disabled_row) {
            lv_obj_add_state(row, LV_STATE_DISABLED);
        }
    }
    lv_obj_add_event_cb(band, SpyEnter, LV_EVENT_ALL, nullptr);  // first: it sees an event before the swipe does,
    ClaimSwipe(band, Turn);
    lv_obj_add_event_cb(band, SpyLeave, LV_EVENT_ALL, nullptr);  // and last: it sees it handled
    lv_obj_update_layout(band);
    s_depth = s_deepest = s_scroll_events = s_page = s_turns = 0;
    return band;
}

// The finger presses at `from_y`, moves to `to_y` one read at a time in steps of `each` px, rests `rest_reads` reads there,
// and lifts. LVGL then runs its own end of the drag for a few reads.
void Drag(int from_y, int to_y, int each, int rest_reads = 0)
{
    s_x = 200;
    s_y = from_y;
    s_down = true;
    Tick();
    const int step = to_y < from_y ? -each : each;
    for (int y = from_y + step; step < 0 ? y >= to_y : y <= to_y; y += step) {
        s_y = y;
        Tick();
    }
    if (s_y != to_y) {
        s_y = to_y;
        Tick();
    }
    for (int i = 0; i < rest_reads; ++i) {
        Tick();
    }
    s_down = false;
    for (int i = 0; i < 6; ++i) {
        Tick();
    }
}

// The same across the glass: the finger presses at `from_x` on the row `y`, moves to `to_x`, and lifts.
void DragX(int from_x, int to_x, int y, int each, int rest_reads = 0)
{
    s_y = y;
    s_x = from_x;
    s_down = true;
    Tick();
    const int step = to_x < from_x ? -each : each;
    for (int x = from_x + step; step < 0 ? x >= to_x : x <= to_x; x += step) {
        s_x = x;
        Tick();
    }
    if (s_x != to_x) {
        s_x = to_x;
        Tick();
    }
    for (int i = 0; i < rest_reads; ++i) {
        Tick();
    }
    s_down = false;
    for (int i = 0; i < 6; ++i) {
        Tick();
    }
}

// The drag LVGL hands the swipe is `kScrollLimit` px short of the finger's travel: LVGL starts a scroll only once the
// finger has gone past its scroll limit (10 px by default) and counts what follows. Found by this test's own sweep at one
// pixel a read: a finger that travelled 73 px turns a page, one that travelled 72 does not.
constexpr int kScrollLimit = 9;

void TestNoRecursion()
{
    lv_obj_t *band = NewBand();
    Drag(350, 150, 8);  // a long drag up over the band
    Check(s_deepest <= 2, "a drag's put-back is not answered as a new drag: scroll events are never more than two deep");
    Check(s_scroll_events > 0, "the drag's scrolls reached the band");
    Check(s_scroll_events <= 2 * 30, "each move of the finger makes the swipe one scroll of its own, no more");
    lv_obj_delete(band);
}

void TestTurnsOnePage()
{
    lv_obj_t *band = NewBand();
    // The finger travels 64 px past the scroll limit: 64 px of drag. Slowly, so no flick helps it.
    Drag(350, 350 - (kScrollLimit + dt::kTouchSwipeDistance), 1);
    Check(s_turns == 1, "a 64 px drag turns once");
    Check(s_page == 1, "a 64 px drag up turns forward exactly one page");
    Check(lv_obj_get_scroll_y(band) == kPageH, "the band is on the new page after the lift");
    // Nothing puts it back later: let LVGL's throw and the timers run on.
    for (int i = 0; i < 20; ++i) {
        Tick();
    }
    Check(lv_obj_get_scroll_y(band) == kPageH, "the page stays turned");
    Check(s_turns == 1, "and it is not turned again");
    lv_obj_delete(band);
}

void TestShortDragPlaces()
{
    lv_obj_t *band = NewBand();
    Drag(350, 350 - (kScrollLimit + dt::kTouchSwipeDistance - 1), 1, 20);  // 63 px, and the finger rests before it lifts
    Check(s_turns == 0, "63 px that rested turns nothing");
    Check(lv_obj_get_scroll_y(band) == 0, "and the band is where the page keeps it");
    lv_obj_delete(band);
}

void TestSecondSwipeAndBack()
{
    lv_obj_t *band = NewBand();
    Drag(350, 150, 4);
    Check(s_page == 1 && lv_obj_get_scroll_y(band) == kPageH, "a drag over the band's rows turns to page 1");
    Drag(350, 150, 4);
    Check(s_page == 2 && lv_obj_get_scroll_y(band) == 2 * kPageH, "the next swipe starts from the new page and turns to page 2");
    Drag(150, 350, 4);
    Check(s_page == 1 && lv_obj_get_scroll_y(band) == kPageH, "a swipe down turns back one page");
    Check(s_turns == 3, "three swipes, three turns");
    lv_obj_delete(band);
}

void TestFlick()
{
    lv_obj_t *band = NewBand();
    Drag(350, 350 - (kScrollLimit + 40), 20);  // 40 px of drag in two reads: short of a swipe, a flick
    Check(s_page == 1 && s_turns == 1, "a short, quick drag that is still moving as it lifts is a flick: one page");
    lv_obj_delete(band);
}

void TestDisabledChild()
{
    // A drag that begins on a disabled child: LVGL sends the band no throw for it. The turn still comes, once.
    lv_obj_t *band = NewBand(/*disabled_row=*/3);  // rows 0-2 are page 0, row 3 the first of page 1
    lv_obj_scroll_to_y(band, kPageH, LV_ANIM_OFF);
    s_page = 1;
    Drag(150, 150 - (kScrollLimit + dt::kTouchSwipeDistance), 4);  // from the glass's row 3: the band's top
    Check(s_turns == 1, "a drag that begins on a disabled child turns once");
    Check(s_page == 2 && lv_obj_get_scroll_y(band) == 2 * kPageH, "and to the next page, which stays");
    Check(s_deepest <= 2, "with no recursion");
    lv_obj_delete(band);

    band = NewBand(3);
    lv_obj_scroll_to_y(band, kPageH, LV_ANIM_OFF);
    s_page = 1;
    Drag(150, 150 - (kScrollLimit + 20), 4, 20);  // short, and rested: it places
    Check(s_turns == 0 && lv_obj_get_scroll_y(band) == kPageH, "a short drag on a disabled child places and turns nothing");
    lv_obj_delete(band);
}

void TestKeysAreLeftAlone()
{
    lv_obj_t *band = NewBand();
    // A scroll nobody's finger made — a key's, a focus's — is not a drag: nothing is put back and nothing turns.
    lv_obj_scroll_to_y(band, kRowH, LV_ANIM_OFF);
    Tick();
    Check(lv_obj_get_scroll_y(band) == kRowH, "a scroll with no finger on the band is left where it went");
    Check(s_turns == 0, "and turns no page");
    lv_obj_delete(band);
}

// --- The horizontal pager, built for real, on a page that scrolls ----------------------------------------------------------

constexpr int32_t kView = dt::kSpacingContent + 2 * dt::kSpacingSm;  // one page's start to the next's: the pager's own rule

struct PagerPage {
    lv_obj_t *page;  // a page taller than the glass, so a vertical drag has somewhere to go
    widgets::HorizontalPager pager;
};

PagerPage NewPagerPage(int chips)
{
    PagerPage p{};
    p.page = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(p.page);
    lv_obj_set_pos(p.page, 0, 0);
    lv_obj_set_size(p.page, 480, 800);
    lv_obj_set_flex_flow(p.page, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(p.page, LV_DIR_VER);
    lv_obj_set_style_pad_hor(p.page, dt::kSpacingEdge, 0);
    lv_obj_set_style_pad_top(p.page, 100, 0);
    p.pager = widgets::MakeHorizontalPager(p.page);
    for (int i = 0; i < chips; ++i) {
        char word[16];
        std::snprintf(word, sizeof word, "Chip %d", i + 1);
        widgets::MakeChip(p.pager.root, word);
    }
    widgets::UpdateHorizontalPager(p.pager);
    lv_obj_t *spacer = lv_obj_create(p.page);  // the page's own height, past the glass
    lv_obj_remove_style_all(spacer);
    lv_obj_set_size(spacer, 10, 1200);
    lv_obj_update_layout(p.page);
    return p;
}

// The pages the rule gives from the chips' own widths: each page the whole items that fit the content column, `spacing.md` apart.
int ExpectedPages(lv_obj_t *band)
{
    int pages = 1;
    int32_t used = 0;
    for (uint32_t i = 0; i < lv_obj_get_child_count(band); ++i) {
        const int32_t width = lv_obj_get_width(lv_obj_get_child(band, static_cast<int32_t>(i)));
        const int32_t with = used == 0 ? width : used + dt::kSpacingMd + width;
        if (used != 0 && with > dt::kSpacingContent) {
            ++pages;
            used = width;
        } else {
            used = with;
        }
    }
    return pages;
}

void StripPoint(const PagerPage &p, int *x_from, int *x_to, int *y)
{
    lv_area_t a;
    lv_obj_get_coords(p.pager.root, &a);
    *y = (a.y1 + a.y2) / 2;
    *x_from = a.x2 - 60;
    *x_to = a.x1 + 60;
}

void TestPagerPages()
{
    PagerPage few = NewPagerPage(2);
    Check(widgets::HorizontalPagerPages(few.pager) == 1, "two chips fit one page");
    Check(lv_obj_has_flag(few.pager.words_row, LV_OBJ_FLAG_HIDDEN), "and the page words stay hidden");
    lv_obj_delete(few.page);

    PagerPage many = NewPagerPage(12);
    const int pages = widgets::HorizontalPagerPages(many.pager);
    Check(pages > 1, "twelve chips need more than one page");
    Check(pages == ExpectedPages(many.pager.root), "a page is the whole chips that fit the content column, as many as fit");
    Check(!lv_obj_has_flag(many.pager.words_row, LV_OBJ_FLAG_HIDDEN), "the page words show");
    Check(widgets::HorizontalPagerPage(many.pager) == 0, "it opens on the first page");
    // The words alone say the page, left-aligned where the strip begins: one label, "1 of N", no page dots beside it.
    Check(lv_obj_get_child_count(many.pager.words_row) == 1, "the words are all that stands under the strip: no page dots");
    lv_obj_t *words = lv_obj_get_child(many.pager.words_row, 0);
    char expected[24];
    std::snprintf(expected, sizeof expected, "1 of %d", pages);
    Check(std::string(lv_label_get_text(words)) == expected, "the words read '1 of N'");
    lv_area_t wa, ra;
    lv_obj_get_coords(words, &wa);
    lv_obj_get_coords(many.pager.words_row, &ra);
    Check(wa.x1 == ra.x1, "left-aligned at the row's left edge");
    Check(lv_obj_get_style_text_font(words, LV_PART_MAIN) == ui::Font(ui::Type::kCaption), "in caption");
    lv_obj_delete(many.page);
}

void TestHorizontalSwipe()
{
    PagerPage p = NewPagerPage(12);
    const int pages = widgets::HorizontalPagerPages(p.pager);
    int x_from, x_to, y;
    StripPoint(p, &x_from, &x_to, &y);
    DragX(x_from, x_to - kScrollLimit - 0, y, 1);  // slowly, a long way: no flick needed
    Check(widgets::HorizontalPagerPage(p.pager) == 1, "a horizontal drag left on the strip turns exactly one page");
    Check(lv_obj_get_scroll_x(p.pager.root) == kView, "the band is on the new page's start");
    Check(lv_obj_get_scroll_y(p.page) == 0, "and the page behind has not moved");
    Check(lvgl_display::s_at_ends == 0, "a drag that turns a page plays no at-end sound");
    DragX(x_to, x_from, y, 1);
    Check(widgets::HorizontalPagerPage(p.pager) == 0 && lv_obj_get_scroll_x(p.pager.root) == 0, "a drag right turns back one page");
    {
        char again[24];
        std::snprintf(again, sizeof again, "1 of %d", pages);
        Check(std::string(lv_label_get_text(lv_obj_get_child(p.pager.words_row, 0))) == again, "and the words follow it");
    }
    Check(lvgl_display::s_at_ends == 0, "nor does the turn back");
    DragX(x_to, x_from, y, 1);
    Check(widgets::HorizontalPagerPage(p.pager) == 0, "a drag right on the first page turns nothing: nothing wraps");
    Check(lvgl_display::s_at_ends == 1, "and plays the at-end sound, once");
    for (int i = 0; i < pages + 1; ++i) {
        DragX(x_from, x_to, y, 1);
    }
    Check(widgets::HorizontalPagerPage(p.pager) == pages - 1, "drags left stop at the last page");
    Check(lvgl_display::s_at_ends == 3, "the drags past the last page play it once each, the turns before it never");
    lv_obj_delete(p.page);
}

void TestVerticalDragChains()
{
    PagerPage p = NewPagerPage(12);
    int x_from, x_to, y;
    StripPoint(p, &x_from, &x_to, &y);
    const int x = (x_from + x_to) / 2;
    s_x = x;
    s_y = y;
    s_down = true;
    Tick();
    for (int dy = 4; dy <= 120; dy += 4) {
        s_y = y - dy;
        Tick();
    }
    s_down = false;
    for (int i = 0; i < 6; ++i) {
        Tick();
    }
    Check(widgets::HorizontalPagerPage(p.pager) == 0 && lv_obj_get_scroll_x(p.pager.root) == 0, "a vertical drag on the strip turns no pager page");
    Check(lv_obj_get_scroll_y(p.page) > 0, "it chains to the page behind, which scrolls");
    lv_obj_delete(p.page);
}

}  // namespace

int main()
{
    lv_init();
    static uint8_t buf[480 * 40 * 4];
    lv_display_t *display = lv_display_create(480, 800);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_L8);
    lv_display_set_theme(display, nullptr);  // as the firmware: no LVGL theme is compiled in
    s_group = lv_group_create();
    lv_group_set_default(s_group);
    if (ui::Start() != ESP_OK) {
        std::printf("ui::Start failed\n");
        return 1;
    }
    lv_display_set_buffers(display, buf, nullptr, sizeof buf, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, Flush);
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, ReadPointer);
    Tick();

    TestNoRecursion();
    TestTurnsOnePage();
    TestShortDragPlaces();
    TestSecondSwipeAndBack();
    TestFlick();
    TestDisabledChild();
    TestKeysAreLeftAlone();
    TestPagerPages();
    TestHorizontalSwipe();
    TestVerticalDragChains();

    std::printf("swipe on LVGL: %d checks, %d failed\n", s_checks, s_failures);
    return s_failures == 0 ? 0 : 1;
}
