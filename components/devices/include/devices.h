// The Sticky's devices beyond the glass and touch, each read the way Seeed's drivers and demos do: the SHT40, the
// LSM6DS3TR-C, the BQ27220, the PCF8563 on the sensor bus; the buzzer; the PDM microphone; the MicroSD card on the
// shared SPI2 bus (STICKY_HARDWARE_SPECS §4). Call Init() once the board is up. Every read is safe from any one task at
// a time; none is called from an interrupt.

#pragma once

#include <cstdint>
#include <ctime>

#include "esp_err.h"
#include "sounds.h"

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
// Sets the RTC to `when`, its seconds at 0 and its date as given — the date a ReadClock returned, kept (STICKY_HARDWARE_SPECS §4). One
// block write of the seven time registers, which also clears the oscillator-stop flag a never-set clock reads with. ESP_ERR_INVALID_STATE
// with no RTC, ESP_FAIL when the write or the date is refused (a date outside 1900 to 2099 or a day the month lacks).
esp_err_t SetClock(const std::tm &when);

// Whether the RTC's timer or alarm flag was set at boot — RTC_INTn low, which powers the board on from off
// (STICKY_HARDWARE_SPECS §5.2). Init reads the flags, then clears them, so RTC_INTn rests high.
bool RtcStartedBoard();
// Stops the RTC's timer and alarm, clears their flags and disables both interrupts: before any power-off, or an
// interrupt left low powers the board straight back on (STICKY_HARDWARE_SPECS §5.3).
esp_err_t ClearRtcWake();
// Counts down `seconds` (1–255) at 1 Hz, then holds RTC_INTn low until cleared — a power-on from off.
esp_err_t SetRtcWake(uint8_t seconds);

// --- The buzzer --------------------------------------------------------------------------------------------------

// One sound at a time, from a task of the buzzer's own; every call returns at once, from any task but an interrupt, and
// cuts whatever is playing — a tick never waits. A feature sounds an outcome by its word, DESIGN.md's sound token
// (sounds.h); a sound of its own is read as it plays, so pass a table that outlives it (a constexpr one). The format and
// how it maps to LEDC are sound_rules.h's.
void Play(sound::Word word);
void Play(const sound::Sound &sound);
void Silence();
void Tick();             // the press feedback, sound.tick: 30 ms at 4 kHz
void Tone(uint32_t ms);  // the full square at 4 kHz for this long, up to 65535 ms

// --- The PDM microphone ------------------------------------------------------------------------------------------

// Powers the microphone, lets it settle, reads `ms` of 16 kHz mono, powers it off, and gives the level as RMS and its peak.
esp_err_t ListenLevel(uint32_t ms, float *rms, int *peak);
// The same in steps, for a run of levels read with one settling (the console's `sound band`): StartListening powers the
// microphone and drops at least `settle_ms` of samples — its power-up and its filter's start, never in a level — and
// ReadLevel reads levels until StopListening powers it off. One listener at a time.
esp_err_t StartListening(uint32_t settle_ms);
esp_err_t ReadLevel(uint32_t ms, float *rms, int *peak);
void StopListening();

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
