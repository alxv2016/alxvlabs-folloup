// Host test of the status line (components/widgets/src/status_line.cpp and components/router/status.cpp) on LVGL's real
// objects, the real fonts and the real router, on the computer with no board: the band is `spacing.list-row-height` high and
// its content stands at the same y empty, with one line, with two, and with more; its words stand in `caption`, centred
// in the band, wrapped on words and ended by LVGL's dots past two lines; its lower rule shows only while news does; it is no
// stop, no press and no scroll; Say draws and sounds success and error, never info, and the same words again draw nothing and
// still sound; Clear retracts its own line only; a screen with no band says nothing; a change of screen — Push, Pop, GoTo,
// PopToRoot — clears the band; and under an open dialog the band holds still, the news drawn, and sounded, as the dialog
// closes, a held line dropped by a screen change never drawn nor heard (DESIGN → Components → The kit; APP_ARCHITECTURE §7).
// The pure rules are status_rules_test.cpp's. run.sh builds LVGL's sources from managed_components once, into build/host/lvgl,
// and the sources named below, against stand-ins for the ESP-IDF headers (tests/host/stubs).
// host-sources: components/ui/ui.cpp components/ui/line_box.cpp components/ui/fonts/bitcount/*.c components/widgets/marks/marks.c components/widgets/src/*.cpp components/widgets/src/internal/*.cpp components/router/router.cpp components/router/overlays.cpp components/router/focus.cpp components/router/touch.cpp components/router/routes.cpp components/router/screen.cpp components/router/status.cpp
// Build and run: bash tests/host/run.sh

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "design_tokens.h"
#include "lvgl.h"
#include "lvgl_display.h"
#include "router/router.h"
#include "ui.h"
#include "widgets/widgets.h"

namespace dt = design_tokens;

// --- The stand-ins for lvgl_display's side ------------------------------------------------------------------------------------

