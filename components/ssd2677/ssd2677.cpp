// The SSD2677's sequences are Seeed's, from its reference driver for the Sticky (MIT, Seeed Studio): the same
// initialisation, waveform slots (E0/E6/A5), stream encodings and power steps. What is ours:
//   - the controller is reset and initialised once, and again only after deep sleep or a failure; a change of mode is
//     a new waveform latch alone, as GxEPD2 drives this controller;
//   - a slot is latched only when it differs from the one loaded;
//   - a partial refresh may run at a fixed frame rate (0x30) from an allow-list: 50 to 85 Hz of the datasheet's codes;
//   - a partial refresh streams only the changed pixels' box, through the partial window (0x83, PTLW).
// No waveform can be loaded: PSR's LUT_EN (0x00's second byte, bit 7) is 0, so the OTP's load at reset stands.
// The controller does not mirror X itself here, so each row is streamed right to left.
//
// Only what the datasheet documents or Seeed's driver sends is sent; 0xA5 as a command, 0x62, 0xE7, 0xE9 and 0xE0's bit
// 4 are Seeed's alone, undocumented. 0x90 and 0x91 enter OTP programming and burn the OTP; they must never appear in
// this file (STICKY_EPAPER_WAVEFORMS §2, §3.1).

#include "ssd2677.h"

#include <algorithm>
#include <cstring>

#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace ssd2677 {
namespace {

using epaper::Mode;
constexpr const char *kTag = "ssd2677";

constexpr int kBusyLevel = 0;  // BUSY_N low while the controller works
constexpr uint32_t kBusyTimeoutMs = 30000;
constexpr uint32_t kResetLowMs = 20;
constexpr uint32_t kResetHighMs = 10;
constexpr uint8_t kFrameRateOtp = 0x08;  // bit 3, the dynamic frame rate, as Seeed initialises it

// Seeed's slots: forced temperatures (TS_SET, a signed byte in °C) that select the OTP's full, partial and grey
// waveforms — full −24 to −15 °C, partial 8 to 35 °C, grey −15 and −6 °C (STICKY_EPAPER_WAVEFORMS §3.1).
uint8_t FullSlot(int c)
{
    return c <= 10 ? 0xE8 : c <= 20 ? 0xEB : c <= 30 ? 0xEE : 0xF1;
}
uint8_t PartialSlot(int c)
{
    return c <= 10 ? 0x08 : c <= 20 ? 0x0F : c <= 30 ? 0x19 : 0x23;
}
uint8_t GraySlot(int c)
{
    return c <= 15 ? 0xF1 : 0xFA;
}

// Spreads a byte's bits to the low bit of each two-bit pair, most significant first: 1011… → 01 00 01 01 ….
uint16_t Spread(uint8_t b)
{
    uint16_t out = 0;
    for (int i = 7; i >= 0; --i) {
        out = static_cast<uint16_t>((out << 2) | ((b >> i) & 1));
    }
    return out;
}

uint8_t Reverse8(uint8_t b)
{
    b = static_cast<uint8_t>((b & 0xF0) >> 4 | (b & 0x0F) << 4);
    b = static_cast<uint8_t>((b & 0xCC) >> 2 | (b & 0x33) << 2);
    return static_cast<uint8_t>((b & 0xAA) >> 1 | (b & 0x55) << 1);
}

// Reverses the order of four two-bit pixels in a byte.
uint8_t Reverse4x2(uint8_t b)
{
    b = static_cast<uint8_t>((b & 0xF0) >> 4 | (b & 0x0F) << 4);
    return static_cast<uint8_t>((b & 0xCC) >> 2 | (b & 0x33) << 2);
}

class Ssd2677 final : public epaper::Panel {
public:
    explicit Ssd2677(epaper::Bus &bus) : bus_(bus)
    {
        // Per source byte, mirrored: its bits reversed, then spread to pairs. A one-bit pixel to two is white 11 and
        // black 00 (spread × 3); old and new together are the old pixel's bit high, the new one's low.
        for (int i = 0; i < 256; ++i) {
            spread_[i] = Spread(Reverse8(static_cast<uint8_t>(i)));
        }
    }
    ~Ssd2677() override { heap_caps_free(previous_); }

