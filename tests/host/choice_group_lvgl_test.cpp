// Host test of the radio rows' and choice cards' groups on LVGL's real objects, groups and input, on the computer with no board: the
// rows (or cards) that share a parent are a group; OK or a tap on one chooses it, clears the others and then sends
// LV_EVENT_VALUE_CHANGED on it, once; choosing the chosen one sends nothing; moving the focus never chooses; the programmatic set
// sends nothing; a card of any number is a group of one and sends on each flip (DESIGN → Components → The kit; STICKY_DESIGN_TOKENS §9).
// run.sh builds LVGL's sources from managed_components (a firmware build fetches them) into build/host/lvgl, once, and the
// sources named below, against stand-ins for the ESP-IDF headers (tests/host/stubs).
// host-sources: components/ui/ui.cpp components/ui/line_box.cpp components/ui/fonts/bitcount/*.c components/widgets/marks/marks.c components/widgets/src/*.cpp components/widgets/src/internal/*.cpp components/router/router.cpp components/router/overlays.cpp components/router/focus.cpp components/router/touch.cpp components/router/routes.cpp components/router/screen.cpp components/router/status.cpp
// Build and run: bash tests/host/run.sh

#include <cstdio>
#include <string>
#include <vector>

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
void SetGroup(lv_group_t *group)
{
    if (group == nullptr) {
        return;
    }
    s_group = group;
    for (lv_indev_t *indev = lv_indev_get_next(nullptr); indev != nullptr; indev = lv_indev_get_next(indev)) {
        lv_indev_set_group(indev, group);
    }
    lv_group_set_default(group);
}
void SetBackHandler(void (*back)()) { s_back = back; }
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

constexpr uint32_t kReadMs = 33;
bool s_ok_down;
bool s_down;
int s_x, s_y;

