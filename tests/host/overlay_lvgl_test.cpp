// Host test of the router's overlay stack (components/router/overlays.cpp) with the real select sheet, dialog and keyboard on
// LVGL's real objects, groups and keypad, on the computer with no board: opening an overlay hands the buttons to its own group,
// made LVGL's default before its controls are built, and takes the ring off the control under it; closing hands the buttons
// back, the ring back only if it showed; a dialog opened over a sheet gives the buttons back to the sheet; the screen hears
// OnCovered at the first overlay's opening and the last one's closing only; only the keyboard, a full-glass overlay, closes on a
// full refresh and brings back the screen it opened over, closed from under another overlay too; holding OK closes the top overlay before it pops a screen; and a
// screen change closes what stands on the screen (APP_ARCHITECTURE §7).
// run.sh builds LVGL's sources from managed_components once, into build/host/lvgl, and the sources named below, against stand-ins
// for the ESP-IDF headers (tests/host/stubs).
// host-sources: components/ui/ui.cpp components/ui/line_box.cpp components/ui/fonts/bitcount/*.c components/widgets/marks/marks.c components/widgets/src/*.cpp components/widgets/src/internal/*.cpp components/router/router.cpp components/router/overlays.cpp components/router/focus.cpp components/router/touch.cpp components/router/routes.cpp components/router/screen.cpp components/router/status.cpp
// Build and run: bash tests/host/run.sh

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "lvgl.h"
#include "lvgl_display.h"
#include "router/router.h"
#include "ui.h"
#include "widgets/widgets.h"

// --- The stand-ins for lvgl_display's side ------------------------------------------------------------------------------------

namespace {
lv_indev_t *s_keys = nullptr;
lv_group_t *s_bound = nullptr;
void (*s_back)() = nullptr;
int s_fulls = 0;
uint32_t s_ms = 0;
}  // namespace

