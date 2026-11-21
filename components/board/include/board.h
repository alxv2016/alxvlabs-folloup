// The board's bring-up: the power latch first, then idle levels, the buses and the devices on them, in the order the
// hardware spec gives (STICKY_HARDWARE_SPECS §7) and Seeed's drivers follow. app_shell calls Init() first in Run(); a
// feature never brings hardware up itself (APP_ARCHITECTURE §2).

#pragma once

#include "driver/i2c_master.h"
#include "epaper.h"
#include "esp_err.h"

namespace board {

// Holds power on, sets every shared-bus line idle, and brings up SPI2 (the panel and the card), both I2C buses, the
// charger and the power-input sense. Returns the first error.
esp_err_t Init();

i2c_master_bus_handle_t SensorBus();  // BQ27220, PCF8563, LSM6DS3TR-C, SHT40 (STICKY_HARDWARE_SPECS §4.1)
i2c_master_bus_handle_t TouchBus();   // GT911

// SPI2 is shared by the panel and the card: hold this for a whole transaction on it (STICKY_HARDWARE_SPECS §1.2).
void LockSharedBus();
void UnlockSharedBus();

// --- The glass -------------------------------------------------------------------------------------------------

struct Display {
    epaper::Panel *panel = nullptr;  // owned by board, for the firmware's lifetime
    bool ssd2677 = false;            // V2.0; false is the SSD1677 of V1.0 (STICKY_HARDWARE_SPECS §4.5)
};

// Powers the panel, puts it on SPI2, reads the controller's ID and creates its driver — ssd2677 or ssd1677.
// `ambient_c`, when given, is the SSD2677's waveform temperature, so it never reads its own sensor.
esp_err_t InitDisplay(Display *out, const float *ambient_c);

// --- Touch power ----------------------------------------------------------------------------------------------

esp_err_t PowerTouch(bool on);

// --- Charging and power input (STICKY_HARDWARE_SPECS §5.4) -----------------------------------------------------

esp_err_t SetChargingEnabled(bool enabled);
bool ChargingEnabled();
bool ChargeStatusLow();     // the BQ25616's STAT, raw: low while charging
int PowerInputMillivolts(); // GPIO9, VIN halved; -1 when unread
bool UsbPresent();

// --- Leaving (STICKY_HARDWARE_SPECS §5.3, §6; Seeed's sequences) ---------------------------------------------------

// Deep sleep with every output held, woken by the AI/OK, UP or DOWN button; the panel unpowered unless
// `keep_panel_powered` — for a panel whose own sleep failed. Returns only on failure.
esp_err_t EnterDeepSleep(bool keep_panel_powered = false);

// Releases the power latch. On battery the rail falls and this never returns; on USB it restarts the chip, and
// anything else returns ESP_ERR_TIMEOUT with the board still powered.
esp_err_t PowerOff();

}  // namespace board
