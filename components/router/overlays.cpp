// The overlay stack: OpenOverlay, CloseOverlay, DismissOverlays, which overlay has the buttons, whether the screen is covered,
// and the escape chain's overlay link (router.h; APP_ARCHITECTURE §7).

#include "esp_log.h"
#include "lvgl_display.h"
#include "router/router.h"
#include "router_internal.h"

namespace router {
namespace {

constexpr const char *kTag = "router";

Overlay *s_overlays[kMaxOverlays] = {};
size_t s_count = 0;

size_t Covering()
{
    size_t n = 0;
    for (size_t i = 0; i < s_count; ++i) {
        n += s_overlays[i]->covers_screen ? 1 : 0;
    }
    return n;
}

// The screen hears the edges only: covered when the first covering overlay opens, uncovered when the last one goes.
void TellCovered(bool covered)
{
    if (Screen *screen = detail::TopScreen(); screen != nullptr) {
        screen->OnCovered(covered);
    }
}

}  // namespace

lv_group_t *OpenOverlay(Overlay *overlay)
{
    if (overlay == nullptr || overlay->group != nullptr) {
        return nullptr;  // nothing, or already open
    }
    if (s_count == kMaxOverlays) {
        ESP_LOGW(kTag, "overlay refused: %u are open", static_cast<unsigned>(kMaxOverlays));
        return nullptr;
    }
    lv_group_t *group = lv_group_create();
    if (group == nullptr) {
        ESP_LOGE(kTag, "no memory for an overlay's group");
        return nullptr;
    }
    lv_group_set_wrap(group, true);
    lv_group_set_editing(group, false);
    // The ring off the control focused under it, which the group goes on naming.
    overlay->beneath = lvgl_display::Group();
    overlay->beneath_screen = lv_screen_active();
    lv_obj_t *named = overlay->beneath != nullptr ? lv_group_get_focused(overlay->beneath) : nullptr;
    overlay->ring_showed = named != nullptr && lv_obj_has_state(named, LV_STATE_FOCUS_KEY);
    if (named != nullptr) {
        lv_obj_remove_state(named, LV_STATE_FOCUS_KEY);
    }
    overlay->group = group;
    const bool covers_first = overlay->covers_screen && Covering() == 0;
    s_overlays[s_count++] = overlay;
    lvgl_display::SetGroup(group);
    if (overlay->screen != nullptr) {
        lv_screen_load(overlay->screen);
    }
    if (covers_first) {
        TellCovered(true);
    }
    return group;
}

void CloseOverlay(Overlay *overlay)
{
    size_t at = 0;
    while (at < s_count && s_overlays[at] != overlay) {
        ++at;
    }
    if (overlay == nullptr || at == s_count) {
        return;  // not open
    }
    const bool covered_before = Covering() > 0;
    const bool was_top = at + 1 == s_count;
    for (size_t i = at; i + 1 < s_count; ++i) {
        s_overlays[i] = s_overlays[i + 1];
    }
    s_overlays[--s_count] = nullptr;
    if (!was_top) {
        // The overlay that opened over this one now stands over what this one stood on.
        Overlay *above = s_overlays[at];
        above->beneath = overlay->beneath;
        above->beneath_screen = overlay->beneath_screen;
        above->ring_showed = overlay->ring_showed;
    } else {
        lvgl_display::SetGroup(overlay->beneath);  // before the group goes: the buttons never point at a deleted group
        lv_obj_t *named = overlay->beneath != nullptr ? lv_group_get_focused(overlay->beneath) : nullptr;
        if (named != nullptr && overlay->ring_showed) {
            lv_obj_add_state(named, LV_STATE_FOCUS_KEY);
        }
    }
    // A full-glass overlay whose screen is on the glass — the top one, or one under overlays drawn on its screen (a dialog
    // over the keyboard) — gives way to the screen it opened over, whatever closes it: its screen is about to be deleted.
    if (overlay->screen != nullptr && lv_screen_active() == overlay->screen && overlay->beneath_screen != nullptr) {
        lv_screen_load(overlay->beneath_screen);
        lvgl_display::RequestFull();  // a full-glass overlay closes on a full refresh (DESIGN → Motion)
    }
    lv_group_delete(overlay->group);
    overlay->group = nullptr;
    if (covered_before && Covering() == 0) {
        TellCovered(false);
    }
    if (s_count == 0) {
        detail::StatusOverlaysClosed();  // the news held while it stood is drawn, and sounded, now
    }
}

void DismissOverlays()
{
    while (s_count > 0) {
        Overlay *top = s_overlays[s_count - 1];
        if (top->dismiss != nullptr) {
            top->dismiss(top->user);
        }
        if (s_count > 0 && s_overlays[s_count - 1] == top) {
            if (top->dismiss != nullptr) {
                ESP_LOGW(kTag, "an overlay's dismiss left it open: closed from the stack");
            }
            CloseOverlay(top);
        }
    }
}

bool OverlayOpen()
{
    return s_count > 0;
}

namespace detail {

// The escape chain's overlay link: the top overlay closes as its Cancel does. False with none open.
bool EscapeTopOverlay()
{
    if (s_count == 0) {
        return false;
    }
    Overlay *top = s_overlays[s_count - 1];
    void (*close)(void *) = top->escape != nullptr ? top->escape : top->dismiss;
    if (close != nullptr) {
        close(top->user);
    }
    if (s_count > 0 && s_overlays[s_count - 1] == top) {
        CloseOverlay(top);  // an escape that left it open: the router closes it
    }
    return true;
}

}  // namespace detail
}  // namespace router
