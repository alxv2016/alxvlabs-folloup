// The e-paper glass, as every controller presents it: a Panel that refreshes whole frames, and the Bus it talks over.
// The controllers are their own components, named after them — ssd1677 (V1.0) and ssd2677 (V2.0) — each driving
// the panel with the command sequences Seeed's drivers use (STICKY_HARDWARE_SPECS §4.5).
//
// A frame is the glass as the viewer sees it after the controller's own mirroring: 800 × 480, rows top to bottom,
// most significant bit first. A one-bit frame is 1 = white; a two-bit frame is 0 = black … 3 = white.
//
// Nothing here locks the shared SPI bus: the caller holds board::LockSharedBus() around every call.

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"

namespace epaper {

inline constexpr int kWidth = 800;
inline constexpr int kHeight = 480;
inline constexpr size_t kMonoStride = kWidth / 8;
inline constexpr size_t kMonoBytes = kMonoStride * kHeight;
inline constexpr size_t kGrayStride = kWidth / 4;
inline constexpr size_t kGrayBytes = kGrayStride * kHeight;

// --- The bus ---------------------------------------------------------------------------------------------------

struct BusConfig {
    spi_host_device_t host;
    gpio_num_t cs, dc, rst, busy;
    int read_hz;   // the controller's read limit
    int write_hz;  // its write limit
};

// SPI in half-duplex 3-wire mode, chip select driven by hand so one select serves a slow device for reads and a fast
// one for writes. The data/command line, reset and BUSY are plain GPIO.
class Bus {
public:
    esp_err_t Init(const BusConfig &config);

    esp_err_t Command(uint8_t command);
    esp_err_t Data(const uint8_t *data, size_t len);
    esp_err_t Write(uint8_t command, std::initializer_list<uint8_t> data);  // a command and its parameters
    esp_err_t Read(uint8_t *out);  // one byte, at the read clock, after its command

    // Streams `len` bytes of data in chunks: `fill(dst, offset, n)` writes bytes [offset, offset + n) into dst while
    // the previous chunk is on the wire.
    esp_err_t Stream(size_t len, const std::function<void(uint8_t *dst, size_t offset, size_t n)> &fill);

    void Reset(uint32_t low_ms, uint32_t high_ms);
    // Waits while BUSY reads `busy_level`.
    esp_err_t WaitReady(int busy_level, uint32_t timeout_ms);

private:
    esp_err_t Send(spi_device_handle_t device, const uint8_t *data, size_t len);

    BusConfig config_{};
    spi_device_handle_t read_ = nullptr;
    spi_device_handle_t write_ = nullptr;
    uint8_t *chunks_[2] = {};
};

// Reads the controller's ID with command 0x70 after a reset, as Seeed's probe does: the SSD2677 answers 0x07; the
// SSD1677 does not have the command.
esp_err_t ReadId(Bus &bus, uint8_t *id);

// --- The panel -------------------------------------------------------------------------------------------------

enum class Mode : uint8_t { kFull, kPartial, kGray };

// Rows and byte columns of a frame, inclusive. An empty box has row0 > row1.
struct Box {
    uint16_t row0 = 1, row1 = 0;
    uint16_t byte0 = 0, byte1 = 0;
    bool Empty() const { return row0 > row1; }
};

// Where the last refresh's time went, in ms.
struct Timing {
    uint32_t latch_ms = 0;     // choosing and loading the waveform
    uint32_t stream_ms = 0;    // the frame to the controller
    uint32_t power_on_ms = 0;  // the drive's supply, when it was off
    uint32_t drive_ms = 0;     // the waveform on the glass, until BUSY clears
    int temperature_c = -1;    // the waveform's temperature, -1 when the controller chose it itself
};

// A partial refresh's tuning, on controllers that offer it; 0 or false keeps the default. Each controller accepts only
// its own fields, with values from its datasheet, and refuses the rest.
struct Tuning {
    uint8_t frame_rate = 0;     // SSD2677: 0x30's FR code — 0x04 75 Hz, 0x05 85 Hz
    bool keep_waveform = false; // SSD1677: drive later partials with the waveform already loaded (0x22 = 0xCC)
    uint8_t scan_frames = 0;    // SSD1677: scans of our own waveform, this many frames each, 1 … 8; 0: no scans
    uint8_t scan_budget = 0;    // SSD1677: the scans a changed pixel gets to reach its colour, 2 … 15
};

class Panel {
public:
    virtual ~Panel() = default;

    virtual const char *Name() const = 0;
    // Draws the frame: one bit a pixel for kFull and kPartial, two for kGray. A partial with no full one before it
    // since start, sleep or a grey frame is drawn full.
    virtual esp_err_t Refresh(Mode mode, const uint8_t *frame) = 0;
    // Draws `frame` with the controller's own partial waveform, driving every pixel set in `touched` the whole way to
    // its colour in `frame` and leaving the rest alone — after scans, whose pixels may stand part-way. Controllers
    // without scans return ESP_ERR_NOT_SUPPORTED.
    virtual esp_err_t Settle(const uint8_t *frame, const uint8_t *touched)
    {
        (void)frame;
        (void)touched;
        return ESP_ERR_NOT_SUPPORTED;
    }
    // Scans: one pass of the controller's short waveform, pushing the pixels set in `black` toward black and those in
    // `white` toward white — never both — and holding the rest. `box` bounds both masks. The caller gives each changed
    // pixel a budget of scans (InkBoy's method). ScanReady() says whether the tuning and the temperature allow them.
    virtual bool ScanReady() const { return false; }
    virtual esp_err_t Scan(const uint8_t *black, const uint8_t *white, const Box &box)
    {
        (void)black;
        (void)white;
        (void)box;
        return ESP_ERR_NOT_SUPPORTED;
    }
    // Turns the drive's supply off; the next refresh turns it on again.
    virtual esp_err_t Standby() = 0;
    // Deep sleep: the frame stays on the glass, and the next refresh resets the controller first.
    virtual esp_err_t Sleep() = 0;

    virtual void SetTemperature(float celsius) { (void)celsius; }
    virtual esp_err_t SetTuning(const Tuning &tuning)
    {
        (void)tuning;
        return ESP_ERR_NOT_SUPPORTED;
    }

    const Timing &LastTiming() const { return timing_; }

protected:
    Timing timing_;
};

uint32_t ElapsedMs(int64_t since_us);

}  // namespace epaper
