#include "widgets/scroll_indicator.h"

#include <algorithm>

#include "design_tokens.h"
#include "internal/base.h"
#include "internal/scroll_rules.h"

namespace widgets {
namespace dt = design_tokens;

namespace {
constexpr int32_t kStroke = dt::kSpacingScrollThumb;  // the lane's width, and the shortest piece: a dot
constexpr int32_t kGap = dt::kSpacingSm;              // of white between one piece and the next, cap to cap

// One piece of the stroke from `from` to `to`; hidden when it is shorter than a dot.
void SetSpan(lv_obj_t *piece, int32_t from, int32_t to)
{
    if (to - from < kStroke) {
        lv_obj_add_flag(piece, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_obj_set_pos(piece, 0, from);
    lv_obj_set_size(piece, kStroke, to - from);
    lv_obj_remove_flag(piece, LV_OBJ_FLAG_HIDDEN);
}

// The lane's right edge: `spacing.scroll-x` + its width on the glass, the host's right edge at the content column's.
constexpr int32_t kBeyondHost = dt::kSpacingScrollX + kStroke - (dt::kSpacingEdge + dt::kSpacingContent);

// LVGL clips a child to its parent's area plus the parent's ext draw size, whatever the overflow: the lane stands past the
// host's right edge by more than a focus ring reaches, so the host's reach is the lane's.
void LaneReachCb(lv_event_t *e)
{
    lv_event_set_ext_draw_size(e, kBeyondHost);
}
}  // namespace

ScrollIndicator MakeScrollIndicator(lv_obj_t *host, int32_t band_height, IndicatorCentre centre)
{
    ScrollIndicator indicator{};
    indicator.length = std::min<int32_t>(dt::kSpacingScrollLength, band_height);
    indicator.lane = detail::Bare(host);
    lv_obj_add_flag(indicator.lane, LV_OBJ_FLAG_FLOATING);  // outside the host's layout
    lv_obj_set_size(indicator.lane, kStroke, indicator.length);
    int32_t top = (band_height - indicator.length) / 2;  // centred on the host
    if (centre == IndicatorCentre::kGlass) {
        lv_obj_update_layout(host);
        lv_area_t at;
        lv_obj_get_coords(host, &at);
        top = (lv_display_get_vertical_resolution(lv_obj_get_display(host)) - indicator.length) / 2 - at.y1;
    }
    lv_obj_align(indicator.lane, LV_ALIGN_TOP_RIGHT, kBeyondHost, top);
    lv_obj_add_event_cb(host, LaneReachCb, LV_EVENT_REFR_EXT_DRAW_SIZE, nullptr);
    lv_obj_refresh_ext_draw_size(host);
    indicator.before = detail::Pill(indicator.lane);
    indicator.after = detail::Pill(indicator.lane);
    indicator.thumb = detail::Pill(indicator.lane);
    lv_obj_set_style_bg_color(indicator.before, ui::Fill(dt::kColorBand), 0);
    lv_obj_set_style_bg_color(indicator.after, ui::Fill(dt::kColorBand), 0);
    lv_obj_set_style_bg_color(indicator.thumb, ui::Level(dt::kColorSurfaceOn), 0);
    lv_obj_add_flag(indicator.lane, LV_OBJ_FLAG_HIDDEN);
    return indicator;
}

void UpdateScrollIndicator(const ScrollIndicator &indicator, int32_t page, int32_t pages, int32_t view, int32_t content)
{
    if (content <= view || view <= 0 || pages <= 1) {
        lv_obj_add_flag(indicator.lane, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_obj_remove_flag(indicator.lane, LV_OBJ_FLAG_HIDDEN);
    const int32_t whole = indicator.length;
    // The thumb is the view's share of the length, and travels the rest as the pages go from the first to the last; where it
    // stands for a page is detail::ThumbAt's rule (the ends held, a crowded list's pages spread evenly).
    const int32_t length = std::max<int32_t>(kStroke, static_cast<int32_t>(static_cast<int64_t>(whole) * view / content));
    const int32_t room = whole - length;
    const int32_t at = detail::ThumbAt(room, kGap + kStroke, std::clamp<int32_t>(page, 0, pages - 1), pages);
    SetSpan(indicator.before, 0, at - kGap);
    SetSpan(indicator.thumb, at, at + length);
    SetSpan(indicator.after, at + length + kGap, whole);
}

}  // namespace widgets
