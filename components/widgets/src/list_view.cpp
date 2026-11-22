#include "widgets/list_view.h"

#include <algorithm>
#include <vector>

#include "design_tokens.h"
#include "internal/base.h"
#include "lvgl_display.h"

namespace widgets {
namespace dt = design_tokens;

namespace {
constexpr int32_t kGap = dt::kSpacingMd;

struct State {
    ScrollIndicator indicator;
    int32_t per;
    int32_t pitch;  // a row and the gap under it
    int32_t view;   // what a page holds, px: `per` rows of the nominal height and the gaps between them
    std::vector<int> first_row;  // each page's first row, by index: the whole rows that fit the view
    int page;
};

State *Of(lv_obj_t *band)
{
    return static_cast<State *>(lv_obj_get_user_data(band));
}

int Pages(lv_obj_t *band)
{
    return static_cast<int>(std::max<size_t>(1, Of(band)->first_row.size()));
}

int PageOfRow(const State *s, int index)
{
    int page = 0;
    for (size_t p = 0; p < s->first_row.size(); ++p) {
        if (s->first_row[p] <= index) {
            page = static_cast<int>(p);
        }
    }
    return page;
}

// Pages are whole rows: each takes the rows that fit the view, the next page starting with the next row. Rows of the nominal
// height make `per` of them a page, as always; a row that grew with its words (a two-line option's third line) takes the
// room of more than one, and a page holds fewer.
void Repage(lv_obj_t *band)
{
    State *s = Of(band);
    lv_obj_update_layout(band);
    s->first_row.clear();
    const int32_t n = static_cast<int32_t>(lv_obj_get_child_count(band));
    int32_t used = 0;
    for (int32_t i = 0; i < n; ++i) {
        const int32_t height = lv_obj_get_height(lv_obj_get_child(band, i));
        if (s->first_row.empty() || used + kGap + height > s->view) {
            s->first_row.push_back(static_cast<int>(i));
            used = height;
        } else {
            used += kGap + height;
        }
    }
}

// The page lands at once: the band scrolled to its first row, the indicator's thumb on its place.
void ShowPage(lv_obj_t *band, int page, bool tell)
{
    State *s = Of(band);
    const int pages = Pages(band);
    page = std::clamp(page, 0, pages - 1);
    lv_obj_update_layout(band);
    lv_obj_t *first = s->first_row.empty() ? nullptr : lv_obj_get_child(band, s->first_row[static_cast<size_t>(page)]);
    if (first != nullptr) {
        lv_obj_scroll_to_y(band, lv_obj_get_y(first), LV_ANIM_OFF);
    }
    const int32_t whole = s->view + kGap;
    UpdateScrollIndicator(s->indicator, page, pages, whole, pages * whole);
    const bool changed = page != s->page;
    s->page = page;
    if (changed && tell) {
        lv_obj_send_event(band, LV_EVENT_VALUE_CHANGED, nullptr);
    }
}

// A row that takes the focus outside the page turns it: a row below becomes the page's first, a row above its last.
void OnRowFocused(lv_event_t *e)
{
    lv_obj_t *band = static_cast<lv_obj_t *>(lv_event_get_user_data(e));
    const int index = static_cast<int>(lv_obj_get_index(lv_event_get_target_obj(e)));
    const int page = PageOfRow(Of(band), index);
    if (page != Of(band)->page) {
        ShowPage(band, page, true);
    }
}

// A swipe turns one page; the focus, if it was in the band, goes to the new page's first row.
void TurnPage(lv_obj_t *band, int32_t dir)
{
    State *s = Of(band);
    const int page = std::clamp(s->page + static_cast<int>(dir), 0, Pages(band) - 1);
    if (page == s->page) {
        return;  // at an end: the press's tick answered it, and nothing wraps under a finger
    }
    lv_obj_t *focused = lv_group_get_focused(lvgl_display::Group());
    ShowPage(band, page, true);
    if (focused != nullptr && lv_obj_get_parent(focused) == band) {
        lv_group_focus_obj(lv_obj_get_child(band, s->first_row[static_cast<size_t>(page)]));
    }
}

void OnDelete(lv_event_t *e)
{
    delete Of(lv_event_get_target_obj(e));
}
}  // namespace

ListView MakeListView(lv_obj_t *parent, int32_t rows_per_page, int32_t row_height, IndicatorCentre centre)
{
    constexpr int32_t kReach = dt::kSpacingSm;  // the ring's reach: spacing.focus-offset + spacing.stroke.focus
    ListView view{};
    view.rows_per_page = rows_per_page;
    view.row_height = row_height;
    const int32_t height = rows_per_page * row_height + (rows_per_page - 1) * kGap;
    view.frame = detail::Bare(parent);
    lv_obj_set_size(view.frame, lv_pct(100), height);
    detail::LetRingOut(view.frame);
    // The band, wider and taller than the rows by the ring's reach on every side: it clips what it holds, and the ring is
    // inside it.
    view.root = lv_obj_create(view.frame);
    lv_obj_remove_style_all(view.root);
    lv_obj_set_size(view.root, dt::kSpacingContent + 2 * kReach, height + 2 * kReach);
    lv_obj_set_pos(view.root, -kReach, -kReach);
    lv_obj_set_flex_flow(view.root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(view.root, kReach, 0);
    lv_obj_set_style_pad_row(view.root, kGap, 0);
    detail::ClaimSwipe(view.root, TurnPage);
    view.indicator = MakeScrollIndicator(view.frame, height, centre);
    lv_obj_set_user_data(view.root, new State{view.indicator, rows_per_page, row_height + kGap, height, {}, 0});
    lv_obj_add_event_cb(view.root, OnDelete, LV_EVENT_DELETE, nullptr);
    return view;
}

void UpdateListView(const ListView &view)
{
    lv_obj_t *band = view.root;
    const uint32_t n = lv_obj_get_child_count(band);
    for (uint32_t i = 0; i < n; ++i) {
        lv_obj_t *row = lv_obj_get_child(band, static_cast<int32_t>(i));
        ui::ShowWhenFocused(row, view.frame);  // the list's page turns, not a scroll by the row; the page behind brings the list in
        lv_obj_remove_event_cb_with_user_data(row, OnRowFocused, band);
        lv_obj_add_event_cb(row, OnRowFocused, LV_EVENT_FOCUSED, band);
    }
    // The last page is as tall as the others, so the band never scrolls back by itself to fill the glass: the room under its
    // last row is what the page's first row's top and the view leave.
    Repage(band);
    State *s = Of(band);
    int32_t spare = 0;
    if (n > 0) {
        lv_obj_t *first = lv_obj_get_child(band, s->first_row.back());
        lv_obj_t *last = lv_obj_get_child(band, static_cast<int32_t>(n) - 1);
        spare = std::max<int32_t>(0, s->view - (lv_obj_get_y(last) + lv_obj_get_height(last) - lv_obj_get_y(first)));
    }
    lv_obj_set_style_pad_bottom(band, dt::kSpacingSm + spare, 0);
    s->page = 0;
    ShowPage(band, 0, false);
}

int ListViewPage(const ListView &view)
{
    return Of(view.root)->page;
}

int ListViewPages(const ListView &view)
{
    return Pages(view.root);
}

}  // namespace widgets
