// The SSD1677's sequences are Seeed's, from its reference driver for the Sticky (MIT, Seeed Studio): the same
// commands, values and order for full, partial and grey refreshes. What is ours is the plumbing around them — the
// frame comes in whole, and the panel keeps the frame it showed for the next partial's old plane — and two additions:
//
// A partial may keep the waveform it loaded (0x22 = 0xCC: clock, analog, mode 2, display — no temperature read, no LUT
// load); it is reloaded after every full refresh, which resets the controller, and after a standby.
//
// Scans, InkBoy's method (github.com/kuslota/InkBoy) on this controller: the two RAM planes hold masks, not images —
// 0x26 the pixels to push toward black, 0x24 those to push toward white, both clear to hold — and each scan runs a few
// frames of our own waveform (0x32, the SSD1677 datasheet's sections 6.6–6.7): LUT1 (0x26 clear, 0x24 set) at VSL, toward white;
// LUT2 (0x26 set, 0x24 clear) at VSH1, toward black; LUT0 and LUT3 hold. The polarity was seen on a V1.0 board's
// glass: the Xteink X4 table's opposite sense drove every pixel the wrong way, so the bar's past positions turned black
// and its present one stayed white. The caller gives each changed pixel a budget of scans in one direction, so a pixel that goes and comes
// back nets no charge. Scans keep the clock and the drive's supply on and start with the display step alone (0x0C),
// upload only the rows and byte columns in play, and run only between 15 and 35 °C by the SHT40. The panel's own
// voltages and VCOM stay: after each reset the OTP waveform and voltages are loaded first (0x22 = 0xB9) and only the
// waveform bytes 0–104 are replaced; 0x03, 0x04 and 0x2C are never sent. Nothing is written to the OTP: its
// programming commands (0x30, 0x36, 0x2A, 0x08) are never sent (STICKY_EPAPER_WAVEFORMS §2, §3.3).
//
// The controller mirrors X itself (data entry 0x02 and a reversed RAM X window), so frames go in as given: a frame's
// column c is RAM X 799 − c.

#include "ssd1677.h"

#include <algorithm>
#include <cstring>

#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace ssd1677 {
namespace {

using epaper::Mode;
constexpr const char *kTag = "ssd1677";

constexpr int kBusyLevel = 1;  // BUSY high while the controller works
constexpr uint32_t kBusyTimeoutMs = 8000;
constexpr uint32_t kResetLowMs = 10;
constexpr uint32_t kResetHighMs = 10;
constexpr uint8_t kScanRate = 0x44;  // the frame-rate byte the X4's fast table uses: about 10 ms a frame here

constexpr uint16_t kXEnd = epaper::kWidth - 1;
constexpr uint16_t kYEnd = epaper::kHeight - 1;
constexpr uint16_t kLastByte = epaper::kMonoStride - 1;

class Ssd1677 final : public epaper::Panel {
public:
    explicit Ssd1677(epaper::Bus &bus) : bus_(bus) {}
    ~Ssd1677() override
    {
        heap_caps_free(previous_);
        heap_caps_free(planes_);
    }

