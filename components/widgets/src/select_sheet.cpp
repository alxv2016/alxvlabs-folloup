#include "widgets/select_sheet.h"

#include <vector>

#include "design_tokens.h"
#include "internal/base.h"
#include "internal/glue.h"
#include "internal/marks.h"
#include "internal/sheet_rules.h"
#include "router/router.h"
#include "widgets/button.h"
#include "widgets/list_view.h"

namespace widgets {
namespace dt = design_tokens;

int SelectSheetRowsPerPage(int32_t field_bottom, int32_t glass_height)
{
    return detail::sheet::RowsPerPage(field_bottom, glass_height);
}

namespace {
// The sheet hangs off the glass: it is drawn wider than the glass and a corner radius taller, so only its top outline and its
// rounded top corners show, and no side outline stands on the glass's edge.
constexpr int32_t kHang = dt::kSpacingStrokeStrong;  // wider than the glass by this each side: the outline's width
constexpr int32_t kHangBelow = dt::kRounded2xl;  // taller than the glass by a radius and more, so no bottom corner shows
// An option's words: the row's inside, less the check's own width (widgets_mark_glyph_fact_check, 28 px) and the gap before it, so
// a row holds 20 characters of `body` or `label` to a line (18 px a character) whether it is the chosen one or not.
int32_t WordsWidth()
{
    return dt::kSpacingContent - 2 * dt::kSpacingMd - static_cast<int32_t>(widgets_mark_glyph_fact_check.header.w) - dt::kSpacingSm;
}

struct Sheet {
    Dropdown *dropdown;
    lv_obj_t *catcher;  // a touch above the sheet lands here and nowhere else
    lv_obj_t *box;
    router::Overlay overlay;  // the sheet on the router's overlay stack: its group, holding OK, and focus back to the field
};
Sheet *s_open = nullptr;

// Closes: off the overlay stack — the page's group is bound again, focus still on the field, ringed if it was — then the sheet's
// objects go, and a choice is reported. The objects go first-hand, last: this runs inside the events of the objects it deletes.
void Close(int choose)
{
    Sheet *s = s_open;
    if (s == nullptr) {
        return;
    }
    s_open = nullptr;
    Dropdown *dropdown = s->dropdown;
    lv_obj_t *catcher = s->catcher;
    lv_obj_t *box = s->box;  // its delete event frees `s`
    router::CloseOverlay(&s->overlay);
    lv_obj_delete(box);
    lv_obj_delete(catcher);
    if (choose >= 0) {
        ChooseDropdown(dropdown, choose);
    }
}

void CloseAsCancel(void *)
{
    Close(-1);
}

void OnRowClicked(lv_event_t *e)
{
    Close(static_cast<int>(lv_obj_get_index(lv_event_get_target_obj(e))));
}

void OnCancelClicked(lv_event_t *)
{
    Close(-1);
}

// The sheet freed with its screen: a screen deleted while it is open takes it along, off the overlay stack first.
void OnBoxDelete(lv_event_t *e)
{
    Sheet *s = static_cast<Sheet *>(lv_event_get_user_data(e));
    if (s_open == s) {
        s_open = nullptr;
        router::CloseOverlay(&s->overlay);
    }
    delete s;
}

lv_obj_t *MakeRow(lv_obj_t *parent, const char *text, bool chosen)
{
    lv_obj_t *row = lv_button_create(parent);
    lv_obj_remove_style_all(row);  // touch-exception: no pressed state (DESIGN → Motion → Press)
    lv_obj_set_scrollable(row, false);
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(row, dt::kSpacingTarget, 0);  // the touch floor; a longer option grows the row
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(row, dt::kSpacingMd, 0);
    lv_obj_set_style_pad_ver(row, dt::kSpacingSm, 0);
    lv_obj_set_style_radius(row, dt::kRoundedLg, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(row, ui::Level(chosen ? dt::kColorSurfaceChosen : dt::kColorBackground), 0);
    // The option in `body`, the chosen one in `label`, wrapping to a second line and then a third, each line on the role's line
    // box. The words column is the one that stands the floor tall and centres its lines: a flex track is as tall as its tallest child, and the
    // row's own minimum would only pad under it.
    lv_obj_t *column = detail::Bare(row);
    lv_obj_set_size(column, WordsWidth(), LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(column, dt::kSpacingTarget - 2 * dt::kSpacingSm, 0);
    lv_obj_set_flex_flow(column, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(column, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_t *words = lv_label_create(column);
    lv_label_set_text(words, text);
    const ui::Type role = chosen ? ui::Type::kLabel : ui::Type::kBody;  // the chosen option in bold, the rest in regular
    lv_obj_set_style_text_font(words, ui::Font(role), 0);
    lv_obj_set_style_text_line_space(words, ui::LineBox(role) - lv_font_get_line_height(ui::Font(role)), 0);
    lv_obj_set_style_text_color(words, ui::Level(chosen ? dt::kColorTextOnFill : dt::kColorTextPrimary), 0);
    lv_label_set_long_mode(words, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(words, WordsWidth());
    ui::FitLineBox(words, role);
    if (chosen) {
        // The check alone marks the chosen option (DESIGN → Exceptions); the inversion marks it too, so it is never the only sign.
        lv_obj_t *check = lv_image_create(row);
        lv_image_set_src(check, &widgets_mark_glyph_fact_check);
        lv_obj_set_style_image_recolor(check, ui::Level(dt::kColorTextOnFill), 0);
        lv_obj_set_style_image_recolor_opa(check, LV_OPA_COVER, 0);
        lv_obj_remove_flag(check, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(check, LV_OBJ_FLAG_FLOATING);
        lv_obj_align(check, LV_ALIGN_RIGHT_MID, 0, 0);
    }
    ui::RingOutside(row);
    detail::JoinGroup(row);
    lv_obj_add_event_cb(row, OnRowClicked, LV_EVENT_CLICKED, nullptr);
    return row;
}
}  // namespace

void OpenSelectSheet(Dropdown *dropdown)
{
    if (s_open != nullptr || dropdown->count <= 0) {
        return;
    }
    const int selected = dropdown->selected >= 0 && dropdown->selected < dropdown->count ? dropdown->selected : 0;  // opens on the chosen row, or the first
    lv_obj_t *screen = lv_screen_active();
    const int32_t glass = lv_display_get_vertical_resolution(lv_obj_get_display(screen));
    lv_area_t field;
    lv_obj_get_coords(dropdown->field, &field);
    const int per = detail::sheet::RowsPerPage(field.y2 + 1, glass);
    // A list of fewer options than a page draws only its rows, and the sheet stands that much shorter.
    const int rows = dropdown->count < per ? (dropdown->count < 2 ? 2 : dropdown->count) : per;
    const int32_t height = detail::sheet::Height(rows);
    const int32_t top = glass - height;

    Sheet *s = new Sheet{};
    s->dropdown = dropdown;
    // On the overlay stack before a row is made: its rows and Cancel join the sheet's own group, and nothing behind the sheet
    // is reachable by the buttons. Holding OK is Cancel.
    s->overlay.escape = CloseAsCancel;
    s->overlay.dismiss = CloseAsCancel;
    s->overlay.user = s;
    if (router::OpenOverlay(&s->overlay) == nullptr) {
        delete s;
        return;
    }
    s_open = s;

    // A touch above the sheet is eaten: the tick the input task sounds, and nothing else — no tap lands on the page behind, and a
    // drag does not move it.
    s->catcher = detail::Bare(screen);
    lv_obj_add_flag(s->catcher, LV_OBJ_FLAG_FLOATING);
    lv_obj_add_flag(s->catcher, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(s->catcher, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_set_size(s->catcher, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(s->catcher, 0, 0);

    s->box = detail::Bare(screen);
    lv_obj_add_flag(s->box, LV_OBJ_FLAG_FLOATING);
    lv_obj_add_flag(s->box, LV_OBJ_FLAG_CLICKABLE);  // the sheet's own white is not "above" it
    lv_obj_remove_flag(s->box, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_set_pos(s->box, -kHang, top);
    lv_obj_set_size(s->box, lv_obj_get_width(screen) + 2 * kHang, height + kHangBelow);
    lv_obj_set_style_radius(s->box, dt::kRounded2xl, 0);
    lv_obj_set_style_bg_opa(s->box, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(s->box, ui::Level(dt::kColorBackground), 0);
    detail::Outline(s->box, dt::kSpacingStrokeStrong);
    ui::AddOffset(s->box, ui::Elevation::kAnchored, ui::OffsetSide::kTop);
    detail::LetRingOut(s->box);
    lv_obj_add_event_cb(s->box, OnBoxDelete, LV_EVENT_DELETE, s);

    // Inside the outline: the title, the rows, the foot.
    lv_obj_t *column = detail::Bare(s->box);
    lv_obj_set_size(column, lv_obj_get_width(screen), height - dt::kSpacingStrokeStrong);
    lv_obj_set_pos(column, 0, 0);
    lv_obj_set_flex_flow(column, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_hor(column, dt::kSpacingEdge, 0);
    lv_obj_set_style_pad_top(column, dt::kSpacingMd, 0);
    lv_obj_set_style_pad_row(column, dt::kSpacingMd, 0);
    detail::LetRingOut(column);
    lv_obj_t *title = detail::Line(column, ui::Type::kLabel, dropdown->name.c_str(), false);
    lv_obj_set_width(title, lv_pct(100));

    const ListView view = MakeListView(column, rows, dt::kSpacingTarget);
    std::vector<lv_obj_t *> option_rows;
    for (int i = 0; i < dropdown->count; ++i) {
        option_rows.push_back(MakeRow(view.root, dropdown->options[i], i == dropdown->selected));  // no row is the chosen one when none is
    }
    UpdateListView(view);

    lv_obj_t *foot = detail::Bare(column);
    lv_obj_set_size(foot, lv_pct(100), dt::kSpacingFooter);
    lv_obj_set_flex_flow(foot, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(foot, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(foot, dt::kSpacingMd, 0);
    detail::LetRingOut(foot);
    constexpr int32_t kHalf = (dt::kSpacingContent - dt::kSpacingMd) / 2;
    lv_obj_t *cancel = MakeButton(foot, "Cancel", ButtonVariant::kOutline, kHalf);
    lv_obj_add_event_cb(cancel, OnCancelClicked, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *hint = ui::NewLabel(foot, ui::Type::kCaption, "UP or DOWN picks.\nOK chooses.", true);
    lv_obj_set_flex_grow(hint, 1);
    lv_obj_set_width(hint, 1);

    // Focus opens on the chosen row, turning to its page (the list view's own rule), the ring shown.
    router::Focus(option_rows[static_cast<size_t>(selected)]);  // ringed when the buttons opened it
}

void CloseSelectSheet()
{
    Close(-1);
}

bool SelectSheetOpen()
{
    return s_open != nullptr;
}

}  // namespace widgets
