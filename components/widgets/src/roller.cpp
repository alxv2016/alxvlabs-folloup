#include "widgets/roller.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>

#include "design_tokens.h"
#include "internal/base.h"
#include "internal/roller_rules.h"
#include "lvgl_display.h"
#include "router/router.h"
#include "widgets/button.h"

namespace widgets {
namespace dt = design_tokens;
namespace rr = detail::roller;

namespace {
constexpr int32_t kRow = dt::kSpacingTarget;                              // each of the three rows
constexpr int32_t kBandInset = dt::kSpacingStrokeHairline + dt::kSpacingXs;  // the open band, in from the outline and a gap

struct State {
    int count;
    int index;
    bool circular;
    int first, step, digits;
    const char *(*label)(int index, char *buf, size_t size, void *user);
    void *user;
    rr::Edit edit;
    lv_obj_t *rows[rr::kRows];
    lv_obj_t *band;
    lv_obj_t *bar;  // the footer's bar for open rollers, or nullptr
};

State *Of(lv_obj_t *root)
{
    return static_cast<State *>(lv_obj_get_user_data(root));
}

// The text of value `index`, in `buf`.
const char *TextOf(const State *s, int index, char *buf, size_t size)
{
    if (s->label != nullptr) {
        const char *text = s->label(index, buf, size, s->user);
        return text != nullptr ? text : "";
    }
    std::snprintf(buf, size, "%0*d", s->digits, s->first + index * s->step);
    return buf;
}

// Draws the three rows for the shown value: the value above, the value, the value below; a blank row past the end of a list that
// does not wrap. The open band stays where it is and only the words change, so a step repaints the digits and nothing else.
void Draw(lv_obj_t *root)
{
    State *s = Of(root);
    const bool open = s->edit.open;
    char buf[32];
    for (int r = 0; r < rr::kRows; ++r) {
        const int at = rr::Neighbour(s->index, s->count, r - rr::kMiddle, s->circular);
        lv_label_set_text(s->rows[r], at < 0 ? "" : TextOf(s, at, buf, sizeof buf));
    }
    lv_obj_set_style_text_color(s->rows[rr::kMiddle], ui::Level(open ? dt::kColorTextOnFill : dt::kColorTextPrimary), 0);
    if (open) {
        lv_obj_remove_flag(s->band, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s->band, LV_OBJ_FLAG_HIDDEN);
    }
}

// The flag that marks a footer part this widget hid, so closing shows only those and leaves a part that was hidden before as it was.
constexpr lv_obj_flag_t kHiddenByRoller = LV_OBJ_FLAG_USER_2;

// The footer while open: the bar in its place and the footer's other parts hidden, and back as they were when it closes.
void ShowFooter(lv_obj_t *root, bool open)
{
    State *s = Of(root);
    if (s->bar == nullptr) {
        return;
    }
    lv_obj_t *footer = lv_obj_get_parent(s->bar);
    for (uint32_t i = 0; i < lv_obj_get_child_count(footer); ++i) {
        lv_obj_t *part = lv_obj_get_child(footer, static_cast<int32_t>(i));
        if (part == s->bar) {
            continue;
        }
        if (open) {
            if (!lv_obj_has_flag(part, LV_OBJ_FLAG_HIDDEN)) {
                lv_obj_add_flag(part, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(part, kHiddenByRoller);
            }
        } else if (lv_obj_has_flag(part, kHiddenByRoller)) {
            lv_obj_remove_flag(part, kHiddenByRoller);
            lv_obj_remove_flag(part, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (open) {
        lv_obj_remove_flag(s->bar, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_user_data(s->bar, root);  // the roller Cancel puts back
    } else {
        lv_obj_add_flag(s->bar, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_user_data(s->bar, nullptr);
    }
}

// One step, `dir` -1 for UP and +1 for DOWN. A step into the end of a list that does not wrap changes nothing and says so.
void StepBy(lv_obj_t *root, int dir)
{
    State *s = Of(root);
    const rr::Move move = rr::Step(s->index, s->count, dir, s->circular);
    if (!move.moved) {
        lvgl_display::PlayCue(lvgl_display::Cue::kAtEnd);
        return;
    }
    s->index = move.index;
    Draw(root);
}

// While open, UP and DOWN are the roller's own; each repeat of a held key steps again.
void OnKey(lv_event_t *e)
{
    lv_obj_t *root = lv_event_get_target_obj(e);
    if (!Of(root)->edit.open) {
        return;
    }
    const uint32_t key = lv_event_get_key(e);
    if (key == LV_KEY_UP || key == LV_KEY_LEFT) {
        StepBy(root, -1);
    } else if (key == LV_KEY_DOWN || key == LV_KEY_RIGHT) {
        StepBy(root, +1);
    }
}

// OK opened it, or a tap did, or it closed — OK keeping, a tap on the middle row, or a press that took the focus elsewhere. The
// group's editing state is the one truth: LVGL puts LV_STATE_EDITED on the focused control while the group edits.
void OnStateChanged(lv_event_t *e)
{
    lv_obj_t *root = lv_event_get_target_obj(e);
    State *s = Of(root);
    const bool was = (lv_event_get_prev_state(e) & LV_STATE_EDITED) != 0;
    const bool now = lv_obj_has_state(root, LV_STATE_EDITED);
    if (was == now) {
        return;
    }
    if (now) {
        rr::Open(s->edit, s->index);
        Draw(root);
        ShowFooter(root, true);
    } else {
        const bool changed = rr::Keep(s->edit, s->index);  // a cancel has put the value back and closed the edit already
        Draw(root);
        ShowFooter(root, false);
        if (changed) {
            lv_obj_send_event(root, LV_EVENT_VALUE_CHANGED, nullptr);
        }
    }
}

// Holding OK while open, or the footer's Cancel: the value it opened on. The group's editing ends after this (router::Escape, the
// Cancel's click), and closing then reports nothing.
void OnCancel(lv_event_t *e)
{
    lv_obj_t *root = lv_event_get_target_obj(e);
    State *s = Of(root);
    if (!s->edit.open) {
        return;
    }
    s->index = rr::Cancel(s->edit);
    Draw(root);
}

// A tap, which only a finger makes here: OK's click is the shared edit toggle's. The row it lands in says what it does.
void OnClicked(lv_event_t *e)
{
    lv_indev_t *indev = lv_indev_active();
    if (detail::FromKeys() || indev == nullptr || lv_indev_get_scroll_obj(indev) != nullptr) {
        return;
    }
    lv_obj_t *root = lv_event_get_target_obj(e);
    State *s = Of(root);
    lv_group_t *group = lv_obj_get_group(root);
    if (group == nullptr) {
        return;
    }
    lv_point_t point;
    lv_indev_get_point(indev, &point);
    lv_area_t area;
    lv_obj_get_coords(root, &area);
    switch (rr::TapOn(s->edit.open, rr::RowAt(point.y - area.y1, kRow))) {
    case rr::Tap::kOpen:
        lv_group_focus_obj(root);  // the press did not take the focus (see MakeRoller), so the tap does, and then opens
        lv_group_set_editing(group, true);
        break;
    case rr::Tap::kStepUp:
        StepBy(root, -1);
        break;
    case rr::Tap::kStepDown:
        StepBy(root, +1);
        break;
    case rr::Tap::kKeep:
        lv_group_set_editing(group, false);
        break;
    }
}

// One swipe, one step, on the lift: up moves the list up, so the value below comes to the middle. Only an open roller takes it.
void OnSwipe(lv_obj_t *root, int32_t dir)
{
    if (Of(root)->edit.open) {
        StepBy(root, dir > 0 ? +1 : -1);
    }
}

void OnDelete(lv_event_t *e)
{
    lv_obj_t *root = lv_event_get_target_obj(e);
    State *s = Of(root);
    if (s->bar != nullptr && lv_obj_get_user_data(s->bar) == root) {
        lv_obj_set_user_data(s->bar, nullptr);  // the footer's Cancel names no roller that is gone
    }
    router::Hold(&s->bar, nullptr);
    delete s;
}

// The text's width in px, for the widest value.
int32_t WidthOf(const lv_font_t *font, const char *text)
{
    int32_t width = 0;
    for (const unsigned char *c = reinterpret_cast<const unsigned char *>(text); *c != 0; ++c) {
        if ((*c & 0xC0) != 0x80) {  // a continuation byte is part of the glyph before it
            width += lv_font_get_glyph_width(font, *c < 0x80 ? *c : '0', 0);
        }
    }
    return width;
}

lv_obj_t *MakeCenteredLabel(lv_obj_t *parent, ui::Type role, int32_t width, int32_t y, bool secondary)
{
    lv_obj_t *label = ui::NewLabel(parent, role, "", secondary);
    ui::FitLineBox(label, role);
    lv_obj_set_width(label, width);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(label, 0, y + (kRow - ui::LineBox(role)) / 2);
    return label;
}
}  // namespace

Roller MakeRoller(lv_obj_t *parent, const RollerSpec &spec)
{
    Roller roller{};
    State *s = new State{};
    s->count = std::max(spec.count, 2);
    s->index = std::clamp(spec.selected, 0, s->count - 1);
    s->circular = spec.circular;
    s->first = spec.first;
    s->step = spec.step > 0 ? spec.step : 1;
    s->digits = spec.digits;
    s->label = spec.label;
    s->user = spec.user;

    // The width: the widest value and `spacing.lg` either side, at least the touch target.
    const lv_font_t *font = ui::Font(ui::Type::kLabel);
    int32_t widest = 0;
    char buf[32];
    for (int i = 0; i < s->count; ++i) {
        widest = std::max(widest, WidthOf(font, TextOf(s, i, buf, sizeof buf)));
    }
    const int32_t width = std::max<int32_t>(widest + 2 * dt::kSpacingLg, dt::kSpacingTarget);

    // The frame: the name over the roller, `spacing.md` apart.
    roller.frame = detail::Bare(parent);
    lv_obj_set_size(roller.frame, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(roller.frame, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(roller.frame, dt::kSpacingMd, 0);
    detail::LetRingOut(roller.frame);
    lv_obj_t *name = ui::NewLabel(roller.frame, ui::Type::kCaption, spec.name != nullptr ? spec.name : "", true);
    ui::FitLineBox(name, ui::Type::kCaption);
    lv_obj_set_width(name, LV_SIZE_CONTENT);

    // The roller: a white face, three rows, the outline drawn inside its size.
    roller.root = detail::Bare(roller.frame);
    lv_obj_set_size(roller.root, width, rr::kRows * kRow);
    lv_obj_add_flag(roller.root, LV_OBJ_FLAG_CLICKABLE);
    // LVGL's click focus ends an edit whenever its control is pressed, even the one already focused — a tap on a row of an open
    // roller would close it before the tap is read. So a press does not focus it: the tap does, once it is read (OnClicked).
    lv_obj_remove_flag(roller.root, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_remove_flag(roller.root, LV_OBJ_FLAG_SCROLL_WITH_ARROW);  // OnKey steps the value; LVGL's arrow scroll would also move the rows
    lv_obj_set_style_radius(roller.root, dt::kRoundedLg, 0);
    lv_obj_set_style_bg_opa(roller.root, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(roller.root, ui::Level(dt::kColorBackground), 0);
    detail::LetRingOut(roller.root);

    // The open band behind the middle row: `surface-chosen`, the chosen inversion — the value OK will keep.
    s->band = detail::Bare(roller.root);
    lv_obj_set_size(s->band, width - 2 * kBandInset, kRow);
    lv_obj_set_pos(s->band, kBandInset, kRow);
    lv_obj_set_style_radius(s->band, dt::kRoundedMd, 0);
    lv_obj_set_style_bg_opa(s->band, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(s->band, ui::Level(dt::kColorSurfaceChosen), 0);
    lv_obj_add_flag(s->band, LV_OBJ_FLAG_HIDDEN);

    for (int r = 0; r < rr::kRows; ++r) {
        const bool middle = r == rr::kMiddle;
        s->rows[r] = MakeCenteredLabel(roller.root, middle ? ui::Type::kLabel : ui::Type::kCaption, width, r * kRow, !middle);
    }

    // The edge: the outline over the face, in `spacing.stroke.hairline`, taking no touch.
    lv_obj_t *edge = detail::Bare(roller.root);
    lv_obj_add_flag(edge, LV_OBJ_FLAG_FLOATING);
    lv_obj_set_size(edge, width, rr::kRows * kRow);
    lv_obj_set_pos(edge, 0, 0);
    lv_obj_set_style_radius(edge, dt::kRoundedLg, 0);
    detail::Outline(edge, dt::kSpacingStrokeHairline);

    // Room to scroll: the swipe claims a drag only on an object that has somewhere to scroll to, and puts it back at once
    // (ClaimSwipe), so nothing moves; this transparent part is what the drag has to go to. It paints and takes nothing.
    lv_obj_t *room = detail::Bare(roller.root);
    lv_obj_set_size(room, 1, 2 * rr::kRows * kRow);
    lv_obj_set_pos(room, 0, 0);

    lv_obj_set_user_data(roller.root, s);
    lv_obj_add_event_cb(roller.root, OnDelete, LV_EVENT_DELETE, nullptr);
    lv_obj_add_event_cb(roller.root, OnKey, LV_EVENT_KEY, nullptr);
    lv_obj_add_event_cb(roller.root, OnCancel, LV_EVENT_CANCEL, nullptr);
    lv_obj_add_event_cb(roller.root, OnClicked, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(roller.root, OnStateChanged, LV_EVENT_STATE_CHANGED, nullptr);
    detail::ToggleEditingOnKeyClick(roller.root);
    detail::ClaimSwipe(roller.root, OnSwipe);
    ui::RingOutside(roller.root);
    ui::RingEditing(roller.root);
    detail::JoinGroup(roller.root);
    Draw(roller.root);
    return roller;
}

Roller MakeNumberRoller(lv_obj_t *parent, const char *name, int first, int last, int step, int digits, bool circular, int value)
{
    const int by = step > 0 ? step : 1;
    const int count = std::max((last - first) / by + 1, 2);
    RollerSpec spec{};
    spec.name = name;
    spec.count = count;
    spec.selected = std::clamp((value - first + by / 2) / by, 0, count - 1);
    spec.circular = circular;
    spec.first = first;
    spec.step = by;
    spec.digits = digits;
    return MakeRoller(parent, spec);
}

int RollerSelected(const Roller &roller)
{
    return Of(roller.root)->index;
}

int RollerValue(const Roller &roller)
{
    const State *s = Of(roller.root);
    return s->first + s->index * s->step;
}

void SetRoller(const Roller &roller, int index)
{
    State *s = Of(roller.root);
    s->index = std::clamp(index, 0, s->count - 1);
    Draw(roller.root);
}

void SetRollerValue(const Roller &roller, int value)
{
    const State *s = Of(roller.root);
    SetRoller(roller, (value - s->first + s->step / 2) / s->step);
}

bool RollerOpen(const Roller &roller)
{
    return Of(roller.root)->edit.open;
}

namespace {
// The footer's Cancel: the open roller puts its value back, and the group stops editing, which closes it.
void OnCancelClicked(lv_event_t *e)
{
    lv_obj_t *bar = static_cast<lv_obj_t *>(lv_event_get_user_data(e));
    lv_obj_t *root = static_cast<lv_obj_t *>(lv_obj_get_user_data(bar));
    if (root == nullptr) {
        return;
    }
    lv_obj_send_event(root, LV_EVENT_CANCEL, nullptr);
    if (lv_group_t *group = lv_obj_get_group(root); group != nullptr) {
        lv_group_set_editing(group, false);
    }
}
}  // namespace

lv_obj_t *MakeRollerFooter(lv_obj_t *footer)
{
    lv_obj_t *bar = detail::Bare(footer);
    lv_obj_set_size(bar, lv_pct(100), dt::kSpacingFooter);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(bar, dt::kSpacingMd, 0);
    detail::LetRingOut(bar);
    constexpr int32_t kHalf = (dt::kSpacingContent - dt::kSpacingMd) / 2;
    lv_obj_t *cancel = MakeButton(bar, "Cancel", ButtonVariant::kOutline, kHalf);
    lv_group_remove_obj(cancel);  // a touch stop: UP and DOWN are the open roller's, and holding OK is the buttons' Cancel
    lv_obj_add_event_cb(cancel, OnCancelClicked, LV_EVENT_CLICKED, bar);
    lv_obj_t *hint = ui::NewLabel(bar, ui::Type::kCaption, "UP or DOWN steps.\nOK keeps.\nHold OK cancels.", true);
    lv_obj_set_flex_grow(hint, 1);
    lv_obj_set_width(hint, 1);
    lv_obj_add_flag(bar, LV_OBJ_FLAG_HIDDEN);
    return bar;
}

void SetRollerFooter(const Roller &roller, lv_obj_t *bar)
{
    router::Hold(&Of(roller.root)->bar, bar);  // nullptr if the footer goes first
}

}  // namespace widgets
