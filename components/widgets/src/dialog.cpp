#include "widgets/dialog.h"

#include <string>

#include "design_tokens.h"
#include "internal/base.h"
#include "router/router.h"
#include "widgets/button.h"

namespace widgets {
namespace dt = design_tokens;

Dialog MakeDialog(lv_obj_t *parent, const char *title, const char *message, const char *action, const char *cancel)
{
    // Two half buttons in the box's inside: the content, less the outline and the padding either side.
    constexpr int32_t kInside = dt::kSpacingContent - 2 * dt::kSpacingStrokeStrong - 2 * dt::kSpacingLg;
    constexpr int32_t kHalf = (kInside - dt::kSpacingMd) / 2;
    Dialog dialog{};
    dialog.frame = detail::Bare(parent);
    lv_obj_set_size(dialog.frame, lv_pct(100), LV_SIZE_CONTENT);
    detail::LetRingOut(dialog.frame);  // also lets the offset out of the frame: LVGL clips a child to its parent
    dialog.box = detail::Bare(dialog.frame);
    lv_obj_set_size(dialog.box, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(dialog.box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(dialog.box, dt::kSpacingLg, 0);
    lv_obj_set_style_pad_row(dialog.box, dt::kSpacingMd, 0);
    lv_obj_set_style_radius(dialog.box, dt::kRoundedMd, 0);
    lv_obj_set_style_bg_opa(dialog.box, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(dialog.box, ui::Level(dt::kColorBackground), 0);
    detail::Outline(dialog.box, dt::kSpacingStrokeStrong);
    ui::AddOffset(dialog.box, ui::Elevation::kFloating, ui::OffsetSide::kBottomRight);

    detail::Words(dialog.box, ui::Type::kLabel, title);
    detail::Words(dialog.box, ui::Type::kBody, message);
    lv_obj_t *buttons = detail::Bare(dialog.box);
    lv_obj_set_size(buttons, lv_pct(100), dt::kSpacingTarget);
    lv_obj_set_flex_flow(buttons, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(buttons, dt::kSpacingMd, 0);
    detail::LetRingOut(buttons);
    dialog.action = MakeButton(buttons, action, ButtonVariant::kAction, kHalf);
    dialog.cancel = MakeButton(buttons, cancel, ButtonVariant::kOutline, kHalf);
    return dialog;
}

namespace {

struct Open {
    DialogConfig config;
    std::string title, message, action, cancel;  // the words kept for as long as it is open
    lv_obj_t *catcher;                           // the glass round the box: a touch there lands here and nowhere else
    router::Overlay overlay;
};
Open *s_open = nullptr;

// Closes: off the overlay stack — the group under it bound again, focus back on what opened it — then its objects, then the
// answer. Runs inside the events of the objects it deletes, so they go last.
void Close(bool accepted)
{
    Open *d = s_open;
    if (d == nullptr) {
        return;
    }
    s_open = nullptr;
    const DialogDone done = d->config.done;
    void *user = d->config.user;
    router::CloseOverlay(&d->overlay);
    lv_obj_delete(d->catcher);  // its delete event frees `d`
    if (done != nullptr) {
        done(accepted, user);
    }
}

void OnAction(lv_event_t *)
{
    Close(true);
}

void OnCancel(lv_event_t *)
{
    Close(false);
}

void CancelOverlay(void *)
{
    Close(false);
}

// Freed with its screen: a screen deleted while the dialog is open takes it along, off the overlay stack first.
void OnCatcherDelete(lv_event_t *e)
{
    Open *d = static_cast<Open *>(lv_event_get_user_data(e));
    if (s_open == d) {
        s_open = nullptr;
        router::CloseOverlay(&d->overlay);
    }
    delete d;
}

}  // namespace

void OpenDialog(const DialogConfig &config)
{
    if (s_open != nullptr) {
        return;
    }
    Open *d = new Open{};
    d->config = config;
    d->title = config.title != nullptr ? config.title : "";
    d->message = config.message != nullptr ? config.message : "";
    d->action = config.action != nullptr ? config.action : "";
    d->cancel = config.cancel != nullptr ? config.cancel : "Cancel";
    // On the overlay stack before its buttons are made, so they are its group's only stops. Holding OK is Cancel.
    d->overlay.escape = CancelOverlay;
    d->overlay.dismiss = CancelOverlay;
    d->overlay.user = d;
    if (router::OpenOverlay(&d->overlay) == nullptr) {
        delete d;
        return;
    }
    s_open = d;

    // The catcher: the whole glass, clickable and doing nothing, so a touch outside the box is eaten — the tick, and nothing
    // else — and no drag reaches the page behind (DESIGN → Focus and input). The box stands in it, `spacing.edge` in from each
    // side and in the glass's middle.
    lv_obj_t *screen = lv_screen_active();
    d->catcher = detail::Bare(screen);
    lv_obj_set_floating(d->catcher, true);
    lv_obj_set_clickable(d->catcher, true);
    lv_obj_set_gesture_bubble(d->catcher, false);
    lv_obj_set_size(d->catcher, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(d->catcher, 0, 0);
    lv_obj_set_style_pad_hor(d->catcher, dt::kSpacingEdge, 0);
    lv_obj_set_flex_flow(d->catcher, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(d->catcher, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    detail::LetRingOut(d->catcher);
    lv_obj_add_event_cb(d->catcher, OnCatcherDelete, LV_EVENT_DELETE, d);

    const Dialog dialog = MakeDialog(d->catcher, d->title.c_str(), d->message.c_str(), d->action.c_str(), d->cancel.c_str());
    lv_obj_set_clickable(dialog.box, true);  // the box's own white is not "outside" it
    lv_obj_add_event_cb(dialog.action, OnAction, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(dialog.cancel, OnCancel, LV_EVENT_CLICKED, nullptr);
    router::Focus(config.destructive ? dialog.cancel : dialog.action);  // ringed when the buttons opened it
}

void CloseDialog()
{
    Close(false);
}

bool DialogOpen()
{
    return s_open != nullptr;
}

}  // namespace widgets
