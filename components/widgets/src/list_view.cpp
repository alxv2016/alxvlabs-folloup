#include "widgets/list_view.h"

#include <algorithm>
#include <cstdio>
#include <utility>
#include <vector>

#include "design_tokens.h"
#include "internal/base.h"
#include "lvgl_display.h"

namespace widgets {
namespace dt = design_tokens;

namespace {
constexpr int32_t kGap = dt::kSpacingMd;
constexpr int32_t kReach = dt::kSpacingSm;  // the ring's reach: spacing.focus-offset + spacing.stroke.focus

// A page with no row that takes the focus is one stop: a bare box in the frame that draws the ring round the page's rows when it
// is focused and nothing otherwise. It has the box of its page's rows while that page shows and none while another does.
struct Stop {
    lv_obj_t *obj;
    int page;
};

struct State {
    ScrollIndicator indicator;
    int32_t per;
    int32_t pitch;  // a row and the gap under it
    int32_t view;   // what a page holds, px: `per` rows of the nominal height and the gaps between them
    std::vector<int> first_row;  // each page's first row, by index: the whole rows that fit the view
    int page;
    lv_obj_t *frame;
    lv_obj_t *words;  // "1 of 2" under the view, or nullptr when the screen did not ask for it
    std::vector<Stop> stops;
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

// The stop of the page that shows takes the box of its rows, from the first row's top to the last row's bottom and as wide as
// they are, in the frame's own coordinates, so the ring stands round the facts as a row's stands round it; every other stop has
// no box, so nothing of it is ever in view. `first` is the page's first row, scrolled to the top of the frame by now.
void ShowStops(lv_obj_t *band, lv_obj_t *first, int page)
{
    State *s = Of(band);
    for (const Stop &stop : s->stops) {
        if (stop.page != page || first == nullptr) {
            lv_obj_set_size(stop.obj, 0, 0);
            continue;
        }
        const size_t next = static_cast<size_t>(page) + 1;
        lv_obj_t *last = lv_obj_get_child(band, next < s->first_row.size() ? s->first_row[next] - 1
                                                                            : static_cast<int32_t>(lv_obj_get_child_count(band)) - 1);
        lv_area_t frame, top, bottom;
        lv_obj_get_coords(s->frame, &frame);
        lv_obj_get_coords(first, &top);
        lv_obj_get_coords(last, &bottom);
        lv_obj_set_pos(stop.obj, top.x1 - frame.x1, top.y1 - frame.y1);
        lv_obj_set_size(stop.obj, lv_area_get_width(&top), bottom.y2 - top.y1 + 1);
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
        // A page shows only its own rows: the band is cut to the page's last row and the ring's reach under it, so the next
        // page's first row never peeks into the room a tall row left (DESIGN → Layout → Paging).
        const size_t next = static_cast<size_t>(page) + 1;
        lv_obj_t *last = lv_obj_get_child(band, next < s->first_row.size() ? s->first_row[next] - 1
                                                                            : static_cast<int32_t>(lv_obj_get_child_count(band)) - 1);
        const int32_t rows = lv_obj_get_y(last) + lv_obj_get_height(last) - lv_obj_get_y(first);
        lv_obj_set_height(band, std::min(rows, s->view) + 2 * kReach);
        lv_obj_update_layout(band);
        lv_obj_scroll_to_y(band, lv_obj_get_y(first), LV_ANIM_OFF);
    }
    const int32_t whole = s->view + kGap;
    UpdateScrollIndicator(s->indicator, page, pages, whole, pages * whole);
    if (s->words != nullptr) {
        char words[32];
        std::snprintf(words, sizeof words, "%d of %d", page + 1, pages);
        lv_label_set_text(s->words, words);
    }
    ShowStops(band, first, page);
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

// A stop that takes the focus turns to its page, as a row on another page does.
void OnStopFocused(lv_event_t *e)
{
    lv_obj_t *band = static_cast<lv_obj_t *>(lv_event_get_user_data(e));
    const State *s = Of(band);
    const lv_obj_t *target = lv_event_get_target_obj(e);
    for (const Stop &stop : s->stops) {
        if (stop.obj == target && stop.page != s->page) {
            ShowPage(band, stop.page, true);
            return;
        }
    }
}

// What takes the focus on a page: its first row that is in a focus group, else the page's stop; nullptr for a page of
// facts with no stop (a row that never joined a group).
lv_obj_t *FocusOnPage(lv_obj_t *band, int page)
{
    const State *s = Of(band);
    const int32_t n = static_cast<int32_t>(lv_obj_get_child_count(band));
    const size_t next = static_cast<size_t>(page) + 1;
    const int32_t end = next < s->first_row.size() ? s->first_row[next] : n;
    for (int32_t i = s->first_row[static_cast<size_t>(page)]; i < end; ++i) {
        lv_obj_t *row = lv_obj_get_child(band, i);
        if (lv_obj_get_group(row) != nullptr) {
            return row;
        }
    }
    for (const Stop &stop : s->stops) {
        if (stop.page == page) {
            return stop.obj;
        }
    }
    return nullptr;
}

// Whether `focused` is something of this list: one of its rows or one of its stops.
bool IsOfList(lv_obj_t *band, lv_obj_t *focused)
{
    if (lv_obj_get_parent(focused) == band) {
        return true;
    }
    for (const Stop &stop : Of(band)->stops) {
        if (stop.obj == focused) {
            return true;
        }
    }
    return false;
}

// A swipe turns one page; the focus, if it was in the list, goes to the new page's first stop: its first row that takes focus, or
// the page's own stop.
void TurnPage(lv_obj_t *band, int32_t dir)
{
    State *s = Of(band);
    const int page = std::clamp(s->page + static_cast<int>(dir), 0, Pages(band) - 1);
    if (page == s->page) {
        // At an end: the contact's tick answered the touch; at the lift, the at-end sound says no page is left, and nothing
        // wraps under a finger.
        lvgl_display::PlayCue(lvgl_display::Cue::kAtEnd);
        return;
    }
    // The list's own group, the screen's or an overlay's (a select sheet's list): that of a row or a stop.
    lv_group_t *group = nullptr;
    const int32_t n = static_cast<int32_t>(lv_obj_get_child_count(band));
    for (int32_t i = 0; i < n && group == nullptr; ++i) {
        group = lv_obj_get_group(lv_obj_get_child(band, i));
    }
    for (size_t i = 0; i < s->stops.size() && group == nullptr; ++i) {
        group = lv_obj_get_group(s->stops[i].obj);
    }
    lv_obj_t *focused = group != nullptr ? lv_group_get_focused(group) : nullptr;
    const bool keep = focused != nullptr && IsOfList(band, focused);
    ShowPage(band, page, true);
    if (keep) {
        if (lv_obj_t *target = FocusOnPage(band, page); target != nullptr) {
            lv_group_focus_obj(target);
        }
    }
}

void OnDelete(lv_event_t *e)
{
    delete Of(lv_event_get_target_obj(e));
}

int IndexInGroup(lv_group_t *group, const lv_obj_t *obj)
{
    const uint32_t n = lv_group_get_obj_count(group);
    for (uint32_t i = 0; i < n; ++i) {
        if (lv_group_get_obj_by_index(group, i) == obj) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// Moves `stop`, just added at the end of the group, to the walk's place for the page whose rows are [first, end): before the first row
// that takes focus on a later page, else after the last one that does on an earlier page or the stop of an earlier page — the
// focus order is the page order, with the stop where its facts stand among the rows. With no row of the list in the group, it
// goes where the list's first stop stood before this call (`kept`, its index in the group), so a list of facts only keeps its
// place when its stops are made anew; the very first time it stays where it was added, before what follows it in the walk.
void PlaceStop(lv_obj_t *band, lv_obj_t *stop, int32_t first, int32_t end, int kept)
{
    lv_group_t *group = lv_obj_get_group(stop);
    if (group == nullptr) {
        return;
    }
    const State *s = Of(band);
    const int32_t n = static_cast<int32_t>(lv_obj_get_child_count(band));
    int target = -1;
    for (int32_t i = end; i < n && target < 0; ++i) {
        lv_obj_t *row = lv_obj_get_child(band, i);
        if (lv_obj_get_group(row) == group) {
            target = IndexInGroup(group, row);
        }
    }
    if (target < 0) {
        int last = -1;
        for (int32_t i = 0; i < first; ++i) {
            lv_obj_t *row = lv_obj_get_child(band, i);
            if (lv_obj_get_group(row) == group) {
                last = std::max(last, IndexInGroup(group, row));
            }
        }
        for (const Stop &earlier : s->stops) {
            if (earlier.obj != stop && lv_obj_get_group(earlier.obj) == group) {
                last = std::max(last, IndexInGroup(group, earlier.obj));
            }
        }
        target = last >= 0 ? last + 1 : kept;
    }
    // Bubbles it down one place at a time: LVGL has a swap and no insert, and a swap keeps the focus on the same object.
    const int at = IndexInGroup(group, stop);
    for (int k = at - 1; target >= 0 && k >= target; --k) {
        lv_group_swap_obj(stop, lv_group_get_obj_by_index(group, static_cast<uint32_t>(k)));
    }
}

// One stop for each page in which no row takes the focus, in page order. A page with a row that does has none of its own: its
// facts show with it, and the ring goes to its rows.
void BuildStops(lv_obj_t *band)
{
    State *s = Of(band);
    // The old stops leave the list first: deleting a focused one hands the focus to another, whose handler reads this vector. The
    // place of the first, in its group, is kept for a list that has no row to stand by.
    const std::vector<Stop> old = std::move(s->stops);
    s->stops.clear();
    int kept = -1;
    for (const Stop &stop : old) {
        if (kept < 0 && lv_obj_get_group(stop.obj) != nullptr) {
            kept = IndexInGroup(lv_obj_get_group(stop.obj), stop.obj);
        }
    }
    for (const Stop &stop : old) {
        lv_obj_delete(stop.obj);
    }
    const int32_t n = static_cast<int32_t>(lv_obj_get_child_count(band));
    for (size_t p = 0; p < s->first_row.size(); ++p) {
        const int32_t first = s->first_row[p];
        const int32_t end = p + 1 < s->first_row.size() ? s->first_row[p + 1] : n;
        bool takes_focus = false;
        for (int32_t i = first; i < end && !takes_focus; ++i) {
            takes_focus = lv_obj_get_group(lv_obj_get_child(band, i)) != nullptr;
        }
        if (takes_focus) {
            continue;
        }
        lv_obj_t *stop = detail::Bare(s->frame);
        lv_obj_set_size(stop, 0, 0);
        lv_obj_remove_flag(stop, LV_OBJ_FLAG_SCROLL_WITH_ARROW);  // no key scrolls it: UP and DOWN leave it, turning the page
        lv_obj_set_style_radius(stop, dt::kRoundedLg, 0);         // the ring follows the rows' own corners
        ui::RingOutside(stop);
        detail::JoinGroup(stop, s->frame);  // the page brings the view, and the ring round it, into view
        s->stops.push_back({stop, static_cast<int>(p)});
        PlaceStop(band, stop, first, end, kept);
        lv_obj_add_event_cb(stop, OnStopFocused, LV_EVENT_FOCUSED, band);
    }
}
}  // namespace

ListView MakeListView(lv_obj_t *parent, int32_t rows_per_page, int32_t row_height, IndicatorCentre centre, PageWords words)
{
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
    lv_obj_remove_flag(view.root, LV_OBJ_FLAG_SCROLL_WITH_ARROW);  // the page turns by the focus and a swipe; no key scrolls the band
    lv_obj_set_size(view.root, dt::kSpacingContent + 2 * kReach, height + 2 * kReach);
    lv_obj_set_pos(view.root, -kReach, -kReach);
    lv_obj_set_flex_flow(view.root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(view.root, kReach, 0);
    lv_obj_set_style_pad_row(view.root, kGap, 0);
    detail::ClaimSwipe(view.root, TurnPage);
    view.indicator = MakeScrollIndicator(view.frame, height, centre);
    // "1 of 2" alone says the page: `caption`, `text-secondary`, at the rows' left edge and `spacing.md` under the view's whole
    // height, so it stands in one place on every page. It shows while there is more than one page.
    if (words == PageWords::kUnder) {
        view.words = detail::Line(view.frame, ui::Type::kCaption, "", true);
        lv_obj_set_pos(view.words, 0, height + kGap);
        lv_obj_add_flag(view.words, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_set_user_data(view.root, new State{view.indicator, rows_per_page, row_height + kGap, height, {}, 0, view.frame, view.words, {}});
    lv_obj_add_event_cb(view.root, OnDelete, LV_EVENT_DELETE, nullptr);
    return view;
}

void UpdateListView(const ListView &view)
{
    lv_obj_t *band = view.root;
    State *s = Of(band);
    const uint32_t n = lv_obj_get_child_count(band);
    // The last page is as tall as the others, so the band never scrolls back by itself to fill the glass: the room under its
    // last row is what the page's first row's top and the view leave.
    Repage(band);
    BuildStops(band);
    for (uint32_t i = 0; i < n; ++i) {
        lv_obj_t *row = lv_obj_get_child(band, static_cast<int32_t>(i));
        ui::ShowWhenFocused(row, view.frame);  // the list's page turns, not a scroll by the row; the page behind brings the list in
        lv_obj_remove_event_cb_with_user_data(row, OnRowFocused, band);
        lv_obj_add_event_cb(row, OnRowFocused, LV_EVENT_FOCUSED, band);
    }
    int32_t spare = 0;
    if (n > 0) {
        lv_obj_t *first = lv_obj_get_child(band, s->first_row.back());
        lv_obj_t *last = lv_obj_get_child(band, static_cast<int32_t>(n) - 1);
        spare = std::max<int32_t>(0, s->view - (lv_obj_get_y(last) + lv_obj_get_height(last) - lv_obj_get_y(first)));
    }
    lv_obj_set_style_pad_bottom(band, dt::kSpacingSm + spare, 0);
    // The words take their line under the view only when there is a second page to say.
    if (s->words != nullptr) {
        const bool several = s->first_row.size() > 1;
        if (several) {
            lv_obj_remove_flag(s->words, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s->words, LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_set_height(view.frame, s->view + (several ? kGap + ui::LineBox(ui::Type::kCaption) : 0));
    }
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
