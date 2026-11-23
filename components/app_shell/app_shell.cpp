#include "app_shell.h"

#include "app_hooks.h"
#include "board.h"
#include "devices.h"
#include "lvgl_display.h"
#include "router/router.h"
#include "ui.h"
#include "esp_app_desc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "screen_home.h"

namespace app_shell {
namespace {

constexpr const char *kTag = "app_shell";

void LogIdentity()
{
    const esp_app_desc_t *app = esp_app_get_description();
    ESP_LOGI(kTag, "%s %s, ESP-IDF %s", app->project_name, app->version, app->idf_ver);
}

// The devices on the sensor bus, before the glass: the SHT40 gives the SSD2677 its waveform temperature.
void StartDevices()
{
    ESP_ERROR_CHECK(devices::Init());
}

// The first frame: ui's fonts and styles, the router, then home as its root — under lvgl_display's lock, once the display and
// the buttons are up, and drawn as one full refresh (APP_ARCHITECTURE §3, §7).
void FirstScreen()
{
    ESP_ERROR_CHECK(ui::Start());
    ESP_ERROR_CHECK(router::Init());
    ESP_ERROR_CHECK(router::SetRoot(router::MakeScreen(screen_home::Build)));
}

// A widget's cue, as the buzzer's sound word: the glass's controls ask by event, and only the shell knows the buzzer plays
// them (STICKY_DESIGN_TOKENS §5).
void PlayCue(lvgl_display::Cue cue)
{
    switch (cue) {
    case lvgl_display::Cue::kAtEnd: devices::Play(devices::sound::Word::kAtEnd); break;
    case lvgl_display::Cue::kSuccess: devices::Play(devices::sound::Word::kSuccess); break;
    case lvgl_display::Cue::kError: devices::Play(devices::sound::Word::kError); break;
    }
}

bool ReadRoomTemperature(float *celsius)
{
    devices::Ambient ambient;
    if (devices::ReadAmbient(&ambient) != ESP_OK) {
        return false;
    }
    *celsius = ambient.temperature_c;
    return true;
}

// Brings the glass up on either controller, starts LVGL on it and shows home. A display that does not come up stops
// the boot: the product is its screen.
void StartUi()
{
    devices::Ambient ambient;
    const bool have_ambient = devices::ReadAmbient(&ambient) == ESP_OK;
    board::Display display{};
    ESP_ERROR_CHECK(board::InitDisplay(&display, have_ambient ? &ambient.temperature_c : nullptr));

    // Home is the first frame the glass shows, drawn full (STICKY_DESIGN_TOKENS §5).
    bool touch_ok = false;
    ESP_ERROR_CHECK(lvgl_display::Start(display, &touch_ok, FirstScreen));
    lvgl_display::SetPressFeedback(devices::Tick);
    lvgl_display::SetCuePlayer(PlayCue);
    lvgl_display::SetTemperatureSource(ReadRoomTemperature);
}

}  // namespace

void Install()
{
    // The core's own boot steps register here, first, so they lead each phase.
    app_hooks::OnBoot(app_hooks::Phase::kBoardUp, StartDevices);
    app_hooks::OnBoot(app_hooks::Phase::kLate, LogIdentity);
}

void Run()
{
    app_hooks::Seal();

    // A board that does not come up stops the boot here, with the error in the log.
    ESP_ERROR_CHECK(board::Init());
    app_hooks::RunBoot(app_hooks::Phase::kBoardUp);

    StartUi();

    app_hooks::RunBoot(app_hooks::Phase::kReady);
    app_hooks::RunBoot(app_hooks::Phase::kLate);
}

}  // namespace app_shell
