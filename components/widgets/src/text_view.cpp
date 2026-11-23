#include "widgets/text_view.h"

#include <algorithm>

#include "design_tokens.h"
#include "internal/base.h"
#include "lvgl_display.h"

namespace widgets {
namespace dt = design_tokens;

namespace {
struct State {
    ScrollIndicator indicator;
    lv_obj_t *label;
    int32_t lines;  // whole lines in the band
    int32_t line;   // a line box, px
    int page;
    int pages;
    int opened_page;  // the page when OK opened it for reading: holding OK turns back to it
};

State *Of(lv_obj_t *band)
{
    return static_cast<State *>(lv_obj_get_user_data(band));
}

// A page is the band's lines less one: its last line is the next page's first.
int32_t Step(const State *s)
{
    return std::max<int32_t>(1, s->lines - 1);
}

void ShowPage(lv_obj_t *band, int page, bool tell)
{
    State *s = Of(band);
    page = std::clamp(page, 0, s->pages - 1);
    lv_obj_scroll_to_y(band, page * Step(s) * s->line, LV_ANIM_OFF);
    const int32_t whole = ((s->pages - 1) * Step(s) + s->lines) * s->line;
    UpdateScrollIndicator(s->indicator, page, s->pages, s->lines * s->line, whole);
    const bool changed = page != s->page;
    s->page = page;
    if (changed && tell) {
        lv_obj_send_event(band, LV_EVENT_VALUE_CHANGED, nullptr);
    }
}

// Whole pages of whole lines: the text's lines counted, the page count worked out, and the room that completes the last page
// added under the label so the band never scrolls back by itself.
void Repage(lv_obj_t *band)
{
    State *s = Of(band);
    lv_obj_update_layout(band);
    const int32_t space = lv_obj_get_style_text_line_space(s->label, LV_PART_MAIN);
    const int32_t text = std::max<int32_t>(1, (lv_obj_get_height(s->label) + space + s->line / 2) / s->line);
    s->pages = text <= s->lines ? 1 : static_cast<int>((text - 1 + Step(s) - 1) / Step(s));
    const int32_t whole = ((s->pages - 1) * Step(s) + s->lines) * s->line;
    lv_obj_set_style_pad_bottom(band, whole - lv_obj_get_height(s->label), 0);
    s->page = 0;
    ShowPage(band, 0, false);
}

// One page on or back; at an end nothing changes, and the at-end sound says so after the press's tick.
void Turn(lv_obj_t *band, int dir)
{
    State *s = Of(band);
    const int page = s->page + dir;
    if (page < 0 || page > s->pages - 1) {
        lvgl_display::PlayCue(lvgl_display::Cue::kAtEnd);
        return;
    }
    ShowPage(band, page, true);
}

// While open for reading, UP and DOWN arrive as LV_KEY_UP and LV_KEY_DOWN: each turns a page.
void OnKey(lv_event_t *e)
{
    lv_obj_t *band = lv_event_get_target_obj(e);
    const uint32_t key = lv_event_get_key(e);
    if (key == LV_KEY_DOWN) {
        Turn(band, 1);
    } else if (key == LV_KEY_UP) {
        Turn(band, -1);
    }
}

void TurnPage(lv_obj_t *band, int32_t dir)
{
    Turn(band, static_cast<int>(dir));
}

void OnEditOpened(lv_event_t *e)
{
    State *s = Of(lv_event_get_current_target_obj(e));
    s->opened_page = s->page;
}

// Holding OK while it is open: back to the page it opened on (router::Escape's first link).
void OnCancel(lv_event_t *e)
{
    lv_obj_t *band = lv_event_get_current_target_obj(e);
    ShowPage(band, Of(band)->opened_page, true);
}

void OnDelete(lv_event_t *e)
{
    delete Of(lv_event_get_target_obj(e));
}
}  // namespace

TextView MakeTextView(lv_obj_t *parent, int32_t lines, const char *text)
{
    constexpr int32_t kInset = dt::kSpacingSm;  // the words stand this far inside the view, on every side the ring is
    TextView view{};
    const int32_t line = ui::LineBox(ui::Type::kCaptionLoose);
    // The view is the box the ring belongs to: whole lines and an inset above and below them. It has no box drawn, so the
    // ring alone shows it.
    const int32_t height = lines * line + 2 * kInset;
    view.frame = detail::Bare(parent);
    lv_obj_set_size(view.frame, lv_pct(100), height);
    detail::LetRingOut(view.frame);
    // The band is the words' room: the lines' whole height and the view's width less the inset, so a page turn never shows
    // a part of the line before or after it. It is the focus stop and draws the ring, kInset further out than its own edge,
    // which puts the ring where it would stand round the view; its corners are the view's less the inset, so the ring keeps
    // `rounded.lg` (no box: its corners shape its ring).
    view.root = lv_obj_create(view.frame);
    lv_obj_remove_style_all(view.root);
    lv_obj_set_size(view.root, dt::kSpacingContent - 2 * kInset, lines * line);
    lv_obj_set_pos(view.root, kInset, kInset);
    lv_obj_set_style_radius(view.root, dt::kRoundedLg - kInset, 0);
    lv_obj_remove_flag(view.root, LV_OBJ_FLAG_SCROLL_WITH_ARROW);  // OnKey turns the pages; LVGL's arrow scroll would add a second
    view.label = ui::NewLabel(view.root, ui::Type::kCaptionLoose, text);
    detail::ClaimSwipe(view.root, TurnPage);
    view.indicator = MakeScrollIndicator(view.frame, height);
    lv_obj_set_user_data(view.root, new State{view.indicator, view.label, lines, line, 0, 1, 0});
    lv_obj_add_event_cb(view.root, OnDelete, LV_EVENT_DELETE, nullptr);
    lv_obj_add_event_cb(view.root, OnKey, LV_EVENT_KEY, nullptr);
    lv_obj_add_event_cb(view.root, OnCancel, LV_EVENT_CANCEL, nullptr);
    detail::ToggleEditingOnKeyClick(view.root, OnEditOpened);
    ui::RingOutside(view.root, kInset);
    ui::RingEditing(view.root, kInset);  // 6 px while it is open for reading
    detail::JoinGroup(view.root, view.frame);  // the page brings the view's box, and the ring round it, into view
    Repage(view.root);
    return view;
}

void SetTextView(const TextView &view, const char *text)
{
    lv_label_set_text(view.label, text);
    Repage(view.root);
}

int TextViewPage(const TextView &view)
{
    return Of(view.root)->page;
}

int TextViewPages(const TextView &view)
{
    return Of(view.root)->pages;
}

}  // namespace widgets