namespace {
lv_indev_t *s_keys = nullptr;
lv_group_t *s_bound = nullptr;
uint32_t s_ms = 0;
std::vector<lvgl_display::Cue> s_cues;
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
void SetBackHandler(void (*)()) {}
void SetKeyReadHook(void (*)(lv_indev_t *, lv_indev_data_t *)) {}
void SetTouchReadHook(void (*)(lv_indev_t *, lv_indev_data_t *)) {}
void SetPartialCounting(PartialCounting) {}
void NoteTyping() {}
void RequestFull() {}
void PlayCue(Cue cue) { s_cues.push_back(cue); }
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

// --- A screen with a title, a band and a button below it; and one with no band ------------------------------------------------

struct Page {
    lv_obj_t *root = nullptr;
    lv_obj_t *band = nullptr;
    lv_obj_t *below = nullptr;
};
Page s_page;  // the page last built
Page s_root;  // the root's: the first News

class News final : public router::Screen {
public:
    void OnCreate(lv_obj_t *root, lv_group_t *) override
    {
        ui::StyleScreen(root);
        ui::NewStatusBar(root, "Storage");
        lv_obj_t *band = widgets::MakeStatusLine(root);
        lv_obj_t *below = widgets::MakeButton(root, "Read", widgets::ButtonVariant::kAction);
        s_page = Page{root, band, below};
    }
};

class Plain final : public router::Screen {
public:
    void OnCreate(lv_obj_t *root, lv_group_t *) override
    {
        ui::StyleScreen(root);
        ui::NewStatusBar(root, "About");
        s_page = Page{root, nullptr, widgets::MakeButton(root, "Back", widgets::ButtonVariant::kOutline)};
    }
};

// A screen that says news while it is built and while it leaves: neither has a screen on top to say it on.
router::StatusId s_said_in_create = 99;
router::StatusId s_said_in_exit = 99;
class Sayer final : public router::Screen {
public:
    void OnCreate(lv_obj_t *root, lv_group_t *) override
    {
        ui::StyleScreen(root);
        ui::NewStatusBar(root, "Storage");
        s_page = Page{root, widgets::MakeStatusLine(root), nullptr};
        s_said_in_create = router::Say(router::StatusKind::kError, "No card.");
    }
    void OnExit() override { s_said_in_exit = router::Say(router::StatusKind::kError, "No card."); }
};

// The band's two parts, as the zone's children: the words, then the rule.
lv_obj_t *Words(const Page &page) { return lv_obj_get_child(page.band, 0); }
lv_obj_t *Rule(const Page &page) { return lv_obj_get_child(page.band, 1); }

std::string Text(const Page &page) { return lv_label_get_text(Words(page)); }
bool RuleShows(const Page &page) { return !lv_obj_has_flag(Rule(page), LV_OBJ_FLAG_HIDDEN); }

int32_t BelowY(const Page &page)
{
    lv_obj_update_layout(page.root);
    return lv_obj_get_y(page.below);
}

const char *kOneLine = "Card read.";
const char *kTwoLines = "The card did not mount. Check it is FAT32, then try again.";  // 58 characters
// Past the copy cap and past two lines: LVGL ends the second in dots.
const char *kTooLong =
    "The card did not mount, and nothing on it could be read, so check that it is formatted as FAT32, then try again later.";

void TestBandLayout()
{
    router::SetRoot(std::make_unique<News>());
    s_root = s_page;
    const Page page = s_page;
    lv_obj_update_layout(page.root);
    Check(lv_obj_get_height(page.band) == dt::kSpacingListRowHeight && lv_obj_get_width(page.band) == 480,
          "the band is spacing.list-row-height high and as wide as the glass");
    Check(lv_obj_get_y(page.band) == dt::kSpacingStatusBar, "it stands directly under the title bar");
    Check(Text(page).empty() && !RuleShows(page), "empty: no words and no lower rule — white, no edge");
    const int32_t empty_y = BelowY(page);
    Check(empty_y == dt::kSpacingStatusBar + dt::kSpacingListRowHeight, "the content starts right under the band");

    router::StatusId id = router::Say(router::StatusKind::kInfo, kOneLine);
    lv_obj_update_layout(page.root);
    const lv_obj_t *words = Words(page);
    const int32_t line = ui::LineBox(ui::Type::kCaption);
    Check(id != 0 && Text(page) == kOneLine && RuleShows(page), "one line: the words, and the rule beneath them");
    Check(lv_obj_get_height(words) == line, "one line is one line box tall");
    lv_area_t area;
    lv_obj_get_coords(words, &area);
    Check(area.x1 == dt::kSpacingEdge, "the words stand at spacing.edge");
    Check(lv_obj_get_width(words) == dt::kSpacingContent, "and are spacing.content wide");
    const int32_t centre = lv_obj_get_y(page.band) + lv_obj_get_y(words) + lv_obj_get_height(words) / 2 - lv_obj_get_y(page.band);
    const int32_t middle = (dt::kSpacingListRowHeight - dt::kSpacingStrokeHairline) / 2;
    Check(centre - middle >= -1 && centre - middle <= 1, "they are centred above the rule");
    Check(lv_obj_get_style_text_font(words, LV_PART_MAIN) == ui::Font(ui::Type::kCaption), "in caption");
    Check(lv_obj_get_height(Rule(page)) == dt::kSpacingStrokeHairline && lv_obj_get_width(Rule(page)) == dt::kSpacingContent &&
              lv_obj_get_y(Rule(page)) == dt::kSpacingListRowHeight - dt::kSpacingStrokeHairline,
          "the rule is a hairline, spacing.content wide, in the band's last two pixels");
    Check(BelowY(page) == empty_y, "nothing below moved for one line");

    id = router::Say(router::StatusKind::kError, kTwoLines);
    lv_obj_update_layout(page.root);
    Check(lv_obj_get_height(words) == 2 * line, "two lines are two line boxes tall, wrapped on words");
    Check(std::strstr(Text(page).c_str(), "...") == nullptr, "and nothing is cut");
    Check(BelowY(page) == empty_y, "nothing below moved for two lines");

    router::Say(router::StatusKind::kError, kTooLong);
    lv_obj_update_layout(page.root);
    Check(lv_obj_get_height(words) == 2 * line, "past two lines the label stops at two");
    const std::string text = Text(page);
    Check(text.size() > 3 && text.compare(text.size() - 3, 3, "...") == 0, "and LVGL ends the second line in dots");
    Check(BelowY(page) == empty_y, "nothing below moved for a long one");

    router::Clear(router::Say(router::StatusKind::kInfo, kOneLine));
    lv_obj_update_layout(page.root);
    Check(Text(page).empty() && !RuleShows(page), "retracted: white again, the rule gone");
    Check(BelowY(page) == empty_y, "nothing below moved for the retraction");
}

void TestNoControl()
{
    const Page page = s_page;
    Check(!lv_obj_has_flag(page.band, LV_OBJ_FLAG_CLICKABLE) && !lv_obj_has_flag(page.band, LV_OBJ_FLAG_SCROLLABLE),
          "the band takes no press and no scroll");
    Check(!lv_obj_has_flag(Words(page), LV_OBJ_FLAG_CLICKABLE) && !lv_obj_has_flag(Rule(page), LV_OBJ_FLAG_CLICKABLE),
          "nor do its words or its rule");
    Check(lv_obj_get_group(page.band) == nullptr && lv_obj_get_group(Words(page)) == nullptr && lv_obj_get_group(Rule(page)) == nullptr,
          "none of it is in a focus group: no stop");
    Check(lv_group_get_obj_count(s_bound) == 1, "the screen's only stop is its button");
    Check(lv_obj_get_style_bg_opa(page.band, LV_PART_MAIN) == LV_OPA_TRANSP && lv_obj_get_style_border_width(page.band, LV_PART_MAIN) == 0 &&
              lv_obj_get_style_outline_width(page.band, LV_PART_MAIN) == 0 && lv_obj_get_style_radius(page.band, LV_PART_MAIN) == 0,
          "no fill, no edge, no outline, no corners: nothing a control could be taken from");
}

void TestSounds()
{
    s_cues.clear();
    router::Say(router::StatusKind::kInfo, "Card removed.");
    Check(s_cues.empty(), "info is silent");
    router::Say(router::StatusKind::kSuccess, kOneLine);
    Check(s_cues.size() == 1 && s_cues[0] == lvgl_display::Cue::kSuccess, "success sounds sound.success");
    router::Say(router::StatusKind::kSuccess, kOneLine);
    Check(s_cues.size() == 2 && s_cues[1] == lvgl_display::Cue::kSuccess, "the same words again: the sound again, no redraw");
    const router::StatusId error = router::Say(router::StatusKind::kError, kTwoLines);
    Check(s_cues.size() == 3 && s_cues[2] == lvgl_display::Cue::kError, "error sounds sound.error");
    router::Say(router::StatusKind::kError, kTwoLines);
    Check(s_cues.size() == 4 && s_cues[3] == lvgl_display::Cue::kError, "the same error again: sound.error again");
    router::Clear(error);
    Check(s_cues.size() == 4, "a retraction is silent");
}

void TestClearById()
{
    const Page page = s_page;
    const router::StatusId first = router::Say(router::StatusKind::kInfo, "Card removed.");
    const router::StatusId second = router::Say(router::StatusKind::kSuccess, kOneLine);
    router::Clear(first);
    Check(Text(page) == kOneLine && RuleShows(page), "a stale id retracts nothing: the newer line stays");
    router::Clear(0);
    Check(Text(page) == kOneLine, "nor does id 0");
    router::Clear(second);
    Check(Text(page).empty() && !RuleShows(page), "its own id does");
}

void TestScreenChange()
{
    const Page news = s_page;
    router::Say(router::StatusKind::kInfo, "Card removed.");
    Check(router::Push(std::make_unique<Plain>()) == ESP_OK, "(a screen pushed over)");
    Check(Text(news).empty() && !RuleShows(news), "Push leaves the covered screen's band white");
    s_cues.clear();
    const router::StatusId none = router::Say(router::StatusKind::kError, "No card.");
    Check(none == 0 && s_cues.empty(), "a screen with no band says nothing and makes no sound");
    router::Pop();
    Check(Text(news).empty() && !RuleShows(news), "Pop shows the screen's band white");
    const router::StatusId stale = router::Say(router::StatusKind::kSuccess, kOneLine);
    router::Push(std::make_unique<Plain>());
    router::Pop();
    router::Clear(stale);
    Check(Text(news).empty(), "an id from before the change retracts nothing after it");
    router::Say(router::StatusKind::kInfo, "Card removed.");
    router::Push(std::make_unique<Plain>());
    router::Push(std::make_unique<Plain>());
    router::PopToRoot();
    Check(Text(news).empty() && !RuleShows(news), "PopToRoot leaves it white");
    router::Say(router::StatusKind::kInfo, "Card removed.");
    router::Push(std::make_unique<Plain>());
    router::GoTo(std::make_unique<News>());
    Check(Text(s_page).empty() && !RuleShows(s_page), "GoTo opens a new band white");
    router::Say(router::StatusKind::kInfo, kOneLine);
    Check(Text(s_page) == kOneLine, "and it takes news");
    router::PopToRoot();
    Wait(50);  // the popped screens are deleted on LVGL's timer
}

void TestSayOutsideOnEnter()
{
    s_cues.clear();
    router::Push(std::make_unique<Sayer>());
    const Page sayer = s_page;
    Check(s_said_in_create == 0, "Say from OnCreate returns 0: the screen is not on top yet");
    Check(Text(sayer).empty() && !RuleShows(sayer) && s_cues.empty(), "nothing is drawn and nothing sounds for it");
    router::Push(std::make_unique<Plain>());
    Check(s_said_in_exit == 0, "Say from OnExit returns 0: the screen has left");
    Check(Text(sayer).empty() && s_cues.empty(), "and the leaving band stays white and silent");
    router::PopToRoot();
    Wait(50);
}

widgets::DialogConfig TestDialog()
{
    widgets::DialogConfig config{};
    config.title = "Read the card?";
    config.message = "It mounts, reads and unmounts.";
    config.action = "Read";
    config.destructive = false;
    return config;
}

void TestHoldUnderDialog()
{
    router::PopToRoot();
    const Page page = s_root;
    Check(router::Depth() == 1, "(home)");
    const router::StatusId shown = router::Say(router::StatusKind::kInfo, "Card removed.");
    s_cues.clear();
    widgets::OpenDialog(TestDialog());
    Check(router::OverlayOpen(), "(a dialog is open)");
    const router::StatusId held = router::Say(router::StatusKind::kError, "No card. Insert one to read it.");
    Check(held != 0 && held != shown, "news said under the dialog still gets its id");
    Check(Text(page) == "Card removed." && s_cues.empty(), "the band holds still, and nothing is heard");
    widgets::CloseDialog();
    Check(Text(page) == "No card. Insert one to read it." && RuleShows(page), "when the dialog closes the news is drawn");
    Check(s_cues.size() == 1 && s_cues[0] == lvgl_display::Cue::kError, "and sounded, then");

    // Retracted under the dialog: never drawn, never heard.
    s_cues.clear();
    widgets::OpenDialog(TestDialog());
    const router::StatusId gone = router::Say(router::StatusKind::kSuccess, kOneLine);
    router::Clear(gone);
    widgets::CloseDialog();
    Check(Text(page).empty() && !RuleShows(page) && s_cues.empty(),
          "a held line retracted before the close is never drawn nor heard — the band it replaced goes white, as with no overlay");

    // A screen change drops held news: it is not drawn as Push closes the dialog.
    widgets::OpenDialog(TestDialog());
    router::Say(router::StatusKind::kSuccess, kOneLine);
    router::Push(std::make_unique<Plain>());
    Check(s_cues.empty() && Text(page).empty(), "a screen change drops held news: nothing drawn, nothing heard");
    router::Pop();
    Check(Text(page).empty() && s_cues.empty(), "and it is not drawn when the screen comes back");
    Wait(50);
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
    s_keys = lv_indev_create();
    lv_indev_set_type(s_keys, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(s_keys, [](lv_indev_t *, lv_indev_data_t *data) { data->state = LV_INDEV_STATE_RELEASED; });
    lv_indev_set_mode(s_keys, LV_INDEV_MODE_EVENT);
    if (ui::Start() != ESP_OK) {
        std::printf("ui::Start failed\n");
        return 1;
    }
    router::Init();
    TestBandLayout();
    TestNoControl();
    TestSounds();
    TestClearById();
    TestScreenChange();
    TestSayOutsideOnEnter();
    TestHoldUnderDialog();
    std::printf("status line on LVGL: %d checks, %d failed\n", s_checks, s_failures);
    return s_failures == 0 ? 0 : 1;
}
