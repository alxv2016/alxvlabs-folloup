// Host test of the carousel on LVGL's real objects, groups and input, on the computer with no board: one card at a time, "n of N" and
// the dot beside it, Back absent on the first card and Next reading "Done" on the last, a swipe left or right turning one card on the
// lift and the at-end sound past either end, the ring staying on its stop through a turn and handed to Next when Back goes, and Skip and
// Done leaving (STICKY_DESIGN_TOKENS §9).
// run.sh builds LVGL's sources from managed_components (a firmware build fetches them) into build/host/lvgl, once, and the
// sources named below, against stand-ins for the ESP-IDF headers (tests/host/stubs).
// host-sources: components/ui/ui.cpp components/ui/line_box.cpp components/ui/fonts/bitcount/*.c components/widgets/marks/marks.c components/widgets/src/*.cpp components/widgets/src/internal/*.cpp components/router/router.cpp components/router/overlays.cpp components/router/focus.cpp components/router/touch.cpp components/router/routes.cpp components/router/screen.cpp components/router/status.cpp
// Build and run: bash tests/host/run.sh

#include <cstdio>
#include <string>

#include "design_tokens.h"
#include "lvgl.h"
#include "lvgl_display.h"
#include "ui.h"
#include "widgets/widgets.h"

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
// The at-end cue a widget asks for, counted: a step into the end of a list that stops asks it, a wrap never.
int s_at_ends = 0;
void PlayCue(Cue cue) { s_at_ends += cue == Cue::kAtEnd; }
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

constexpr uint32_t kReadMs = 33;
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

constexpr widgets::CarouselCard kCards[] = {
    {"Say it, and it is kept", "Hold the AI button and speak."},
    {"What matters stays in view", "The follow-ups you pin stay on the glass."},
    {"Everything is on the card", "Your notes are kept on the MicroSD card."},
    {"Ready when you are", "Press Done."},
};

int s_leaves;
void OnLeave(void *) { ++s_leaves; }

lv_obj_t *s_screen = nullptr;
widgets::Carousel s_carousel;

void NewScreen()
{
    if (s_screen != nullptr) {
        lvgl_display::ClearGroup();
        lv_obj_t *blank = lv_obj_create(nullptr);
        lv_screen_load(blank);
        lv_obj_delete(s_screen);
    }
    s_screen = ui::NewScreen();
    lv_screen_load(s_screen);
    lv_obj_t *content = lv_obj_create(s_screen);
    lv_obj_remove_style_all(content);
    lv_obj_set_width(content, lv_pct(100));
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_hor(content, dt::kSpacingEdge, 0);
    lv_obj_set_style_pad_ver(content, dt::kSpacingMd, 0);
    lv_obj_set_style_pad_row(content, dt::kSpacingMd, 0);
    lv_obj_set_scrollable(content, false);
    ui::LetRingOut(content);
    s_carousel = widgets::MakeCarousel(s_screen, content, kCards, 4, OnLeave, nullptr);
    lv_obj_update_layout(s_screen);
    s_leaves = 0;
    lvgl_display::s_at_ends = 0;
    s_down = false;
    Tick();
}

std::string Words(lv_obj_t *obj)
{
    return lv_label_get_text(obj);
}

// The label of a button.
std::string Label(lv_obj_t *button)
{
    return lv_label_get_text(lv_obj_get_child(button, 0));
}

lv_obj_t *Title() { return lv_obj_get_child(s_carousel.card, 0); }

