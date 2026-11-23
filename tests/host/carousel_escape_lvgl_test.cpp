// Host test of holding OK on a carousel, through the router's own escape (router::AttachEscape), on LVGL's real objects and the real
// router, on the computer with no board: the carousel's screen is the root, at depth 1 where going back is nothing, and holding OK
// still leaves — it calls the carousel's `leave`, as Skip does; over another screen, at depth 2, it does the same; a screen with no
// carousel still goes back; and an escape attached outside OnCreate is ignored (STICKY_DESIGN_TOKENS §9, APP_ARCHITECTURE §7).
// run.sh builds LVGL's sources from managed_components (a firmware build fetches them) into build/host/lvgl, once, and the
// sources named below, against stand-ins for the ESP-IDF headers (tests/host/stubs).
// host-sources: components/ui/ui.cpp components/ui/line_box.cpp components/ui/fonts/bitcount/*.c components/widgets/marks/marks.c components/widgets/src/*.cpp components/widgets/src/internal/*.cpp components/router/router.cpp components/router/overlays.cpp components/router/focus.cpp components/router/touch.cpp components/router/routes.cpp components/router/screen.cpp components/router/status.cpp
// Build and run: bash tests/host/run.sh

#include <cstdio>
#include <string>

#include "design_tokens.h"
#include "lvgl.h"
#include "lvgl_display.h"
#include "router/router.h"
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

void Flush(lv_display_t *display, const lv_area_t *, uint8_t *)
{
    lv_display_flush_ready(display);
}

constexpr widgets::CarouselCard kCards[] = {
    {"Say it, and it is kept", "Hold the AI button and speak."},
    {"Ready when you are", "Press Done."},
};

int s_leaves;
void OnLeave(void *) { ++s_leaves; }

// A screen built as the Welcome screen is: a content column, the carousel in it, its footer on the screen.
void CreateWelcome(lv_obj_t *root, lv_group_t *, void *)
{
    ui::StyleScreen(root);
    lv_obj_t *content = lv_obj_create(root);
    lv_obj_remove_style_all(content);
    lv_obj_set_width(content, lv_pct(100));
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollable(content, false);
    widgets::MakeCarousel(root, content, kCards, 2, OnLeave, nullptr);
}

void CreatePlain(lv_obj_t *root, lv_group_t *, void *)
{
    ui::StyleScreen(root);
    ui::NewButton(root, "Back", false, nullptr, nullptr);
    router::AttachEscape(OnLeave, nullptr);  // from OnCreate: heard
}

void CreateNoEscape(lv_obj_t *root, lv_group_t *, void *)
{
    ui::StyleScreen(root);
    ui::NewButton(root, "Back", false, nullptr, nullptr);
}

void Run()
{
    // The carousel's screen is the root: depth 1, where going back is nothing.
    Check(router::SetRoot(router::MakeScreen(CreateWelcome)) == ESP_OK, "the carousel's screen is the root");
    Check(router::Depth() == 1, "depth 1");
    s_leaves = 0;
    Check(router::Escape(), "holding OK at depth 1 on a carousel does something");
    Check(s_leaves == 1, "it calls the carousel's leave, as Skip does");
    Check(router::Depth() == 1, "and the router itself pops nothing at the root");

    // A screen with no escape still goes back; one over the carousel's leaves through its own.
    Check(router::Push(router::MakeScreen(CreateNoEscape)) == ESP_OK && router::Depth() == 2, "a screen with no escape over it");
    Check(router::Escape() && router::Depth() == 1, "holding OK there goes back, as before");
    lv_timer_handler();
    Check(router::Push(router::MakeScreen(CreateWelcome)) == ESP_OK && router::Depth() == 2, "a carousel over home");
    s_leaves = 0;
    Check(router::Escape() && s_leaves == 1, "at depth 2 holding OK calls the carousel's leave too");
    Check(router::Depth() == 2, "leaving the screen is its leave's: the hardware test's pops it");
    Check(router::Push(router::MakeScreen(CreatePlain)) == ESP_OK, "a screen that attached an escape from OnCreate");
    s_leaves = 0;
    Check(router::Escape() && s_leaves == 1 && router::Depth() == 3, "its escape runs in place of going back");

    // Outside OnCreate, an escape is ignored.
    router::AttachEscape(OnLeave, nullptr);
    s_leaves = 0;
    router::Pop();
    router::Pop();
    Check(router::Depth() == 1 && router::Escape(), "an escape attached outside OnCreate attaches nothing: the carousel root still answers");
    Check(s_leaves == 1, "and it is the carousel's, once");
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
    lv_indev_t *keys = lv_indev_create();
    lv_indev_set_type(keys, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(keys, [](lv_indev_t *, lv_indev_data_t *data) { data->state = LV_INDEV_STATE_RELEASED; });
    lv_indev_set_group(keys, s_group);
    if (router::Init() != ESP_OK) {
        std::printf("router::Init failed\n");
        return 1;
    }
    Run();
    std::printf("carousel escape on LVGL: %d checks, %d failed\n", s_checks, s_failures);
    return s_failures == 0 ? 0 : 1;
}
