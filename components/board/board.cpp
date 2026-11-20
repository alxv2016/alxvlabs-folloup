#include "board.h"

#include <initializer_list>

#include "board_config.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "driver/spi_master.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"
#include "esp_chip_info.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "sdkconfig.h"

namespace board {
namespace {

namespace bc = board_config;
constexpr const char *kTag = "board";

i2c_master_bus_handle_t s_sensor_bus = nullptr;
i2c_master_bus_handle_t s_touch_bus = nullptr;
SemaphoreHandle_t s_shared_bus = nullptr;
bool s_charging_enabled = true;

adc_oneshot_unit_handle_t s_adc = nullptr;
adc_cali_handle_t s_adc_cali = nullptr;
adc_channel_t s_adc_channel = ADC_CHANNEL_0;

esp_err_t Output(gpio_num_t pin, int level)
{
    // Preload the level before the pin drives, so it never glitches through the other one.
    ESP_RETURN_ON_ERROR(gpio_set_level(pin, level), kTag, "preload GPIO%d", pin);
    gpio_config_t config = {};
    config.pin_bit_mask = 1ULL << pin;
    config.mode = GPIO_MODE_OUTPUT;
    ESP_RETURN_ON_ERROR(gpio_config(&config), kTag, "configure GPIO%d", pin);
    return gpio_set_level(pin, level);
}

esp_err_t Input(gpio_num_t pin, bool pull_up)
{
    gpio_config_t config = {};
    config.pin_bit_mask = 1ULL << pin;
    config.mode = GPIO_MODE_INPUT;
    config.pull_up_en = pull_up ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    return gpio_config(&config);
}

// Seeed's hold: both latch lines high, held after the button is released (STICKY_HARDWARE_SPECS §5.2). Any hold left
// from a deep sleep is released first, so the pins can be driven again.
esp_err_t HoldPower()
{
    gpio_deep_sleep_hold_dis();
    for (gpio_num_t pin : {bc::kPwrHold, bc::kPwrLock, bc::kTpRst, bc::kTpPwrEn, bc::kPdmEn, bc::kSdPwrEn,
                           bc::kEpPwrEn, bc::kBuzzerPwm}) {
        gpio_hold_dis(pin);
    }
    ESP_RETURN_ON_ERROR(Output(bc::kPwrHold, 1), kTag, "power hold");
    return Output(bc::kPwrLock, 1);
}

// Both chip selects high, the card powered, the panel's lines idle — before the first SPI clock (Seeed's order).
esp_err_t IdleSharedBus()
{
    ESP_RETURN_ON_ERROR(Output(bc::kSdD3, 1), kTag, "SD CS");
    ESP_RETURN_ON_ERROR(Output(bc::kSdPwrEn, 1), kTag, "SD power");
    ESP_RETURN_ON_ERROR(Output(bc::kEpCs, 1), kTag, "EPD CS");
    ESP_RETURN_ON_ERROR(Output(bc::kEpDc, 1), kTag, "EPD DC");
    ESP_RETURN_ON_ERROR(Output(bc::kEpRst, 1), kTag, "EPD reset");
    // No pull-up is fitted on card detect (STICKY_HARDWARE_SPECS §3.3).
    return Input(bc::kSdDetect, true);
}

// Initialised once and never freed: the panel and the card each add their own device (STICKY_HARDWARE_SPECS §1.2).
esp_err_t InitSharedSpi()
{
    spi_bus_config_t bus = {};
    bus.mosi_io_num = bc::kEpSdi;
    bus.miso_io_num = bc::kSdD0;
    bus.sclk_io_num = bc::kEpSck;
    bus.quadwp_io_num = -1;  // every unused data pin -1: a 0 hands GPIO0, the sensor bus's SCL, to SPI
    bus.quadhd_io_num = -1;
    bus.data4_io_num = -1;
    bus.data5_io_num = -1;
    bus.data6_io_num = -1;
    bus.data7_io_num = -1;
    bus.max_transfer_sz = bc::kSharedSpiMaxTransfer;
    return spi_bus_initialize(static_cast<spi_host_device_t>(bc::kSharedSpiHost), &bus, SPI_DMA_CH_AUTO);
}

esp_err_t InitI2c(int port, gpio_num_t sda, gpio_num_t scl, i2c_master_bus_handle_t *bus)
{
    i2c_master_bus_config_t config = {};
    config.i2c_port = static_cast<i2c_port_num_t>(port);
    config.sda_io_num = sda;
    config.scl_io_num = scl;
    config.clk_source = I2C_CLK_SRC_DEFAULT;
    config.glitch_ignore_cnt = 7;
    config.flags.enable_internal_pullup = true;
    return i2c_new_master_bus(&config, bus);
}

esp_err_t InitCharger()
{
    ESP_RETURN_ON_ERROR(Output(bc::kEnBatChg, 0), kTag, "charger enable");  // active low
    return Input(bc::kChargeState, true);
}

void InitPowerSense()
{
    adc_unit_t unit;
    if (adc_oneshot_io_to_channel(bc::kPwrInVolt, &unit, &s_adc_channel) != ESP_OK) {
        return;
    }
    adc_oneshot_unit_init_cfg_t unit_cfg = {};
    unit_cfg.unit_id = unit;
    if (adc_oneshot_new_unit(&unit_cfg, &s_adc) != ESP_OK) {
        s_adc = nullptr;
        return;
    }
    adc_oneshot_chan_cfg_t chan = {};
    chan.atten = ADC_ATTEN_DB_12;
    chan.bitwidth = ADC_BITWIDTH_DEFAULT;
    adc_oneshot_config_channel(s_adc, s_adc_channel, &chan);
    adc_cali_curve_fitting_config_t cali = {};
    cali.unit_id = unit;
    cali.chan = s_adc_channel;
    cali.atten = ADC_ATTEN_DB_12;
    cali.bitwidth = ADC_BITWIDTH_DEFAULT;
    if (adc_cali_create_scheme_curve_fitting(&cali, &s_adc_cali) != ESP_OK) {
        s_adc_cali = nullptr;
    }
}

// Every peripheral enable low, and the buttons released for 60 ms, before a power transition (Seeed's order).
void PrepareToLeave()
{
    for (gpio_num_t pin : {bc::kTpRst, bc::kTpPwrEn, bc::kPdmEn}) {
        gpio_set_level(pin, 0);
    }
    TickType_t released_since = xTaskGetTickCount();
    for (;;) {
        const bool any = gpio_get_level(bc::kPwrButton) == 0 || gpio_get_level(bc::kButtonUp) == 0 ||
                         gpio_get_level(bc::kButtonDown) == 0;
        if (any) {
            released_since = xTaskGetTickCount();
        } else if (xTaskGetTickCount() - released_since >= pdMS_TO_TICKS(60)) {
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

}  // namespace

esp_err_t Init()
{
    // The latch first, so the board stays on once the button is released (STICKY_HARDWARE_SPECS §7).
    ESP_RETURN_ON_ERROR(HoldPower(), kTag, "hold power");

    esp_chip_info_t chip;
    esp_chip_info(&chip);
    ESP_LOGI(kTag, "%s rev v%d.%d, %d core(s)", CONFIG_IDF_TARGET, chip.revision / 100, chip.revision % 100,
             chip.cores);

    s_shared_bus = xSemaphoreCreateRecursiveMutex();
    ESP_RETURN_ON_FALSE(s_shared_bus != nullptr, ESP_ERR_NO_MEM, kTag, "shared bus lock");

    ESP_RETURN_ON_ERROR(IdleSharedBus(), kTag, "idle the shared bus");
    ESP_RETURN_ON_ERROR(Output(bc::kTpPwrEn, 0), kTag, "touch power off");
    ESP_RETURN_ON_ERROR(Output(bc::kPdmEn, 0), kTag, "mic power off");
    vTaskDelay(pdMS_TO_TICKS(100));

    const esp_err_t isr = gpio_install_isr_service(0);
    ESP_RETURN_ON_FALSE(isr == ESP_OK || isr == ESP_ERR_INVALID_STATE, isr, kTag, "GPIO ISR service");

    ESP_RETURN_ON_ERROR(InitSharedSpi(), kTag, "shared SPI2");
    // The sensor bus after boot: GPIO0 is its SCL and a boot strap (STICKY_HARDWARE_SPECS §1.1).
    ESP_RETURN_ON_ERROR(InitI2c(bc::kSensorI2cPort, bc::kMiscI2cSda, bc::kMiscI2cScl, &s_sensor_bus), kTag,
                        "sensor I2C");
    ESP_RETURN_ON_ERROR(InitI2c(bc::kTouchI2cPort, bc::kTpI2cSda, bc::kTpI2cScl, &s_touch_bus), kTag, "touch I2C");
    ESP_RETURN_ON_ERROR(InitCharger(), kTag, "charger");
    InitPowerSense();
    ESP_RETURN_ON_ERROR(Input(bc::kButtonUp, true), kTag, "UP");
    ESP_RETURN_ON_ERROR(Input(bc::kButtonDown, true), kTag, "DOWN");
    ESP_RETURN_ON_ERROR(Input(bc::kPwrButton, true), kTag, "AI/OK");
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(kTag, "board up: SPI2, I2C0 (touch), I2C1 (sensors), charger %s, USB %s",
             s_charging_enabled ? "on" : "off", UsbPresent() ? "present" : "absent");
    return ESP_OK;
}

i2c_master_bus_handle_t SensorBus()
{
    return s_sensor_bus;
}

i2c_master_bus_handle_t TouchBus()
{
    return s_touch_bus;
}

void LockSharedBus()
{
    xSemaphoreTakeRecursive(s_shared_bus, portMAX_DELAY);
}

void UnlockSharedBus()
{
    xSemaphoreGiveRecursive(s_shared_bus);
}

esp_err_t PowerTouch(bool on)
{
    ESP_RETURN_ON_ERROR(gpio_set_level(bc::kTpPwrEn, on ? 1 : 0), kTag, "touch power");
    if (on) {
        vTaskDelay(pdMS_TO_TICKS(bc::kTouchPowerSettleMs));
    }
    return ESP_OK;
}

esp_err_t SetChargingEnabled(bool enabled)
{
    ESP_RETURN_ON_ERROR(gpio_set_level(bc::kEnBatChg, enabled ? 0 : 1), kTag, "charger enable");
    s_charging_enabled = enabled;
    return ESP_OK;
}

bool ChargingEnabled()
{
    return s_charging_enabled;
}

bool ChargeStatusLow()
{
    return gpio_get_level(bc::kChargeState) == 0;
}

int PowerInputMillivolts()
{
    if (s_adc == nullptr || s_adc_cali == nullptr) {
        return -1;
    }
    int sum = 0;
    for (int i = 0; i < 8; ++i) {
        int raw = 0;
        if (adc_oneshot_read(s_adc, s_adc_channel, &raw) != ESP_OK) {
            return -1;
        }
        sum += raw;
    }
    int mv = -1;
    return adc_cali_raw_to_voltage(s_adc_cali, sum / 8, &mv) == ESP_OK ? mv : -1;
}

bool UsbPresent()
{
    // 200 mV or more on the pin counts as USB; the pin reads VIN halved (STICKY_HARDWARE_SPECS §5.4).
    return PowerInputMillivolts() >= 200;
}

esp_err_t EnterDeepSleep()
{
    PrepareToLeave();
    // Seeed's dashboard holds: the latch and the panel's power high; touch, mic, card and buzzer low.
    for (gpio_num_t pin : {bc::kPwrHold, bc::kPwrLock, bc::kEpPwrEn}) {
        gpio_set_level(pin, 1);
        gpio_hold_en(pin);
    }
    for (gpio_num_t pin : {bc::kTpPwrEn, bc::kTpRst, bc::kPdmEn, bc::kSdPwrEn, bc::kBuzzerPwm}) {
        gpio_set_level(pin, 0);
        gpio_hold_en(pin);
    }
    // Seeed's peripheral demo wakes on any of the three buttons, active low, with their RTC pull-ups on.
    const uint64_t mask = (1ULL << bc::kPwrButton) | (1ULL << bc::kButtonUp) | (1ULL << bc::kButtonDown);
    for (gpio_num_t pin : {bc::kPwrButton, bc::kButtonUp, bc::kButtonDown}) {
        rtc_gpio_pullup_en(pin);
    }
    ESP_RETURN_ON_ERROR(esp_sleep_enable_ext1_wakeup_io(mask, ESP_EXT1_WAKEUP_ANY_LOW), kTag, "wake source");
    gpio_deep_sleep_hold_en();
    ESP_LOGI(kTag, "deep sleep; press AI/OK, UP or DOWN to wake");
    vTaskDelay(pdMS_TO_TICKS(20));
    return esp_deep_sleep_try_to_start();
}

esp_err_t PowerOff()
{
    PrepareToLeave();
    ESP_LOGI(kTag, "power off: releasing the latch");
    gpio_hold_dis(bc::kPwrHold);
    gpio_hold_dis(bc::kPwrLock);
    // Seeed's release: HOLD low, then a 10 µs LOCK pulse (STICKY_HARDWARE_SPECS §5.3).
    gpio_set_level(bc::kPwrHold, 0);
    gpio_set_level(bc::kPwrLock, 0);
    esp_rom_delay_us(10);
    gpio_set_level(bc::kPwrLock, 1);
    esp_rom_delay_us(10);
    gpio_set_level(bc::kPwrLock, 0);
    vTaskDelay(pdMS_TO_TICKS(250));
    if (UsbPresent()) {
        ESP_LOGI(kTag, "USB still powers the board: restarting");
        esp_restart();
    }
    // Still running on battery: hold power again and say so.
    gpio_set_level(bc::kPwrHold, 1);
    gpio_set_level(bc::kPwrLock, 1);
    ESP_LOGE(kTag, "Main power did not turn off");
    return ESP_ERR_TIMEOUT;
}

}  // namespace board