    esp_err_t Allocate()
    {
        previous_ = static_cast<uint8_t *>(heap_caps_malloc(epaper::kMonoBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        return previous_ != nullptr ? ESP_OK : ESP_ERR_NO_MEM;
    }

    const char *Name() const override { return "SSD2677"; }

    void SetTemperature(float celsius) override
    {
        temperature_c_ = celsius <= 0 ? 0 : celsius >= 127 ? 127 : static_cast<int>(celsius + 0.5f);
    }

    esp_err_t SetTuning(const epaper::Tuning &tuning) override
    {
        ESP_RETURN_ON_FALSE(!tuning.keep_waveform && tuning.scan_frames == 0, ESP_ERR_INVALID_ARG, kTag,
                            "the SSD1677's tuning");
        const uint8_t rate = tuning.frame_rate;
        // 0x30's FR codes per the datasheet: 0x02 50 Hz … 0x05 85 Hz; 0 or 0x08 keep Seeed's dynamic frame rate.
        ESP_RETURN_ON_FALSE(rate == 0 || rate == kFrameRateOtp || (rate >= 0x02 && rate <= 0x05), ESP_ERR_INVALID_ARG,
                            kTag, "frame rate 0x%02X not allowed", rate);
        partial_rate_ = rate == 0 ? kFrameRateOtp : rate;
        return ESP_OK;
    }

    esp_err_t Refresh(Mode mode, const uint8_t *frame) override
    {
        if (mode == Mode::kPartial && !partial_ready_) {
            mode = Mode::kFull;
        }
        timing_ = {};
        esp_err_t err = initialized_ ? ESP_OK : Init();
        if (err == ESP_OK) {
            err = Draw(mode, frame);
        }
        if (err != ESP_OK) {
            initialized_ = false;  // the next refresh starts from a reset
            partial_ready_ = false;
        }
        return err;
    }

    // The controller stays powered between refreshes: its power-off (0x02) lets black fade within seconds on the
    // glass, as Seeed's driver does too, whatever PSR's ground ties are set to. Deep sleep powers it off
    // (STICKY_EPAPER_WAVEFORMS §3.1, §6).
    esp_err_t Standby() override { return ESP_OK; }

    esp_err_t Sleep() override
    {
        if (initialized_) {
            ESP_RETURN_ON_ERROR(PowerOff(), kTag, "power off");
            ESP_RETURN_ON_ERROR(bus_.Write(0x07, {0xA5}), kTag, "deep sleep");
        }
        initialized_ = false;
        partial_ready_ = false;
        return ESP_OK;
    }

private:
    esp_err_t Wait() { return bus_.WaitReady(kBusyLevel, kBusyTimeoutMs); }

    // Seeed's init_otp: panel setting, booster, the waveform's timings, the resolution, and the OTP's LUT.
    esp_err_t Init()
    {
        bus_.Reset(kResetLowMs, kResetHighMs);
        window_on_ = false;  // a reset turns partial mode off
        ESP_RETURN_ON_ERROR(Wait(), kTag, "reset");
        ESP_RETURN_ON_ERROR(bus_.Write(0x00, {0x2F, 0x0E}), kTag, "panel setting");
        ESP_RETURN_ON_ERROR(Wait(), kTag, "panel setting");
        ESP_RETURN_ON_ERROR(bus_.Write(0x06, {0x0F, 0x8B, 0x93, 0xC1}), kTag, "booster");
        ESP_RETURN_ON_ERROR(bus_.Write(0xE7, {0xC1}), kTag, "E7");
        ESP_RETURN_ON_ERROR(bus_.Write(0x30, {kFrameRateOtp}), kTag, "frame rate");
        ESP_RETURN_ON_ERROR(bus_.Write(0x50, {0x77}), kTag, "VCOM and data interval");
        ESP_RETURN_ON_ERROR(bus_.Write(0x62, {0x76, 0x76, 0x76, 0x5A, 0x9D, 0x8A, 0x76, 0x62}), kTag, "timing");
        ESP_RETURN_ON_ERROR(bus_.Write(0x61, {0x03, 0x20, 0x02, 0xA8}), kTag, "resolution");
        ESP_RETURN_ON_ERROR(bus_.Write(0xE0, {0x10}), kTag, "E0");
        ESP_RETURN_ON_ERROR(bus_.Write(0x65, {0x00, 0x00, 0x00, 0x00}), kTag, "gate start");
        ESP_RETURN_ON_ERROR(bus_.Write(0xE9, {0x01}), kTag, "E9");
        ESP_RETURN_ON_ERROR(Wait(), kTag, "init");
        initialized_ = true;
        powered_ = false;
        latched_ = 0;
        rate_ = kFrameRateOtp;
        return ESP_OK;
    }

    // The waveform's temperature: the SHT40's when given, else the controller's own sensor, read at the read clock.
    esp_err_t Temperature(int *c)
    {
        if (temperature_c_ >= 0) {
            *c = temperature_c_;
            return ESP_OK;
        }
        uint8_t raw = 0;
        ESP_RETURN_ON_ERROR(bus_.Command(0x40), kTag, "temperature");
        ESP_RETURN_ON_ERROR(Wait(), kTag, "temperature");
        ESP_RETURN_ON_ERROR(bus_.Read(&raw), kTag, "temperature");
        *c = raw > 127 ? 0 : raw;
        return ESP_OK;
    }

    esp_err_t Latch(uint8_t slot)
    {
        if (slot == latched_) {
            return ESP_OK;
        }
        // TSFIX: the temperature from 0xE6, a signed byte in °C. 0x12 also sets bit 4, and 0xA5 is no command in the
        // datasheet — both are Seeed's. 0xA5 is what loads the forced temperature's waveform: without it a full
        // refresh drives 406 ms, not 876 (STICKY_EPAPER_WAVEFORMS §2, §3.1, §4.1).
        ESP_RETURN_ON_ERROR(bus_.Write(0xE0, {0x12}), kTag, "E0");
        ESP_RETURN_ON_ERROR(bus_.Write(0xE6, {slot}), kTag, "E6");
        ESP_RETURN_ON_ERROR(bus_.Command(0xA5), kTag, "A5");
        ESP_RETURN_ON_ERROR(Wait(), kTag, "A5");
        vTaskDelay(pdMS_TO_TICKS(10));
        latched_ = slot;
        return ESP_OK;
    }

    // The partial window, 0x83 (PTLW): partial mode on for a box, off for a full or grey refresh. Source addresses run
    // right to left (s = 799 - x, since rows stream that way) and the gates are the rows. The gates still scan the
    // whole glass, so only the upload shrinks (STICKY_EPAPER_WAVEFORMS §3.1, §4.1).
    esp_err_t SetWindow(const epaper::Box *box)
    {
        if (box == nullptr) {
            if (!window_on_) {
                return ESP_OK;
            }
            window_on_ = false;
            return bus_.Write(0x83, {0x00, 0x00, 0x03, 0xBF, 0x00, 0x00, 0x02, 0xA7, 0x00});  // POR window, mode off
        }
        const uint16_t hrst = static_cast<uint16_t>((epaper::kMonoStride - 1 - box->byte1) * 8);
        const uint16_t hred = static_cast<uint16_t>((epaper::kMonoStride - 1 - box->byte0) * 8 + 7);
        window_on_ = true;
        return bus_.Write(0x83, {static_cast<uint8_t>(hrst >> 8), static_cast<uint8_t>(hrst),
                                 static_cast<uint8_t>(hred >> 8), static_cast<uint8_t>(hred),
                                 static_cast<uint8_t>(box->row0 >> 8), static_cast<uint8_t>(box->row0),
                                 static_cast<uint8_t>(box->row1 >> 8), static_cast<uint8_t>(box->row1), 0x01});
    }

    // The rows and source bytes where `frame` differs from the glass; Empty() when it does not.
    epaper::Box ChangedBox(const uint8_t *frame) const
    {
        epaper::Box box;
        box.row0 = epaper::kHeight;
        box.byte0 = epaper::kMonoStride;
        for (uint16_t r = 0; r < epaper::kHeight; ++r) {
            const uint8_t *now = frame + r * epaper::kMonoStride;
            const uint8_t *was = previous_ + r * epaper::kMonoStride;
            if (std::memcmp(now, was, epaper::kMonoStride) == 0) {
                continue;
            }
            for (uint16_t b = 0; b < epaper::kMonoStride; ++b) {
                if (now[b] != was[b]) {
                    box.row0 = std::min(box.row0, r);
                    box.row1 = std::max(box.row1, r);
                    box.byte0 = std::min(box.byte0, b);
                    box.byte1 = std::max(box.byte1, b);
                }
            }
        }
        return box;
    }

    // A partial's two bits a pixel, old and new interleaved, for the box alone — each row right to left. A source
    // byte is two output bytes: every row and every chunk Bus::Stream hands over is even, so a pair never splits.
    esp_err_t StreamBox(const uint8_t *frame, const epaper::Box &box)
    {
        const size_t row_out = (box.byte1 - box.byte0 + 1) * 2;
        ESP_RETURN_ON_ERROR(bus_.Command(0x10), kTag, "data");
        return bus_.Stream(row_out * (box.row1 - box.row0 + 1), [&](uint8_t *dst, size_t offset, size_t n) {
            for (size_t k = 0; k < n; k += 2) {
                const size_t at = offset + k;
                const size_t src = (box.row0 + at / row_out) * epaper::kMonoStride + box.byte1 - (at % row_out) / 2;
                const uint16_t wide = static_cast<uint16_t>((spread_[previous_[src]] << 1) | spread_[frame[src]]);
                dst[k] = static_cast<uint8_t>(wide >> 8);
                dst[k + 1] = static_cast<uint8_t>(wide);
            }
        });
    }

    esp_err_t PowerOff()
    {
        if (!powered_) {
            return ESP_OK;
        }
        ESP_RETURN_ON_ERROR(bus_.Write(0x02, {0x00}), kTag, "power off");
        ESP_RETURN_ON_ERROR(Wait(), kTag, "power off");
        powered_ = false;
        return ESP_OK;
    }

    // Streams the frame as two bits a pixel, each row right to left. Chunks hold whole source bytes: they are even.
    esp_err_t Stream(Mode mode, const uint8_t *frame)
    {
        ESP_RETURN_ON_ERROR(bus_.Command(0x10), kTag, "data");
        return bus_.Stream(epaper::kGrayBytes, [&](uint8_t *dst, size_t offset, size_t n) {
            size_t row = offset / epaper::kGrayStride;
            size_t col = offset % epaper::kGrayStride;  // output byte in the row, from the left
            if (mode == Mode::kGray) {
                for (size_t k = 0; k < n; ++k) {
                    dst[k] = Reverse4x2(frame[row * epaper::kGrayStride + (epaper::kGrayStride - 1 - col)]);
                    if (++col == epaper::kGrayStride) {
                        col = 0;
                        ++row;
                    }
                }
                return;
            }
            const bool full = mode == Mode::kFull;
            for (size_t k = 0; k < n; k += 2) {
                const size_t src = row * epaper::kMonoStride + (epaper::kMonoStride - 1 - col / 2);
                const uint16_t cur = spread_[frame[src]];
                const uint16_t wide = full ? static_cast<uint16_t>(cur * 3)
                                           : static_cast<uint16_t>((spread_[previous_[src]] << 1) | cur);
                dst[k] = static_cast<uint8_t>(wide >> 8);
                dst[k + 1] = static_cast<uint8_t>(wide);
                col += 2;
                if (col == epaper::kGrayStride) {
                    col = 0;
                    ++row;
                }
            }
        });
    }

    esp_err_t Draw(Mode mode, const uint8_t *frame)
    {
        // A full starts from the supply off, as one after a greyscale refresh does: the frame streams unpowered (and
        // the waveform latches, when its slot changed), then the supply powers on to drive — about 83 ms more a full.
        // It lessens the pale right edge of a V2.0 board's black flash (STICKY_HARDWARE_SPECS §8.1).
        if (mode == Mode::kFull) {
            ESP_RETURN_ON_ERROR(PowerOff(), kTag, "power off before a full");
        }
        int64_t t = esp_timer_get_time();
        int c = 25;
        ESP_RETURN_ON_ERROR(Temperature(&c), kTag, "temperature");
        timing_.temperature_c = c;
        const uint8_t slot = mode == Mode::kGray ? GraySlot(c) : mode == Mode::kFull ? FullSlot(c) : PartialSlot(c);
        ESP_RETURN_ON_ERROR(Latch(slot), kTag, "latch");
        timing_.latch_ms = epaper::ElapsedMs(t);

        t = esp_timer_get_time();
        const epaper::Box box = mode == Mode::kPartial && partial_ready_ ? ChangedBox(frame) : epaper::Box{};
        if (!box.Empty()) {
            ESP_RETURN_ON_ERROR(SetWindow(&box), kTag, "window");
            ESP_RETURN_ON_ERROR(StreamBox(frame, box), kTag, "stream");
        } else {
            ESP_RETURN_ON_ERROR(SetWindow(nullptr), kTag, "window off");
            ESP_RETURN_ON_ERROR(Stream(mode, frame), kTag, "stream");
        }
        timing_.stream_ms = epaper::ElapsedMs(t);

        const uint8_t rate = mode == Mode::kPartial ? partial_rate_ : kFrameRateOtp;
        if (rate != rate_) {
            ESP_RETURN_ON_ERROR(bus_.Write(0x30, {rate}), kTag, "frame rate");
            rate_ = rate;
        }
        if (!powered_) {
            t = esp_timer_get_time();
            ESP_RETURN_ON_ERROR(bus_.Command(0x04), kTag, "power on");
            ESP_RETURN_ON_ERROR(Wait(), kTag, "power on");
            timing_.power_on_ms = epaper::ElapsedMs(t);
            powered_ = true;
        }
        t = esp_timer_get_time();
        ESP_RETURN_ON_ERROR(bus_.Write(0x12, {0x00}), kTag, "refresh");
        ESP_RETURN_ON_ERROR(Wait(), kTag, "refresh");
        timing_.drive_ms = epaper::ElapsedMs(t);

        if (mode == Mode::kGray) {
            ESP_RETURN_ON_ERROR(PowerOff(), kTag, "power off");  // Seeed's: the grey drive does not stay on
            partial_ready_ = false;
        } else {
            std::memcpy(previous_, frame, epaper::kMonoBytes);
            partial_ready_ = true;
        }
        return ESP_OK;
    }

    epaper::Bus &bus_;
    bool initialized_ = false;
    bool powered_ = false;
    bool partial_ready_ = false;
    bool window_on_ = false;  // PTLW's partial mode is on in the controller
    uint8_t latched_ = 0;
    uint8_t rate_ = kFrameRateOtp;
    uint8_t partial_rate_ = kFrameRateOtp;
    int temperature_c_ = -1;
    uint8_t *previous_ = nullptr;  // the one-bit frame on the glass
    uint16_t spread_[256] = {};
};

}  // namespace

std::unique_ptr<epaper::Panel> Create(epaper::Bus &bus)
{
    auto panel = std::make_unique<Ssd2677>(bus);
    if (panel->Allocate() != ESP_OK) {
        return nullptr;
    }
    return panel;
}

}  // namespace ssd2677
