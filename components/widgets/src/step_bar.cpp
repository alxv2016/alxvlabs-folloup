#include "widgets/step_bar.h"

#include <algorithm>

#include "design_tokens.h"
#include "internal/base.h"

namespace widgets {
namespace dt = design_tokens;

StepBar MakeStepBar(lv_obj_t *parent, int32_t width, int count)
{
    StepBar bar{};
    bar.count = count > kStepMax ? kStepMax : count < 1 ? 1 : count;
    bar.width = width;
    bar.root = detail::Bare(parent);
    lv_obj_set_size(bar.root, width, dt::kSpacingProgress);
    for (int i = 0; i < bar.count; ++i) {
        bar.track[i] = detail::Pill(bar.root);
        lv_obj_set_style_bg_color(bar.track[i], ui::Fill(dt::kColorBand), 0);
        bar.fill[i] = detail::Pill(bar.root);
        lv_obj_set_style_bg_color(bar.fill[i], ui::Level(dt::kColorSurfaceAction), 0);
        SetStep(bar, i, StepState::kWaiting);
    }
    return bar;
}

namespace {
void Show(lv_obj_t *piece, int32_t from, int32_t to)
{
    lv_obj_set_pos(piece, from, 0);
    lv_obj_set_size(piece, to - from, dt::kSpacingProgress);
    lv_obj_remove_flag(piece, LV_OBJ_FLAG_HIDDEN);
}
}  // namespace

void SetStep(const StepBar &bar, int index, StepState state, int32_t permille)
{
    if (index < 0 || index >= bar.count) {
        return;
    }
    const int32_t gap = dt::kSpacingStepGap;
    const int32_t dot = dt::kSpacingProgress;
    // Piece i of n: its share of the length, the gaps between.
    const int32_t from = (bar.width + gap) * index / bar.count;
    const int32_t to = (bar.width + gap) * (index + 1) / bar.count - gap;
    const int32_t seg = to - from;
    lv_obj_t *fill = bar.fill[index];
    lv_obj_t *track = bar.track[index];
    // A failed step is hollow: a white pill inside a solid outline, never the dither a word could sit on.
    const bool failed = state == StepState::kFailed;
    lv_obj_set_style_bg_color(fill, ui::Level(failed ? dt::kColorBackground : dt::kColorSurfaceAction), 0);
    lv_obj_set_style_border_width(fill, failed ? dt::kSpacingStrokeDefault : 0, 0);
    lv_obj_set_style_border_color(fill, ui::Level(dt::kColorOutline), 0);
    lv_obj_set_style_border_opa(fill, LV_OPA_COVER, 0);
    switch (state) {
    case StepState::kWaiting:
        lv_obj_add_flag(fill, LV_OBJ_FLAG_HIDDEN);
        Show(track, from, to);
        break;
    case StepState::kRunning: {
        const int32_t most = std::max(dot, seg - dt::kSpacingSm - dot);
        const int32_t reach = std::clamp((seg * std::clamp<int32_t>(permille, 0, 1000) + 500) / 1000, dot, most);
        Show(fill, from, from + reach);
        Show(track, from + reach + dt::kSpacingSm, to);
        break;
    }
    case StepState::kDone:
    case StepState::kFailed:
        Show(fill, from, to);
        lv_obj_add_flag(track, LV_OBJ_FLAG_HIDDEN);
        break;
    }
}

}  // namespace widgets