namespace lvgl_display {
lv_group_t *Group() { return s_bound; }
void SetGroup(lv_group_t *group)
{
    s_bound = group;
    lv_indev_set_group(s_keys, group);
    lv_group_set_default(group);
}
void ClearGroup()
{
    lv_group_set_editing(s_bound, false);
    lv_group_remove_all_objs(s_bound);
}
void SetBackHandler(void (*back)()) { s_back = back; }
// The router's read hooks, kept so a test can run a key read through the focus rules as LVGL's read would.
void (*s_key_hook)(lv_indev_t *, lv_indev_data_t *) = nullptr;
void (*s_touch_hook)(lv_indev_t *, lv_indev_data_t *) = nullptr;
void SetKeyReadHook(void (*hook)(lv_indev_t *, lv_indev_data_t *)) { s_key_hook = hook; }
void SetTouchReadHook(void (*hook)(lv_indev_t *, lv_indev_data_t *)) { s_touch_hook = hook; }
void SetPartialCounting(PartialCounting) {}
void NoteTyping() {}
void RequestFull() { ++s_fulls; }
void PlayCue(Cue) {}
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

void Flush(lv_display_t *display, const lv_area_t *, uint8_t *)
{
    lv_display_flush_ready(display);
}

uint32_t Ticks() { return s_ms; }

void Wait(uint32_t ms)
{
    s_ms += ms;
    lv_timer_handler();
}

bool Ringed(lv_obj_t *obj)
{
    return lv_obj_has_state(obj, LV_STATE_FOCUS_KEY);
}

// --- A screen that says what it hears, with a dropdown and two buttons --------------------------------------------------------

std::vector<std::string> s_log;

struct Page {
    lv_obj_t *root = nullptr;
    lv_group_t *group = nullptr;
    widgets::Dropdown *dropdown = nullptr;
    lv_obj_t *button = nullptr;
};
Page s_home;  // home's page, the root
Page s_page;  // the page on top: home's, or one pushed over it

class Probe final : public router::Screen {
public:
    explicit Probe(std::string name) : name_(std::move(name)) {}
    void OnCreate(lv_obj_t *root, lv_group_t *group) override
    {
        ui::StyleScreen(root);
        static const char *const kOptions[] = {"Never", "Every day", "Every week"};
        s_page = Page{root, group, widgets::MakeDropdown(root, "Repeat", kOptions, 3, 0),
                      widgets::MakeButton(root, "Save", widgets::ButtonVariant::kAction)};
    }
    void OnCovered(bool covered) override { s_log.push_back(name_ + (covered ? " covered" : " uncovered")); }
    void OnExit() override { s_log.push_back(name_ + " exit"); }

private:
    std::string name_;
};

int s_dialog_answers = 0;
bool s_dialog_accepted = true;

void OpenTestDialog(bool destructive)
{
    widgets::DialogConfig config{};
    config.title = "Delete this note?";
    config.message = "It goes from the device.";
    config.action = "Delete";
    config.destructive = destructive;
    config.done = [](bool accepted, void *) {
        ++s_dialog_answers;
        s_dialog_accepted = accepted;
    };
    widgets::OpenDialog(config);
}

// --- The tests --------------------------------------------------------------------------------------------------------------

void TestGroupHandedOverAndBack()
{
    lv_group_focus_obj(s_page.button);  // the page opened with no ring: a move rings, as a key press does
    lv_group_focus_obj(s_page.dropdown->field);
    Check(Ringed(s_page.dropdown->field), "the field is ringed on the page");
    const uint32_t groups = lv_group_get_count();
    s_log.clear();
    widgets::OpenSelectSheet(s_page.dropdown);
    Check(router::OverlayOpen() && widgets::SelectSheetOpen(), "the sheet is an open overlay");
    Check(s_bound != s_page.group && lv_group_get_default() == s_bound, "the buttons go to the sheet's own group, LVGL's default");
    Check(lv_group_get_count() == groups + 1, "a group of its own, made by the router");
    Check(lv_group_get_obj_count(s_bound) == 4, "its stops are its three rows and Cancel, nothing behind it");
    Check(!Ringed(s_page.dropdown->field) && lv_group_get_focused(s_page.group) == s_page.dropdown->field,
          "the field under it loses its ring, and its group still names it");
    Check(s_log == std::vector<std::string>{"page covered"}, "the screen is told it is covered");
    widgets::CloseSelectSheet();
    Check(!router::OverlayOpen() && s_bound == s_page.group, "closing gives the buttons back to the page");
    Check(lv_group_get_count() == groups, "and the sheet's group is deleted");
    Check(lv_group_get_focused(s_page.group) == s_page.dropdown->field && Ringed(s_page.dropdown->field),
          "focus is on the field, ringed, as it was when the sheet opened");
    Check(s_log == std::vector<std::string>{"page covered", "page uncovered"}, "and the screen is told it is uncovered");

    // Opened with no ring under it — a tap's — it comes back with none.
    lv_obj_remove_state(s_page.dropdown->field, LV_STATE_FOCUS_KEY);
    widgets::OpenSelectSheet(s_page.dropdown);
    widgets::CloseSelectSheet();
    Check(lv_group_get_focused(s_page.group) == s_page.dropdown->field && !Ringed(s_page.dropdown->field),
          "a field that had no ring when the sheet opened has none when it closes");
    Check(s_fulls == 0, "a sheet opens and closes on partials: no full refresh");
    s_log.clear();
}

void TestDialogOverSheet()
{
    lv_group_focus_obj(s_page.dropdown->field);
    widgets::OpenSelectSheet(s_page.dropdown);
    lv_group_t *sheet = s_bound;
    lv_obj_t *row = lv_group_get_focused(sheet);
    Check(row != nullptr && Ringed(row), "the sheet opens on its chosen row, ringed");
    s_log.clear();
    OpenTestDialog(true);
    Check(widgets::DialogOpen() && s_bound != sheet && lv_group_get_obj_count(s_bound) == 2,
          "a dialog over the sheet takes the buttons: its two buttons are the only stops");
    lv_obj_t *focused = lv_group_get_focused(s_bound);
    Check(focused != nullptr && std::string(lv_label_get_text(lv_obj_get_child(focused, 0))) == "Cancel",
          "a destructive dialog opens on Cancel");
    Check(!Ringed(row), "the sheet's row under it loses its ring");
    Check(s_log.empty(), "the screen hears nothing more: it was covered already");
    widgets::CloseDialog();
    Check(s_dialog_answers == 1 && !s_dialog_accepted, "closing it as Cancel answers Cancel");
    Check(s_bound == sheet && lv_group_get_focused(sheet) == row && Ringed(row),
          "closing the dialog gives the buttons back to the sheet, its row ringed again");
    Check(widgets::SelectSheetOpen() && s_log.empty(), "the sheet stays open, and the screen still hears nothing");
    widgets::CloseSelectSheet();
    Check(s_bound == s_page.group && s_log == std::vector<std::string>{"page uncovered"},
          "the sheet closing last uncovers the screen");
    Check(s_fulls == 0, "a dialog opens and closes on partials: no full refresh");

    OpenTestDialog(false);
    focused = lv_group_get_focused(s_bound);
    Check(focused != nullptr && std::string(lv_label_get_text(lv_obj_get_child(focused, 0))) == "Delete",
          "any other dialog opens on its action");
    widgets::CloseDialog();
    s_log.clear();
}

void TestKeyboardFull()
{
    lv_group_focus_obj(s_page.button);
    widgets::KeyboardConfig config{};
    config.title = "Device name";
    config.max_length = 63;
    config.action = "Save";
    int answers = 0;
    config.done = [](bool, void *user) { ++*static_cast<int *>(user); };
    config.user = &answers;
    const widgets::Keyboard keyboard = widgets::OpenKeyboard(config);
    Check(keyboard.screen != nullptr && lv_screen_active() == keyboard.screen, "the keyboard's own screen is on the glass");
    Check(s_bound != s_page.group && lv_group_get_obj_count(s_bound) > 30, "the buttons walk its keys, in its own group");
    Check(s_fulls == 0, "it opens on a partial");
    Check(!widgets::OpenKeyboard(config).screen, "one keyboard at a time");
    router::Escape();
    Check(answers == 1 && !router::OverlayOpen(), "holding OK cancels it, answered");
    Check(lv_screen_active() == s_page.root && s_bound == s_page.group, "the page it opened over is back, its group bound");
    Check(s_fulls == 1, "closing a full-glass overlay draws a full refresh, once");
    Check(lv_group_get_focused(s_page.group) == s_page.button && Ringed(s_page.button), "focus on what opened it, ringed");
    Wait(10);  // its screen is deleted after the close
    s_fulls = 0;
    s_log.clear();
}

void TestEscapeChain()
{
    router::Push(std::make_unique<Probe>("top"));
    Check(router::Depth() == 2, "a screen over home");
    widgets::OpenSelectSheet(s_page.dropdown);
    OpenTestDialog(false);
    s_dialog_answers = 0;
    s_log.clear();
    Check(router::Escape() && widgets::DialogOpen() == false && s_dialog_answers == 1 && !s_dialog_accepted,
          "holding OK first closes the dialog, as its Cancel");
    Check(router::Depth() == 2 && widgets::SelectSheetOpen(), "the sheet under it stays, and no screen is popped");
    Check(router::Escape() && !widgets::SelectSheetOpen() && router::Depth() == 2, "the next hold closes the sheet");
    Check(s_log == std::vector<std::string>{"top uncovered"}, "the screen is uncovered then, and only then");
    Check(router::Escape() && router::Depth() == 1, "the next pops the screen");
    s_page = s_home;
    Wait(10);

    // A screen change closes what stands on the screen it leaves.
    router::Push(std::make_unique<Probe>("other"));
    widgets::OpenSelectSheet(s_page.dropdown);
    router::Pop();
    Check(!router::OverlayOpen() && !widgets::SelectSheetOpen() && router::Depth() == 1, "a pop closes the overlays first");
    s_page = s_home;
    Wait(10);
}

void TestOverlayRules()
{
    // The overlay stack itself: a full stack refuses another, and an overlay already open is not opened twice.
    router::Overlay overlays[router::kMaxOverlays + 1];
    for (size_t i = 0; i < router::kMaxOverlays; ++i) {
        Check(router::OpenOverlay(&overlays[i]) != nullptr, "an overlay opens, up to kMaxOverlays");
    }
    Check(router::OpenOverlay(&overlays[router::kMaxOverlays]) == nullptr, "one past kMaxOverlays is refused");
    Check(router::OpenOverlay(&overlays[0]) == nullptr, "an open overlay is not opened again");
    // One closed from the middle: the one above it stands on what it stood on.
    router::CloseOverlay(&overlays[1]);
    Check(s_bound == overlays[router::kMaxOverlays - 1].group, "closing one under the top leaves the top with the buttons");
    router::DismissOverlays();
    Check(!router::OverlayOpen() && s_bound == lv_obj_get_group(s_page.button), "DismissOverlays closes them all, the page bound");
    router::CloseOverlay(&overlays[0]);
    Check(!router::OverlayOpen(), "closing one that is not open does nothing");
}

// A full-glass overlay closed from under another drawn on its screen — a dialog over the keyboard — still gives the glass back
// to the screen it opened over, drawn full, before its screen is deleted; the overlay over it keeps the buttons.
void TestFullGlassClosedUnderAnother()
{
    lv_group_focus_obj(s_page.dropdown->field);
    s_fulls = 0;
    router::Overlay full;
    lv_obj_t *own = lv_obj_create(nullptr);
    full.screen = own;
    Check(router::OpenOverlay(&full) != nullptr && lv_screen_active() == own, "a full-glass overlay loads its own screen");
    router::Overlay over;  // drawn on the full-glass overlay's screen
    lv_group_t *over_group = router::OpenOverlay(&over);
    Check(over_group != nullptr && lv_screen_active() == own, "an overlay over it leaves its screen on the glass");
    router::CloseOverlay(&full);
    Check(lv_screen_active() == s_page.root, "closing the full-glass one under it brings back the page it opened over");
    Check(s_fulls == 1, "drawn as a full refresh");
    Check(router::OverlayOpen() && s_bound == over_group, "the overlay over it stays open, with the buttons");
    lv_obj_delete(own);  // as the keyboard deletes its screen once closed: never the screen on the glass
    router::CloseOverlay(&over);
    Check(!router::OverlayOpen() && s_bound == s_page.group, "closing that gives the buttons back to the page");
    Check(lv_screen_active() == s_page.root && s_fulls == 1, "and the page stays, with no second full refresh");
    s_fulls = 0;
}

// A button press and release read through the router's focus rules, as LVGL's keypad read hands them over: from here the
// buttons are the last input given, so a focus the code makes with no input being read rings, as after a press on the board.
void PressThroughFocusRules()
{
    lv_indev_data_t data{};
    data.key = LV_KEY_NEXT;
    data.state = LV_INDEV_STATE_PRESSED;
    lvgl_display::s_key_hook(nullptr, &data);
    data.state = LV_INDEV_STATE_RELEASED;
    lvgl_display::s_key_hook(nullptr, &data);
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
    lv_display_set_theme(display, nullptr);
    // The buttons: a keypad, so a focus move rings as the firmware's does; LVGL's own reads of it are off.
    s_keys = lv_indev_create();
    lv_indev_set_type(s_keys, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(s_keys, [](lv_indev_t *, lv_indev_data_t *data) { data->state = LV_INDEV_STATE_RELEASED; });
    lv_indev_set_mode(s_keys, LV_INDEV_MODE_EVENT);
    if (ui::Start() != ESP_OK) {
        std::printf("ui::Start failed\n");
        return 1;
    }
    router::Init();
    PressThroughFocusRules();
    router::SetRoot(std::make_unique<Probe>("page"));
    s_home = s_page;
    TestGroupHandedOverAndBack();
    TestDialogOverSheet();
    TestKeyboardFull();
    TestEscapeChain();
    TestOverlayRules();
    TestFullGlassClosedUnderAnother();
    std::printf("overlays on LVGL: %d checks, %d failed\n", s_checks, s_failures);
    return s_failures == 0 ? 0 : 1;
}
