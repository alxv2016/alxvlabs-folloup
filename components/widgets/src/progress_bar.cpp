#include "widgets/progress_bar.h"

#include <algorithm>

#include "design_tokens.h"
#include "internal/base.h"

namespace widgets {
namespace dt = design_tokens;

ProgressBar MakeProgressBar(lv_obj_t *parent, int32_t width, bool lead)
{
    ProgressBar bar{};
    bar.width = width;
    bar.stroke = lead ? dt::kSpacingProgressLead : dt::kSpacingProgress;
    bar.root = detail::Bare(parent);
    lv_obj_set_size(bar.root, width, bar.stroke);
    bar.track = detail::Pill(bar.root);
    lv_obj_set_style_bg_color(bar.track, ui::Fill(dt::kColorBand), 0);
    bar.fill = detail::Pill(bar.root);
    lv_obj_set_style_bg_color(bar.fill, ui::Level(dt::kColorSurfaceAction), 0);
    SetProgress(bar, 0);
    return bar;
}

namespace {
void Show(lv_obj_t *piece, int32_t from, int32_t to, int32_t stroke)
{
    lv_obj_set_pos(piece, from, 0);
    lv_obj_set_size(piece, to - from, stroke);
    lv_obj_remove_flag(piece, LV_OBJ_FLAG_HIDDEN);
}
}  // namespace

void SetProgress(const ProgressBar &bar, int32_t percent)
{
    percent = std::clamp<int32_t>(percent, 0, 100);
    const int32_t length = bar.width;
    const int32_t dot = bar.stroke;
    const int32_t gap = dt::kSpacingSm;
    const int32_t room = std::max(dot, length - gap - dot);
    constexpr int32_t kSteps = 99;  // 100 values: 0 is the empty bar, 99 the last with track, 100 one step more
    if (percent >= 100) {
        Show(bar.fill, 0, room * 100 / kSteps, dot);
        lv_obj_add_flag(bar.track, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    if (percent <= 0) {
        lv_obj_add_flag(bar.fill, LV_OBJ_FLAG_HIDDEN);
        Show(bar.track, 0, length, dot);
        return;
    }
    const int32_t floor_end = std::max(dot, room / kSteps);
    const int32_t end = std::clamp(room * percent / kSteps, floor_end, std::max(floor_end, room));
    Show(bar.fill, 0, end, dot);
    if (end + gap + dot <= length) {
        Show(bar.track, end + gap, length, dot);
    } else {
        lv_obj_add_flag(bar.track, LV_OBJ_FLAG_HIDDEN);
    }
}

}  // namespace widgets
