// The Sticky's devices beyond the glass and touch, each read the way Seeed's drivers and demos do: the SHT40, the
// LSM6DS3TR-C, the BQ27220, the PCF8563 on the sensor bus; the buzzer; the PDM microphone; the MicroSD card on the
// shared SPI2 bus (STICKY_HARDWARE_SPECS §4). Call Init() once the board is up. Every read is safe from any one task at
// a time; none is called from an interrupt.

#pragma once

#include <cstdint>
#include <ctime>

#include "esp_err.h"

namespace devices {

struct Present {
    bool sht40 = false;
    bool imu = false;
    bool gauge = false;
    bool rtc = false;
    bool buzzer = false;
};

// Adds every device to its bus and probes it; a device that does not answer is marked absent, never an error.
esp_err_t Init();
Present Found();

// --- SHT40 -------------------------------------------------------------------------------------------------------

struct Ambient {
    float temperature_c = 0;
    float humidity_pct = 0;
};
esp_err_t ReadAmbient(Ambient *out);

// --- LSM6DS3TR-C -------------------------------------------------------------------------------------------------

struct Motion {
    float accel_g[3] = {};
    float gyro_dps[3] = {};
};
esp_err_t ReadMotion(Motion *out);

// --- BQ27220 -----------------------------------------------------------------------------------------------------

struct Battery {
    int voltage_mv = 0;
    int current_ma = 0;  // negative while discharging
    int percent = 0;
    int health_pct = 0;
    int remaining_mah = 0;
    int full_mah = 0;
    float temperature_c = 0;
    int cycles = 0;
    bool scaled = false;  // the stock profile reads ten times high, and is divided (STICKY_HARDWARE_SPECS §4.3)
};
esp_err_t ReadBattery(Battery *out);

// --- PCF8563 -----------------------------------------------------------------------------------------------------

esp_err_t ReadClock(std::tm *out);

// --- The buzzer --------------------------------------------------------------------------------------------------

void Tick();                       // the press feedback: a short tone, returns at once
void Tone(uint32_t ms);            // a tone of this length, returns at once

// --- The PDM microphone ------------------------------------------------------------------------------------------

// Powers the microphone, reads `ms` of 16 kHz mono, powers it off, and gives the level as RMS and its peak.
esp_err_t ListenLevel(uint32_t ms, float *rms, int *peak);

// --- MicroSD -----------------------------------------------------------------------------------------------------

struct Card {
    bool inserted = false;
    bool mounted = false;
    uint64_t capacity_mib = 0;
    int entries = 0;  // in the root, up to 99
    char name[16] = {};
};
// Checks card detect, mounts at /sdcard, reads the size and the root, and unmounts — the shared bus stays up.
esp_err_t ProbeCard(Card *out);

}  // namespace devices
