#include "widgets/dropdown.h"

#include "design_tokens.h"
#include "internal/base.h"
#include "internal/marks.h"
#include "widgets/select_sheet.h"
#include "widgets/value_row.h"

namespace widgets {
namespace dt = design_tokens;

namespace {
void OpenSheet(Dropdown *dropdown, void *)
{
    OpenSelectSheet(dropdown);
}

void OnClicked(lv_event_t *e)
{
    Dropdown *dropdown = static_cast<Dropdown *>(lv_event_get_user_data(e));
    if (dropdown->opener != nullptr && !lv_obj_has_state(dropdown->field, LV_STATE_DISABLED)) {
        dropdown->opener(dropdown, dropdown->user);
    }
}

void OnDelete(lv_event_t *e)
{
    delete static_cast<Dropdown *>(lv_event_get_user_data(e));
}
}  // namespace

Dropdown *MakeDropdown(lv_obj_t *parent, const char *name, const char *const *options, int count, int selected)
{
    // A `selected` outside the options (-1, or past the end) starts on the first, as SetDropdownSelected refuses it; no options, none.
    selected = (selected >= 0 && selected < count) ? selected : (count > 0 ? 0 : -1);
    Dropdown *dropdown = new Dropdown{};
    dropdown->name = name;
    dropdown->opener = OpenSheet;  // the select sheet, until a caller swaps in its own
    dropdown->options = options;
    dropdown->count = count;
    dropdown->selected = selected;
    dropdown->frame = detail::Bare(parent);
    lv_obj_set_size(dropdown->frame, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(dropdown->frame, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(dropdown->frame, dt::kSpacingMd, 0);  // the reason stands a page gap under the field, as any block
    detail::LetRingOut(dropdown->frame);
    const ValueRow row = MakeValueRow(dropdown->frame, name, selected >= 0 ? options[selected] : "");
    dropdown->field = row.root;
    dropdown->value = row.value;
    // The chevron stands after the value, `spacing.md` on (the row's column gap); it takes the row's disabled state with the
    // words, which the row passes down.
    detail::NewMark(row.root, &widgets_mark_glyph_chevron, dt::kColorTextPrimary, dt::kColorTextPrimary);
    dropdown->reason = detail::Words(dropdown->frame, ui::Type::kCaption, "", true);
    lv_obj_add_flag(dropdown->reason, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(row.root, OnClicked, LV_EVENT_CLICKED, dropdown);
    lv_obj_add_event_cb(row.root, OnDelete, LV_EVENT_DELETE, dropdown);
    return dropdown;
}

int DropdownSelected(const Dropdown *dropdown)
{
    return dropdown->selected;
}

void SetDropdownSelected(Dropdown *dropdown, int index)
{
    if (index < 0 || index >= dropdown->count) {
        return;
    }
    dropdown->selected = index;
    lv_label_set_text(dropdown->value, dropdown->options[index]);
}

void ChooseDropdown(Dropdown *dropdown, int index)
{
    if (index < 0 || index >= dropdown->count || index == dropdown->selected) {
        return;
    }
    SetDropdownSelected(dropdown, index);
    lv_obj_send_event(dropdown->field, LV_EVENT_VALUE_CHANGED, nullptr);
}

void SetDropdownEnabled(Dropdown *dropdown, bool enabled, const char *reason)
{
    lv_obj_set_state(dropdown->field, LV_STATE_DISABLED, !enabled);
    if (!enabled && reason != nullptr) {
        lv_label_set_text(dropdown->reason, reason);
        lv_obj_remove_flag(dropdown->reason, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(dropdown->reason, LV_OBJ_FLAG_HIDDEN);
    }
}

void SetDropdownOpener(Dropdown *dropdown, DropdownOpener open, void *user)
{
    dropdown->opener = open;
    dropdown->user = user;
}

}  // namespace widgets