// The indicator: the words, and the dot that is the pill.
std::string Indicator()
{
    lv_obj_t *card = s_carousel.card;
    lv_obj_t *indicator = lv_obj_get_child(lv_obj_get_parent(card), lv_obj_get_index(card) + 1);
    return Words(lv_obj_get_child(indicator, 0));
}
int PillAt()
{
    lv_obj_t *card = s_carousel.card;
    lv_obj_t *dots = lv_obj_get_child(lv_obj_get_child(lv_obj_get_parent(card), lv_obj_get_index(card) + 1), 1);
    for (uint32_t i = 0; i < lv_obj_get_child_count(dots); ++i) {
        if (lv_obj_has_state(lv_obj_get_child(dots, static_cast<int32_t>(i)), LV_STATE_CHECKED)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// The dots' widths as laid out: the current page is a pill `spacing.page-pill` wide, every other a dot `spacing.page-dot`.
std::string DotWidths()
{
    lv_obj_t *card = s_carousel.card;
    lv_obj_t *dots = lv_obj_get_child(lv_obj_get_child(lv_obj_get_parent(card), lv_obj_get_index(card) + 1), 1);
    lv_obj_update_layout(dots);
    std::string out;
    for (uint32_t i = 0; i < lv_obj_get_child_count(dots); ++i) {
        out += std::to_string(lv_obj_get_width(lv_obj_get_child(dots, static_cast<int32_t>(i)))) + (i + 1 < lv_obj_get_child_count(dots) ? "," : "");
    }
    return out;
}

bool Shown(lv_obj_t *obj) { return !lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN); }

void Click(lv_obj_t *button)
{
    lv_obj_update_layout(s_screen);
    lv_area_t a;
    lv_obj_get_coords(button, &a);
    s_x = (a.x1 + a.x2) / 2;
    s_y = (a.y1 + a.y2) / 2;
    s_down = true;
    Tick();
    s_down = false;
    Tick();
    Tick();
}

// The finger from `from_x` to `to_x` across the card, and lifted.
void Swipe(int from_x, int to_x, int each)
{
    lv_area_t a;
    lv_obj_get_coords(s_carousel.card, &a);
    s_y = (a.y1 + a.y2) / 2;
    s_x = from_x;
    s_down = true;
    Tick();
    const int step = to_x < from_x ? -each : each;
    for (int x = from_x + step; step < 0 ? x >= to_x : x <= to_x; x += step) {
        s_x = x;
        Tick();
    }
    s_down = false;
    for (int i = 0; i < 6; ++i) {
        Tick();
    }
}

void TestFace()
{
    NewScreen();
    lv_area_t a;
    lv_obj_get_coords(s_carousel.card, &a);
    Check(lv_area_get_width(&a) == dt::kSpacingContent, "the card is the content's width, 432 px");
    lv_area_t foot;
    lv_obj_get_coords(s_carousel.next, &foot);
    Check(a.y2 < foot.y1 - dt::kSpacingMd, "the card fills the column down to the indicator, above the footer");
    Check(a.y2 - a.y1 + 1 > 500, "and is tall: it fills the height, not its words'");
    Check(Words(Title()) == "Say it, and it is kept", "card 1's title");
    Check(Indicator() == "1 of 4" && PillAt() == 0, "the indicator says 1 of 4 and the first dot is the pill");
    Check(!Shown(s_carousel.back), "card 1 has no Back");
    Check(Label(s_carousel.skip) == "Skip" && Label(s_carousel.next) == "Next", "Skip and Next");
    lv_area_t skip;
    lv_obj_get_coords(s_carousel.skip, &skip);
    Check(skip.x1 == dt::kSpacingEdge + 133 + dt::kSpacingMd, "Skip stands in the middle slot, x 173, with Back's slot empty");
    lv_area_t next;
    lv_obj_get_coords(s_carousel.next, &next);
    Check(next.x1 == dt::kSpacingEdge + 2 * (133 + dt::kSpacingMd) && lv_area_get_width(&next) == 133, "Next in the third slot, 133 wide");
    Check(lv_group_get_focused(lvgl_display::Group()) == s_carousel.next, "focus opens on Next");
    Check(!lv_obj_has_state(s_carousel.next, LV_STATE_FOCUS_KEY), "with no ring");
    Check(lv_obj_get_style_border_opa(s_carousel.skip, LV_PART_MAIN) == LV_OPA_COVER, "Skip is outlined like Back");
}

void TestButtons()
{
    NewScreen();
    Click(s_carousel.next);
    Check(widgets::CarouselIndex(s_carousel) == 1 && Words(Title()) == "What matters stays in view", "Next turns to card 2");
    Check(Indicator() == "2 of 4" && PillAt() == 1, "2 of 4, the second dot");
    Check(DotWidths() == "8,28,8,8", "and the second dot is the pill: " + DotWidths());
    Check(Shown(s_carousel.back), "card 2 has Back");
    Click(s_carousel.next);
    Click(s_carousel.next);
    Check(widgets::CarouselIndex(s_carousel) == 3 && Label(s_carousel.next) == "Done", "the last card's Next reads Done");
    Check(s_leaves == 0, "nothing has left yet");
    Click(s_carousel.back);
    Check(widgets::CarouselIndex(s_carousel) == 2 && Label(s_carousel.next) == "Next", "Back turns back, and Next reads Next again");
    Click(s_carousel.next);
    Click(s_carousel.next);
    Check(s_leaves == 1, "Done leaves");
    Click(s_carousel.skip);
    Check(s_leaves == 2, "Skip leaves, from any card");
}

void TestSwipe()
{
    NewScreen();
    Swipe(400, 300, 4);
    Check(widgets::CarouselIndex(s_carousel) == 1, "a swipe left turns to the next card");
    Swipe(400, 300, 4);
    Swipe(400, 250, 4);
    Check(widgets::CarouselIndex(s_carousel) == 3, "each swipe turns one card, however far");
    Check(lvgl_display::s_at_ends == 0, "no at-end sound while a card turns");
    Swipe(400, 300, 4);
    Check(widgets::CarouselIndex(s_carousel) == 3 && lvgl_display::s_at_ends == 1, "a swipe left on the last card turns nothing and sounds at-end");
    Swipe(100, 200, 4);
    Check(widgets::CarouselIndex(s_carousel) == 2, "a swipe right turns back one card");
    Swipe(100, 200, 4);
    Swipe(100, 200, 4);
    Check(widgets::CarouselIndex(s_carousel) == 0 && !Shown(s_carousel.back), "back to card 1, where Back is gone");
    Swipe(100, 200, 4);
    Check(widgets::CarouselIndex(s_carousel) == 0 && lvgl_display::s_at_ends == 2, "a swipe right on the first card turns nothing and sounds at-end");
    Check(lv_obj_get_scroll_x(s_carousel.card) == 0, "nothing follows the finger: the card is not scrolled");
    // A short drag places and turns nothing.
    Swipe(300, 300 - 30, 2);
    Check(widgets::CarouselIndex(s_carousel) == 0, "a short, slow drag turns nothing");
}

void TestRing()
{
    NewScreen();
    // The ring on Back stays on Back through a turn; Back going away hands it to Next.
    widgets::ShowCarouselCard(s_carousel, 1);
    lv_group_focus_obj(s_carousel.back);
    lv_obj_add_state(s_carousel.back, LV_STATE_FOCUS_KEY);
    widgets::ShowCarouselCard(s_carousel, 2);
    Check(lv_group_get_focused(lvgl_display::Group()) == s_carousel.back && lv_obj_has_state(s_carousel.back, LV_STATE_FOCUS_KEY), "a turn keeps the ring on its stop");
    widgets::ShowCarouselCard(s_carousel, 0);
    Check(lv_group_get_focused(lvgl_display::Group()) == s_carousel.next, "back to card 1: focus goes to Next");
    Check(lv_obj_has_state(s_carousel.next, LV_STATE_FOCUS_KEY), "and the ring with it, since it showed");
    // The buttons walk Skip and Next only on card 1.
    lv_group_focus_obj(s_carousel.skip);
    lv_group_focus_next(lvgl_display::Group());
    Check(lv_group_get_focused(lvgl_display::Group()) == s_carousel.next, "DOWN from Skip reaches Next");
    lv_group_focus_next(lvgl_display::Group());
    Check(lv_group_get_focused(lvgl_display::Group()) == s_carousel.skip, "and on from Next wraps to Skip, Back being absent");
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
    lv_indev_t *pointer = lv_indev_create();
    lv_indev_set_type(pointer, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(pointer, ReadPointer);
    Tick();

    TestFace();
    TestButtons();
    TestSwipe();
    TestRing();

    std::printf("carousel on LVGL: %d checks, %d failed\n", s_checks, s_failures);
    return s_failures == 0 ? 0 : 1;
}
