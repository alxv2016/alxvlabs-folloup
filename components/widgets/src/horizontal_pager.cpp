#include "widgets/horizontal_pager.h"

#include <algorithm>
#include <cstdio>
#include <vector>

#include "design_tokens.h"
#include "internal/base.h"
#include "lvgl_display.h"

namespace widgets {
namespace dt = design_tokens;

namespace {
constexpr int32_t kReach = dt::kSpacingSm;                              // the ring's reach: spacing.focus-offset + spacing.stroke.focus
constexpr int32_t kView = dt::kSpacingContent + 2 * kReach;             // the band, and the distance from one page's start to the next's
constexpr int32_t kGap = dt::kSpacingMd;                                // between items

struct State {
    lv_obj_t *frame;
    lv_obj_t *words_row;
    lv_obj_t *words;               // "1 of 2", or nullptr while everything fits one page
    std::vector<int> first_item;   // each page's first item, by index
    int page;
};

State *Of(lv_obj_t *band)
{
    return static_cast<State *>(lv_obj_get_user_data(band));
}

int Pages(lv_obj_t *band)
{
    return static_cast<int>(std::max<size_t>(1, Of(band)->first_item.size()));
}

int PageOfItem(const State *s, int index)
{
    int page = 0;
    for (size_t p = 0; p < s->first_item.size(); ++p) {
        if (s->first_item[p] <= index) {
            page = static_cast<int>(p);
        }
    }
    return page;
}

// The page lands at once: the band scrolled to its start, the words saying which page it is.
void ShowPage(lv_obj_t *band, int page, bool tell)
{
    State *s = Of(band);
    page = std::clamp(page, 0, Pages(band) - 1);
    lv_obj_scroll_to_x(band, page * kView, LV_ANIM_OFF);
    if (s->words != nullptr) {
        char words[24];
        std::snprintf(words, sizeof words, "%d of %d", page + 1, Pages(band));
        lv_label_set_text(s->words, words);
    }
    const bool changed = page != s->page;
    s->page = page;
    if (changed && tell) {
        lv_obj_send_event(band, LV_EVENT_VALUE_CHANGED, nullptr);
    }
}

// An item that takes the focus on another page turns to it: an item to the right becomes the page's first, one to the left
// its last.
void OnItemFocused(lv_event_t *e)
{
    lv_obj_t *band = static_cast<lv_obj_t *>(lv_event_get_user_data(e));
    const int page = PageOfItem(Of(band), static_cast<int>(lv_obj_get_index(lv_event_get_target_obj(e))));
    if (page != Of(band)->page) {
        ShowPage(band, page, true);
    }
}

// A swipe turns one page; the focus, if it was in the strip, goes to the new page's first item.
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
    // The items' own group, the screen's or an overlay's.
    lv_obj_t *item = lv_obj_get_child_count(band) > 0 ? lv_obj_get_child(band, 0) : nullptr;
    lv_group_t *group = item != nullptr ? lv_obj_get_group(item) : nullptr;
    lv_obj_t *focused = group != nullptr ? lv_group_get_focused(group) : nullptr;
    ShowPage(band, page, true);
    if (focused != nullptr && lv_obj_get_parent(focused) == band) {
        lv_group_focus_obj(lv_obj_get_child(band, s->first_item[static_cast<size_t>(page)]));
    }
}

void OnDelete(lv_event_t *e)
{
    delete Of(lv_event_get_target_obj(e));
}
}  // namespace

