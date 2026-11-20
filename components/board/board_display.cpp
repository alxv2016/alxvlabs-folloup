// The glass: our drivers for the SSD1677 (V1.0) and the SSD2677 (V2.0) on the shared SPI2 bus. The controller's ID
// picks the driver, so one image runs on both revisions (STICKY_HARDWARE_SPECS §4.5).

#include <memory>

#include "board.h"
#include "board_config.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ssd1677.h"
#include "ssd2677.h"

namespace board {
namespace {

namespace bc = board_config;
constexpr const char *kTag = "board_display";

epaper::Bus s_bus;
std::unique_ptr<epaper::Panel> s_panel;

}  // namespace

esp_err_t InitDisplay(Display *out, const float *ambient_c)
{
    ESP_RETURN_ON_FALSE(out != nullptr, ESP_ERR_INVALID_ARG, kTag, "no output");

    // EPD_EN high powers EP_3V3; the panel needs 100 ms to settle (STICKY_HARDWARE_SPECS §3.1).
    gpio_config_t power = {};
    power.pin_bit_mask = 1ULL << bc::kEpPwrEn;
    power.mode = GPIO_MODE_OUTPUT;
    ESP_RETURN_ON_ERROR(gpio_config(&power), kTag, "panel power pin");
    ESP_RETURN_ON_ERROR(gpio_set_level(bc::kEpPwrEn, 1), kTag, "panel power");
    vTaskDelay(pdMS_TO_TICKS(bc::kPanelPowerSettleMs));

    epaper::BusConfig bus = {};
    bus.host = static_cast<spi_host_device_t>(bc::kSharedSpiHost);
    bus.cs = bc::kEpCs;
    bus.dc = bc::kEpDc;
    bus.rst = bc::kEpRst;
    bus.busy = bc::kEpBusy;
    bus.read_hz = bc::kPanelReadHz;
    bus.write_hz = bc::kPanelWriteHz;

    LockSharedBus();
    esp_err_t err = s_bus.Init(bus);
    uint8_t id = 0;
    if (err == ESP_OK) {
        err = epaper::ReadId(s_bus, &id);
    }
    UnlockSharedBus();
    ESP_RETURN_ON_ERROR(err, kTag, "the panel's bus and ID");

    out->ssd2677 = id == ssd2677::kChipId;
    s_panel = out->ssd2677 ? ssd2677::Create(s_bus) : ssd1677::Create(s_bus);
    ESP_RETURN_ON_FALSE(s_panel != nullptr, ESP_ERR_NO_MEM, kTag, "the panel's driver");
    if (ambient_c != nullptr) {
        s_panel->SetTemperature(*ambient_c);
    }
    out->panel = s_panel.get();
    ESP_LOGI(kTag, "panel: %s (%s), ID 0x%02X", s_panel->Name(), out->ssd2677 ? "V2.0, matte" : "V1.0, glossy", id);
    return ESP_OK;
}

}  // namespace board
