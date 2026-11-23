// The status line: Say, Clear and the band a screen attaches, drawn from status_rules.h's steps (router.h; APP_ARCHITECTURE §7).

#include "esp_log.h"
#include "lvgl_display.h"
#include "router/router.h"
#include "router_internal.h"

namespace router {
namespace {

constexpr const char *kTag = "router";

StatusState s_state;
detail::StatusBand s_band;     // the screen on top's band, nullptrs when it has none
detail::StatusBand s_pending;  // what the screen being built has attached so far
bool s_creating = false;
bool s_entered = false;  // a screen is on top, between StatusEntered and the next StatusLeaving

lvgl_display::Cue CueOf(StatusSound sound)
{
    return sound == StatusSound::kError ? lvgl_display::Cue::kError : lvgl_display::Cue::kSuccess;
}

// Does what a step says to the band: the sound first — it leads the refresh, and a repeat of the words asks it with no redraw —
// then the words and the rule.
void Apply(const StatusStep &step)
{
    if (step.sound != StatusSound::kNone) {
        lvgl_display::PlayCue(CueOf(step.sound));
    }
    if (step.draw == StatusDraw::kNothing || s_band.words == nullptr) {
        return;
    }
    if (step.draw == StatusDraw::kShow) {
        lv_label_set_text(s_band.words, s_state.Drawn().words);
        lv_obj_remove_flag(s_band.rule, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_label_set_text(s_band.words, "");
        lv_obj_add_flag(s_band.rule, LV_OBJ_FLAG_HIDDEN);
    }
}

}  // namespace

StatusId Say(StatusKind kind, const char *words)
{
    const StatusStep step = s_state.Say(kind, words, OverlayOpen());
    if (step.id == 0) {
        // Never the words: they are the person's, and may be a file's name.
        ESP_LOGW(kTag, "status line: nothing said (kind %d) — %s", static_cast<int>(kind),
                 !s_entered                ? "no screen is entered: one is being built or is leaving, say from OnEnter or later"
                 : !s_state.HasBand()      ? "the screen has no band"
                                           : "no words");
        return 0;
    }
    if (step.over_cap) {
        ESP_LOGW(kTag, "status line: %u characters (kind %d), over the %u cap: the second line ends in dots",
                 static_cast<unsigned>(step.chars), static_cast<int>(kind), static_cast<unsigned>(kStatusMaxChars));
    }
    if (step.cut) {
        ESP_LOGW(kTag, "status line: %u characters (kind %d) are more than the %u bytes kept: cut at a character's edge",
                 static_cast<unsigned>(step.chars), static_cast<int>(kind), static_cast<unsigned>(kStatusBytes));
    }
    Apply(step);
    return step.id;
}

void Clear(StatusId id)
{
    Apply(s_state.Clear(id, OverlayOpen()));
}

void AttachStatusLine(lv_obj_t *words, lv_obj_t *rule)
{
    if (!s_creating || words == nullptr || rule == nullptr) {
        ESP_LOGW(kTag, "status line: a band attached outside OnCreate is ignored");
        return;
    }
    s_pending = detail::StatusBand{words, rule};
}

namespace detail {

void BeginStatusBand()
{
    s_pending = StatusBand{};
    s_creating = true;
}

StatusBand TakeStatusBand()
{
    s_creating = false;
    const StatusBand band = s_pending;
    s_pending = StatusBand{};
    return band;
}

void StatusLeaving()
{
    // The band of the screen leaving is left white, so it is white when the screen is shown again; no refresh of its own, the
    // screen changing is one.
    if (s_state.Drawn().id != 0 && s_band.words != nullptr) {
        lv_label_set_text(s_band.words, "");
        lv_obj_add_flag(s_band.rule, LV_OBJ_FLAG_HIDDEN);
    }
    s_state.ScreenChanged(false);
    s_band = StatusBand{};
    s_entered = false;
}

void StatusEntered(const StatusBand &entering)
{
    s_band = entering;
    s_entered = true;
    s_state.ScreenChanged(entering.words != nullptr);
}

void StatusOverlaysClosed()
{
    Apply(s_state.OverlaysClosed());
}

}  // namespace detail
}  // namespace router