HorizontalPager MakeHorizontalPager(lv_obj_t *parent)
{
    HorizontalPager pager{};
    pager.frame = detail::Bare(parent);
    lv_obj_set_size(pager.frame, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(pager.frame, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(pager.frame, dt::kSpacingMd, 0);
    lv_obj_set_style_pad_top(pager.frame, kReach, 0);  // the band's reach above the chips, as the draft's strip stands 8 px under its top
    detail::LetRingOut(pager.frame);

    lv_obj_t *strip = detail::Bare(pager.frame);
    lv_obj_set_size(strip, lv_pct(100), dt::kSpacingTarget);
    detail::LetRingOut(strip);
    // The band, wider and taller than the items by the ring's reach all round: it clips what it holds, and the ring is inside it.
    pager.root = lv_obj_create(strip);
    lv_obj_remove_style_all(pager.root);
    lv_obj_set_size(pager.root, kView, dt::kSpacingTarget + 2 * kReach);
    lv_obj_set_pos(pager.root, -kReach, -kReach);
    lv_obj_set_flex_flow(pager.root, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(pager.root, kReach, 0);
    lv_obj_set_style_pad_column(pager.root, kGap, 0);
    detail::ClaimSwipe(pager.root, TurnPage, detail::SwipeAxis::kHorizontal);

    // "1 of 2" alone says the page: `caption`, `text-secondary`, at the strip's left edge, one line box high, under the strip.
    pager.words_row = detail::Bare(pager.frame);
    lv_obj_set_size(pager.words_row, lv_pct(100), ui::LineBox(ui::Type::kCaption));
    lv_obj_set_flex_flow(pager.words_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(pager.words_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(pager.words_row, LV_OBJ_FLAG_HIDDEN);

    lv_obj_set_user_data(pager.root, new State{pager.frame, pager.words_row, nullptr, {}, 0});
    lv_obj_add_event_cb(pager.root, OnDelete, LV_EVENT_DELETE, nullptr);
    return pager;
}

void UpdateHorizontalPager(const HorizontalPager &pager)
{
    lv_obj_t *band = pager.root;
    State *s = Of(band);
    const int32_t n = static_cast<int32_t>(lv_obj_get_child_count(band));

    // Pages: each takes the whole items that fit the content column, and starts a view's width after the last one's start, so
    // an item of the next page never shows at the band's edge — a cut word is a clipped word, and half a chip half a hit area.
    for (int32_t i = 0; i < n; ++i) {
        lv_obj_set_style_margin_left(lv_obj_get_child(band, i), 0, 0);
    }
    lv_obj_update_layout(band);
    s->first_item.clear();
    int32_t flow_end = kReach;  // the right edge of the last item placed, in the band's own coordinates
    int32_t page_start = 0;
    for (int32_t i = 0; i < n; ++i) {
        lv_obj_t *item = lv_obj_get_child(band, i);
        const int32_t width = lv_obj_get_width(item);
        const int32_t at = flow_end + (i > 0 ? kGap : 0);
        const bool starts_page = s->first_item.empty() || at + width - page_start > dt::kSpacingContent;
        if (starts_page) {
            const int page = static_cast<int>(s->first_item.size());
            s->first_item.push_back(static_cast<int>(i));
            page_start = page * kView + kReach;
            lv_obj_set_style_margin_left(item, page_start - at, 0);
            flow_end = page_start + width;
        } else {
            flow_end = at + width;
        }
    }
    // The last page is as wide as the others, so the band never scrolls back by itself to fill the view.
    const int32_t pages = static_cast<int32_t>(std::max<size_t>(1, s->first_item.size()));
    lv_obj_set_style_pad_right(band, std::max<int32_t>(kReach, pages * kView - flow_end), 0);

    // The items turn the pager's page and not the band by a pixel: the page behind brings the whole pager into view.
    for (int32_t i = 0; i < n; ++i) {
        lv_obj_t *item = lv_obj_get_child(band, i);
        ui::ShowWhenFocused(item, pager.frame);
        lv_obj_remove_event_cb_with_user_data(item, OnItemFocused, band);
        lv_obj_add_event_cb(item, OnItemFocused, LV_EVENT_FOCUSED, band);
    }

    // The count in words: figures keep their width, so a turn redraws the same box.
    lv_obj_clean(s->words_row);
    s->words = nullptr;
    if (pages > 1) {
        s->words = detail::Line(s->words_row, ui::Type::kCaption, "", true);
        lv_obj_remove_flag(s->words_row, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s->words_row, LV_OBJ_FLAG_HIDDEN);
    }
    s->page = 0;
    ShowPage(band, 0, false);
}

int HorizontalPagerPage(const HorizontalPager &pager)
{
    return Of(pager.root)->page;
}

int HorizontalPagerPages(const HorizontalPager &pager)
{
    return Pages(pager.root);
}

}  // namespace widgets