    esp_err_t Allocate()
    {
        previous_ = static_cast<uint8_t *>(heap_caps_malloc(epaper::kMonoBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        planes_ = static_cast<uint8_t *>(heap_caps_malloc(2 * epaper::kMonoBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        return previous_ != nullptr && planes_ != nullptr ? ESP_OK : ESP_ERR_NO_MEM;
    }

    const char *Name() const override { return "SSD1677"; }

    void SetTemperature(float celsius) override { temperature_c_ = celsius; }

    esp_err_t SetTuning(const epaper::Tuning &tuning) override
    {
        ESP_RETURN_ON_FALSE(tuning.frame_rate == 0, ESP_ERR_INVALID_ARG, kTag, "no frame-rate setting");
        ESP_RETURN_ON_FALSE(tuning.scan_frames <= 8, ESP_ERR_INVALID_ARG, kTag, "scans of %u frames not allowed",
                            tuning.scan_frames);
        ESP_RETURN_ON_FALSE(tuning.scan_frames == 0 || (tuning.scan_budget >= 2 && tuning.scan_budget <= 15),
                            ESP_ERR_INVALID_ARG, kTag, "a budget of %u scans not allowed", tuning.scan_budget);
        tuning_ = tuning;
        custom_loaded_ = false;
        lut_loaded_ = false;
        return ESP_OK;
    }

    bool ScanReady() const override
    {
        return tuning_.scan_frames != 0 && partial_ready_ && temperature_c_ >= 15.0f && temperature_c_ <= 35.0f;
    }

    esp_err_t Refresh(Mode mode, const uint8_t *frame) override
    {
        if (mode == Mode::kPartial && !partial_ready_) {
            mode = Mode::kFull;
        }
        timing_ = {};
        const esp_err_t err = mode == Mode::kGray   ? Gray(frame)
                              : mode == Mode::kFull ? Full(frame)
                                                    : Partial(frame, previous_);
        return Fail(err);
    }

    esp_err_t Settle(const uint8_t *frame, const uint8_t *touched) override
    {
        if (!partial_ready_) {
            timing_ = {};
            return Fail(Full(frame));
        }
        // The old plane says each touched pixel was the opposite colour, so the OTP waveform drives it fully.
        uint8_t *old_plane = planes_;
        for (size_t i = 0; i < epaper::kMonoBytes; ++i) {
            old_plane[i] = static_cast<uint8_t>(frame[i] ^ touched[i]);
        }
        timing_ = {};
        return Fail(Partial(frame, old_plane));
    }

    esp_err_t Scan(const uint8_t *black, const uint8_t *white, const epaper::Box &box) override
    {
        ESP_RETURN_ON_FALSE(ScanReady(), ESP_ERR_INVALID_STATE, kTag, "scans are off");
        timing_ = {};
        return Fail(DoScan(black, white, box));
    }

    esp_err_t Standby() override
    {
        if (mode_ == kNone || standby_) {
            return ESP_OK;
        }
        // Already off — a 0xFF partial turns the clock and the drive's supply off at its end — so nothing is driven:
        // 0x03 with the clock off holds BUSY until the timeout (STICKY_EPAPER_WAVEFORMS §6).
        if (powered_) {
            ESP_RETURN_ON_ERROR(Fail(DoStandby()), kTag, "standby");
        }
        standby_ = true;
        after_standby_ = true;
        lut_loaded_ = false;
        return ESP_OK;
    }

    esp_err_t Sleep() override
    {
        ESP_RETURN_ON_ERROR(bus_.Write(0x10, {0x03}), kTag, "deep sleep");
        vTaskDelay(pdMS_TO_TICKS(100));
        mode_ = kNone;
        partial_ready_ = false;
        custom_loaded_ = false;
        ram_masks_ = false;
        return ESP_OK;
    }

private:
    enum State : uint8_t { kNone, kFullMode, kPartialMode, kGrayMode };

    esp_err_t Wait() { return bus_.WaitReady(kBusyLevel, kBusyTimeoutMs); }

    esp_err_t DoStandby()
    {
        ESP_RETURN_ON_ERROR(bus_.Write(0x22, {0x03}), kTag, "standby");  // analog and clock off
        powered_ = false;
        ESP_RETURN_ON_ERROR(bus_.Command(0x20), kTag, "standby");
        return Wait();
    }

    esp_err_t Fail(esp_err_t err)
    {
        if (err != ESP_OK) {
            mode_ = kNone;  // the next refresh starts from a reset, and a full one
            partial_ready_ = false;
            ram_masks_ = false;
        }
        return err;
    }

    // Seeed's init_base: a reset (not for grey), a software reset (full and grey), then the panel's geometry and the
    // border waveform for the mode.
    esp_err_t Init(State state)
    {
        powered_ = false;
        ram_masks_ = false;
        if (state != kGrayMode) {
            bus_.Reset(kResetLowMs, kResetHighMs);
            ESP_RETURN_ON_ERROR(Wait(), kTag, "reset");
        }
        if (state == kFullMode || state == kGrayMode) {
            ESP_RETURN_ON_ERROR(bus_.Command(0x12), kTag, "software reset");
            ESP_RETURN_ON_ERROR(Wait(), kTag, "software reset");
        }
        const uint8_t border = state == kGrayMode ? 0x00 : state == kPartialMode ? 0x80 : 0x01;
        ESP_RETURN_ON_ERROR(bus_.Write(0x18, {0x80}), kTag, "temperature sensor");  // internal
        ESP_RETURN_ON_ERROR(bus_.Write(0x3C, {border}), kTag, "border");
        ESP_RETURN_ON_ERROR(bus_.Write(0x0C, {0xAE, 0xC7, 0xC3, 0xC0, 0x80}), kTag, "soft start");
        ESP_RETURN_ON_ERROR(bus_.Write(0x01, {kYEnd & 0xFF, kYEnd >> 8, 0x02}), kTag, "driver output");
        ESP_RETURN_ON_ERROR(bus_.Write(0x11, {0x02}), kTag, "data entry");  // X decrements: the mirror
        ESP_RETURN_ON_ERROR(Window({0, kYEnd, 0, kLastByte}), kTag, "window");
        ESP_RETURN_ON_ERROR(Wait(), kTag, "init");
        mode_ = state;
        standby_ = false;
        after_standby_ = false;
        lut_loaded_ = false;
        custom_loaded_ = false;
        return ESP_OK;
    }

    // A RAM window: frame rows row0…row1, frame bytes byte0…byte1 — RAM X from 799 − 8·byte0 down.
    esp_err_t Window(const epaper::Box &b)
    {
        const uint16_t x_start = static_cast<uint16_t>(kXEnd - 8 * b.byte0);
        const uint16_t x_end = static_cast<uint16_t>(kXEnd - (8 * b.byte1 + 7));
        ESP_RETURN_ON_ERROR(bus_.Write(0x44, {static_cast<uint8_t>(x_start & 0xFF), static_cast<uint8_t>(x_start >> 8),
                                              static_cast<uint8_t>(x_end & 0xFF), static_cast<uint8_t>(x_end >> 8)}),
                            kTag, "RAM X");
        ESP_RETURN_ON_ERROR(bus_.Write(0x45, {static_cast<uint8_t>(b.row0 & 0xFF), static_cast<uint8_t>(b.row0 >> 8),
                                              static_cast<uint8_t>(b.row1 & 0xFF), static_cast<uint8_t>(b.row1 >> 8)}),
                            kTag, "RAM Y");
        ESP_RETURN_ON_ERROR(bus_.Write(0x4E, {static_cast<uint8_t>(x_start & 0xFF), static_cast<uint8_t>(x_start >> 8)}),
                            kTag, "RAM X counter");
        return bus_.Write(0x4F, {static_cast<uint8_t>(b.row0 & 0xFF), static_cast<uint8_t>(b.row0 >> 8)});
    }

    // Takes the controller out of standby when it is already in the mode; else initialises it for the mode.
    esp_err_t Prepare(State state)
    {
        if (mode_ == state) {
            if (standby_) {
                ESP_RETURN_ON_ERROR(bus_.Write(0x22, {0xC0}), kTag, "resume");  // clock and analog on
                powered_ = true;
                ESP_RETURN_ON_ERROR(bus_.Command(0x20), kTag, "resume");
                ESP_RETURN_ON_ERROR(Wait(), kTag, "resume");
                standby_ = false;
            }
            return ESP_OK;
        }
        return Init(state);
    }

    // The window's bytes of a plane: rows row0…row1, bytes byte0…byte1 of each.
    esp_err_t Plane(uint8_t command, const uint8_t *data, const epaper::Box &b)
    {
        ESP_RETURN_ON_ERROR(bus_.Command(command), kTag, "plane 0x%02X", command);
        const size_t width = static_cast<size_t>(b.byte1 - b.byte0 + 1);
        const size_t rows = static_cast<size_t>(b.row1 - b.row0 + 1);
        if (width == epaper::kMonoStride) {
            const uint8_t *start = data + static_cast<size_t>(b.row0) * epaper::kMonoStride;
            return bus_.Stream(rows * width, [start](uint8_t *dst, size_t offset, size_t n) {
                std::memcpy(dst, start + offset, n);
            });
        }
        return bus_.Stream(rows * width, [&](uint8_t *dst, size_t offset, size_t n) {
            for (size_t k = 0; k < n; ++k) {
                const size_t o = offset + k;
                dst[k] = data[(b.row0 + o / width) * epaper::kMonoStride + b.byte0 + o % width];
            }
        });
    }

    esp_err_t Planes(const uint8_t *old_plane, const uint8_t *new_plane)
    {
        const epaper::Box all = {0, kYEnd, 0, kLastByte};
        ESP_RETURN_ON_ERROR(Window(all), kTag, "window");
        ESP_RETURN_ON_ERROR(Plane(0x26, old_plane, all), kTag, "old plane");
        ESP_RETURN_ON_ERROR(Plane(0x24, new_plane, all), kTag, "new plane");
        ram_masks_ = false;  // the RAM holds images now
        return ESP_OK;
    }

    esp_err_t Drive(uint8_t sequence)
    {
        ESP_RETURN_ON_ERROR(bus_.Write(0x22, {sequence}), kTag, "update sequence");
        const int64_t start = esp_timer_get_time();
        ESP_RETURN_ON_ERROR(bus_.Command(0x20), kTag, "master activation");
        ESP_RETURN_ON_ERROR(Wait(), kTag, "drive");
        timing_.drive_ms = epaper::ElapsedMs(start);
        // 0x80 and 0x40 turn the clock and the drive's supply on; 0x02 and 0x01 turn them off at the end.
        if ((sequence & 0x03) != 0) {
            powered_ = false;
        } else if ((sequence & 0xC0) == 0xC0) {
            powered_ = true;
        }
        return ESP_OK;
    }

    // Turns the clock and the drive's supply on once, so a scan is the display step alone (0x0C) — InkBoy's way:
    // re-enabling them on every drive (0xCC) cost about 80 ms a drive on a V1.0 board.
    esp_err_t PowerOn()
    {
        if (powered_) {
            return ESP_OK;
        }
        const int64_t start = esp_timer_get_time();
        ESP_RETURN_ON_ERROR(bus_.Write(0x22, {0xC0}), kTag, "power on");
        ESP_RETURN_ON_ERROR(bus_.Command(0x20), kTag, "power on");
        ESP_RETURN_ON_ERROR(Wait(), kTag, "power on");
        timing_.power_on_ms += epaper::ElapsedMs(start);
        powered_ = true;
        return ESP_OK;
    }

    esp_err_t Full(const uint8_t *frame)
    {
        int64_t t = esp_timer_get_time();
        ESP_RETURN_ON_ERROR(Prepare(kFullMode), kTag, "prepare full");
        timing_.latch_ms = epaper::ElapsedMs(t);
        t = esp_timer_get_time();
        ESP_RETURN_ON_ERROR(Planes(frame, frame), kTag, "planes");
        timing_.stream_ms = epaper::ElapsedMs(t);
        ESP_RETURN_ON_ERROR(Drive(0xF4), kTag, "full");  // keeps the analog supply and clock on afterwards
        std::memcpy(previous_, frame, epaper::kMonoBytes);
        partial_ready_ = true;
        return ESP_OK;
    }

    // The OTP's partial waveform; `old_plane` is what the glass is taken to show.
    esp_err_t Partial(const uint8_t *frame, const uint8_t *old_plane)
    {
        int64_t t = esp_timer_get_time();
        ESP_RETURN_ON_ERROR(Prepare(kPartialMode), kTag, "prepare partial");
        timing_.latch_ms = epaper::ElapsedMs(t);
        t = esp_timer_get_time();
        ESP_RETURN_ON_ERROR(Planes(old_plane, frame), kTag, "planes");
        timing_.stream_ms = epaper::ElapsedMs(t);
        ESP_RETURN_ON_ERROR(bus_.Write(0x21, {0x00, 0x00}), kTag, "update control");
        ESP_RETURN_ON_ERROR(Drive(PartialSequence()), kTag, "partial");
        lut_loaded_ = true;
        custom_loaded_ = false;  // the OTP's waveform replaced ours
        std::memcpy(previous_, frame, epaper::kMonoBytes);
        return ESP_OK;
    }

    esp_err_t DoScan(const uint8_t *black, const uint8_t *white, const epaper::Box &box)
    {
        int64_t t = esp_timer_get_time();
        ESP_RETURN_ON_ERROR(Prepare(kPartialMode), kTag, "prepare scan");
        if (!custom_loaded_) {
            ESP_RETURN_ON_ERROR(LoadScanWaveform(), kTag, "scan waveform");
        }
        timing_.latch_ms = epaper::ElapsedMs(t);

        // The upload covers this scan's masks and wherever the last scan's were, so those are cleared to hold; after
        // images, the whole RAM.
        epaper::Box region = box;
        if (!ram_masks_) {
            region = {0, kYEnd, 0, kLastByte};
        } else if (!last_box_.Empty()) {
            region = region.Empty() ? last_box_ : epaper::Box{std::min(region.row0, last_box_.row0),
                                                              std::max(region.row1, last_box_.row1),
                                                              std::min(region.byte0, last_box_.byte0),
                                                              std::max(region.byte1, last_box_.byte1)};
        }
        t = esp_timer_get_time();
        if (!region.Empty()) {
            ESP_RETURN_ON_ERROR(Window(region), kTag, "window");
            ESP_RETURN_ON_ERROR(Plane(0x26, black, region), kTag, "black mask");
            ESP_RETURN_ON_ERROR(Plane(0x24, white, region), kTag, "white mask");
        }
        ram_masks_ = true;
        last_box_ = box;
        timing_.stream_ms = epaper::ElapsedMs(t);

        ESP_RETURN_ON_ERROR(PowerOn(), kTag, "power on");
        ESP_RETURN_ON_ERROR(Drive(0x0C), kTag, "scan");  // our waveform as loaded, the supply already on
        lut_loaded_ = false;
        return ESP_OK;
    }

    // Two bits a pixel become two planes: the high bit, inverted, to 0x24; the low bit, inverted, to 0x26.
    esp_err_t Gray(const uint8_t *frame)
    {
        int64_t t = esp_timer_get_time();
        ESP_RETURN_ON_ERROR(Prepare(kGrayMode), kTag, "prepare grey");
        timing_.latch_ms = epaper::ElapsedMs(t);
        uint8_t *high = planes_;
        uint8_t *low = planes_ + epaper::kMonoBytes;
        for (size_t i = 0; i < epaper::kMonoBytes; ++i) {
            const uint8_t a = frame[i * 2], b = frame[i * 2 + 1];  // eight pixels, four in each byte
            uint8_t h = 0, l = 0;
            for (int p = 0; p < 8; ++p) {
                const uint8_t px = static_cast<uint8_t>(((p < 4 ? a : b) >> (6 - (p & 3) * 2)) & 0x03);
                h = static_cast<uint8_t>((h << 1) | ((px >> 1) ^ 1));
                l = static_cast<uint8_t>((l << 1) | ((px & 1) ^ 1));
            }
            high[i] = h;
            low[i] = l;
        }
        t = esp_timer_get_time();
        ESP_RETURN_ON_ERROR(Planes(low, high), kTag, "planes");  // 0x26 low, 0x24 high
        timing_.stream_ms = epaper::ElapsedMs(t);
        ESP_RETURN_ON_ERROR(bus_.Write(0x1A, {0x67, 0x00}), kTag, "grey temperature");  // Seeed's forced value
        ESP_RETURN_ON_ERROR(Drive(0xD7), kTag, "grey");
        partial_ready_ = false;  // the old plane no longer holds a one-bit frame
        return ESP_OK;
    }

    // The OTP's partial waveform and the panel's voltages for this temperature first, then our scan waveform over bytes
    // 0–104 only. LUT bytes: 0–49 the five LUTs' phases, two bits each (00 VSS, 01 VSH1, 10 VSL); 50–99 each group's
    // four phase lengths and its repeat; 100–104 the frame rate.
    esp_err_t LoadScanWaveform()
    {
        ESP_RETURN_ON_ERROR(bus_.Write(0x22, {0xB9}), kTag, "OTP load");  // clock, temperature, LUT mode 2, clock off
        powered_ = false;
        ESP_RETURN_ON_ERROR(bus_.Command(0x20), kTag, "OTP load");
        ESP_RETURN_ON_ERROR(Wait(), kTag, "OTP load");
        uint8_t lut[105] = {};
        lut[10] = 0x80;                 // LUT1, toward white: group 0 phase A at VSL
        lut[20] = 0x40;                 // LUT2, toward black: group 0 phase A at VSH1
        lut[50] = tuning_.scan_frames;  // TP[0A]; every other phase 0, so skipped; RP[0] 0, so once
        for (int i = 100; i < 105; ++i) {
            lut[i] = kScanRate;
        }
        ESP_RETURN_ON_ERROR(bus_.Command(0x32), kTag, "LUT");
        ESP_RETURN_ON_ERROR(bus_.Data(lut, sizeof(lut)), kTag, "LUT");
        custom_loaded_ = true;
        return ESP_OK;
    }

    // The partial's update sequence. Seeed's loads the temperature and the waveform every time (0xFC, or 0xFF after a
    // standby); a kept waveform drives with what is loaded (0xCC). On a V1.0 board the load measured about 3 ms, and a
    // forced temperature (0x1A) found no faster partial waveform in the OTP: 467 ms from room temperature to 70 °C,
    // 1505 ms at 90 °C.
    uint8_t PartialSequence()
    {
        if (tuning_.keep_waveform && lut_loaded_ && !custom_loaded_) {
            return 0xCC;
        }
        return after_standby_ ? 0xFF : 0xFC;
    }

    epaper::Bus &bus_;
    epaper::Tuning tuning_;
    bool powered_ = false;        // the clock and the drive's supply are on
    bool lut_loaded_ = false;     // a partial's OTP waveform is in the controller
    bool custom_loaded_ = false;  // our scan waveform is in the controller
    bool ram_masks_ = false;      // the RAM holds scan masks, bounded by last_box_; else images
    epaper::Box last_box_;
    float temperature_c_ = -100;  // the SHT40's, for the scans' window; unknown keeps them off
    State mode_ = kNone;
    bool standby_ = false;
    bool after_standby_ = false;
    bool partial_ready_ = false;
    uint8_t *previous_ = nullptr;  // the one-bit frame on the glass
    uint8_t *planes_ = nullptr;    // the grey frame's two planes; Settle's old plane
};

}  // namespace

std::unique_ptr<epaper::Panel> Create(epaper::Bus &bus)
{
    auto panel = std::make_unique<Ssd1677>(bus);
    if (panel->Allocate() != ESP_OK) {
        return nullptr;
    }
    return panel;
}

}  // namespace ssd1677
