#include "app_shell.h"

#include "app_hooks.h"
#include "board.h"
#include "devices.h"
#include "lvgl_display.h"
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

// The first frame: ui's fonts and styles, then home — under lvgl_display's lock, drawn as one full refresh.
void FirstScreen()
{
    ESP_ERROR_CHECK(ui::Start());
    screen_home::Show();
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