void ReadKeypad(lv_indev_t *, lv_indev_data_t *data)
{
    data->key = LV_KEY_ENTER;
    data->state = s_ok_down ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
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

void PressOk()
{
    s_ok_down = true;
    Tick();
    s_ok_down = false;
    Tick();
}

void Tap(lv_obj_t *obj)
{
    lv_obj_update_layout(obj);
    lv_area_t a;
    lv_obj_get_coords(obj, &a);
    s_x = (a.x1 + a.x2) / 2;
    s_y = (a.y1 + a.y2) / 2;
    s_down = true;
    Tick();
    s_down = false;
    Tick();
}

// --- The screen ---------------------------------------------------------------------------------------------------------

lv_obj_t *s_root = nullptr;
lv_obj_t *s_column = nullptr;

void NewScreen()
{
    if (s_root != nullptr) {
        lvgl_display::ClearGroup();
        lv_obj_t *blank = lv_obj_create(nullptr);
        lv_screen_load(blank);
        lv_obj_delete(s_root);
    }
    s_root = ui::NewScreen();
    lv_screen_load(s_root);
    s_column = lv_obj_create(s_root);
    lv_obj_remove_style_all(s_column);
    lv_obj_set_width(s_column, lv_pct(100));
    lv_obj_set_flex_grow(s_column, 1);
    lv_obj_set_flex_flow(s_column, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_hor(s_column, dt::kSpacingEdge, 0);
    lv_obj_set_style_pad_row(s_column, dt::kSpacingMd, 0);
    lv_obj_set_scrollable(s_column, false);
    ui::LetRingOut(s_column);
}

lv_obj_t *Box()
{
    lv_obj_t *box = lv_obj_create(s_column);
    lv_obj_remove_style_all(box);
    lv_obj_set_size(box, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(box, dt::kSpacingMd, 0);
    lv_obj_set_scrollable(box, false);
    ui::LetRingOut(box);
    return box;
}

// What each member of a group said, in the order it said it: the member's index, and the states of the others at that moment.
struct Heard {
    int count = 0;
    int last = -1;
    std::vector<bool> others_clear_when_heard;  // each time: every other member was clear
};
struct Group {
    std::vector<lv_obj_t *> members;
    Heard heard;
};

void OnChanged(lv_event_t *e)
{
    Group *g = static_cast<Group *>(lv_event_get_user_data(e));
    lv_obj_t *chosen = lv_event_get_target_obj(e);
    ++g->heard.count;
    bool clear = true;
    for (size_t i = 0; i < g->members.size(); ++i) {
        if (g->members[i] == chosen) {
            g->heard.last = static_cast<int>(i);
        } else if (lv_obj_has_state(g->members[i], LV_STATE_CHECKED)) {
            clear = false;
        }
    }
    g->heard.others_clear_when_heard.push_back(clear && lv_obj_has_state(chosen, LV_STATE_CHECKED));
}

void Listen(Group &g)
{
    for (lv_obj_t *m : g.members) {
        lv_obj_add_event_cb(m, OnChanged, LV_EVENT_VALUE_CHANGED, &g);
    }
}

// Which members are chosen, as a string of 1 and 0.
std::string Chosen(const Group &g)
{
    std::string out;
    for (lv_obj_t *m : g.members) {
        out += lv_obj_has_state(m, LV_STATE_CHECKED) ? '1' : '0';
    }
    return out;
}

lv_obj_t *Focused()
{
    return lv_group_get_focused(s_group);
}

void FocusOn(lv_obj_t *obj)
{
    lv_group_focus_obj(obj);
    Tick();
}

void Clear(Group &g)
{
    g.heard = {};
}

// --- Radio rows ----------------------------------------------------------------------------------------------------------

void TestRadioRows()
{
    NewScreen();
    lv_obj_t *box = Box();
    Group g;
    for (const char *words : {"Never", "Every day", "Every week", "Every two weeks, on a Monday", "Every month"}) {
        g.members.push_back(widgets::MakeRadioRow(box, words));
    }
    Listen(g);
    lv_obj_update_layout(s_root);
    Tick();

    Check(Chosen(g) == "00000", "no row is chosen to begin with");
    widgets::SetRadioChosen(g.members[2], true);
    Check(Chosen(g) == "00100" && g.heard.count == 0, "SetRadioChosen chooses one and sends nothing");
    widgets::SetRadioChosen(g.members[0], true);
    Check(Chosen(g) == "10000" && g.heard.count == 0, "SetRadioChosen on another clears the first, still sending nothing");
    widgets::SetRadioChosen(g.members[0], false);
    Check(Chosen(g) == "00000" && g.heard.count == 0, "SetRadioChosen false clears it, sending nothing");
    widgets::SetRadioChosen(g.members[2], true);

    // The focus never chooses: a walk through every row changes nothing and says nothing.
    FocusOn(g.members[0]);
    for (int i = 0; i < 5; ++i) {
        lv_group_focus_next(s_group);
        Tick();
    }
    Check(Chosen(g) == "00100" && g.heard.count == 0, "moving the focus through every row chooses none and says nothing");

    // OK chooses: the others are cleared first, then one change is sent on the chosen row.
    FocusOn(g.members[1]);
    PressOk();
    Check(Chosen(g) == "01000", "OK on a row chooses it and clears the one that was chosen");
    Check(g.heard.count == 1 && g.heard.last == 1, "one LV_EVENT_VALUE_CHANGED, on the chosen row");
    Check(g.heard.others_clear_when_heard.size() == 1 && g.heard.others_clear_when_heard[0], "the others were cleared before the event was sent");
    Check(Focused() == g.members[1], "the focus stays where it was: choosing moves nothing");

    // Choosing the chosen row again sends nothing.
    PressOk();
    Check(Chosen(g) == "01000" && g.heard.count == 1, "OK on the chosen row again sends nothing and keeps it chosen");
    Check(widgets::RadioChosen(g.members[1]) && !widgets::RadioChosen(g.members[0]), "RadioChosen reads the state");

    // A tap chooses the same way.
    Clear(g);
    Tap(g.members[3]);
    Check(Chosen(g) == "00010", "a tap on a row chooses it and clears the rest");
    Check(g.heard.count == 1 && g.heard.last == 3 && g.heard.others_clear_when_heard[0], "one change on the tapped row, the others already clear");
    Tap(g.members[3]);
    Check(Chosen(g) == "00010" && g.heard.count == 1, "a tap on the chosen row sends nothing");

    // A disabled row is not chosen, by OK or a tap.
    lv_obj_add_state(g.members[4], LV_STATE_DISABLED);
    Tap(g.members[4]);
    Check(Chosen(g) == "00010" && g.heard.count == 1, "a disabled row is not chosen by a tap");
    lv_obj_remove_state(g.members[4], LV_STATE_DISABLED);
}

// Two groups under two parents are two groups; a sibling that is not a choice is left alone.
void TestGroupsAreParents()
{
    NewScreen();
    lv_obj_t *first_box = Box();
    lv_obj_t *second_box = Box();
    Group a, b;
    for (const char *words : {"Small", "Medium", "Large"}) {
        a.members.push_back(widgets::MakeRadioRow(first_box, words));
        b.members.push_back(widgets::MakeRadioRow(second_box, words));
    }
    lv_obj_t *note = ui::NewLabel(first_box, ui::Type::kCaption, "A note between the rows");
    (void)note;
    Listen(a);
    Listen(b);
    lv_obj_update_layout(s_root);
    Tick();
    widgets::SetRadioChosen(a.members[0], true);
    widgets::SetRadioChosen(b.members[0], true);
    Check(Chosen(a) == "100" && Chosen(b) == "100", "each parent keeps its own chosen row");
    FocusOn(a.members[2]);
    PressOk();
    Check(Chosen(a) == "001" && Chosen(b) == "100", "choosing in one parent leaves the other parent's choice alone");
    Check(a.heard.count == 1 && b.heard.count == 0, "and only the chosen row's group heard");
}

// --- Choice cards ----------------------------------------------------------------------------------------------------------

void TestChoiceCards()
{
    NewScreen();
    lv_obj_t *box = Box();
    Group g;
    g.members.push_back(widgets::MakeChoiceCard(box, "On Wi-Fi only", false, "Waits until the Sticky can reach a network."));
    g.members.push_back(widgets::MakeChoiceCard(box, "Straight away", false, "Sends each recording as soon as it is saved."));
    g.members.push_back(widgets::MakeChoiceCard(box, "Never", false, "Recordings stay on the card, as audio only."));
    Listen(g);
    lv_obj_update_layout(s_root);
    Tick();

    widgets::SetChoiceChosen(g.members[0], true);
    widgets::SetChoiceChosen(g.members[1], true);
    Check(Chosen(g) == "010" && g.heard.count == 0, "SetChoiceChosen keeps one chosen and sends nothing");
    FocusOn(g.members[2]);
    Check(Chosen(g) == "010" && g.heard.count == 0, "the focus on a card chooses nothing");
    PressOk();
    Check(Chosen(g) == "001" && g.heard.count == 1 && g.heard.last == 2, "OK chooses the card, clears the other, and sends one change on it");
    Check(g.heard.others_clear_when_heard[0], "the others were clear when it was sent");
    PressOk();
    Check(Chosen(g) == "001" && g.heard.count == 1, "OK on the chosen card sends nothing");
    Clear(g);
    Tap(g.members[0]);
    Check(Chosen(g) == "100" && g.heard.count == 1 && g.heard.last == 0, "a tap does what OK does");
    Check(widgets::ChoiceChosen(g.members[0]) && !widgets::ChoiceChosen(g.members[2]), "ChoiceChosen reads the state");
}

// A card of any number is a group of one: it flips and says so each time, and no other card is touched.
void TestManyCards()
{
    NewScreen();
    lv_obj_t *box = Box();
    Group g;
    g.members.push_back(widgets::MakeChoiceCard(box, "Keep the recording after it is transcribed", true));
    g.members.push_back(widgets::MakeChoiceCard(box, "Share follow-ups with my phone", true, "Only on your Wi-Fi"));
    Listen(g);
    lv_obj_update_layout(s_root);
    Tick();
    widgets::SetChoiceChosen(g.members[0], true);
    widgets::SetChoiceChosen(g.members[1], true);
    Check(Chosen(g) == "11" && g.heard.count == 0, "the program can set both: they are not a group");
    FocusOn(g.members[0]);
    PressOk();
    Check(Chosen(g) == "01" && g.heard.count == 1 && g.heard.last == 0, "OK flips it off and sends one change; the other stays");
    PressOk();
    Check(Chosen(g) == "11" && g.heard.count == 2, "OK again flips it on and sends again");
    Tap(g.members[1]);
    Check(Chosen(g) == "10" && g.heard.count == 3 && g.heard.last == 1, "a tap flips the other and sends on it");
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
    lv_indev_t *keypad = lv_indev_create();
    lv_indev_set_type(keypad, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(keypad, ReadKeypad);
    lv_indev_set_group(keypad, s_group);
    Tick();

    TestRadioRows();
    TestGroupsAreParents();
    TestChoiceCards();
    TestManyCards();

    std::printf("choice groups on LVGL: %d checks, %d failed\n", s_checks, s_failures);
    return s_failures == 0 ? 0 : 1;
}
