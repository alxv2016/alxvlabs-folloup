// The display: LVGL keeps the whole 480 x 800 portrait screen in 8-bit grey and redraws only what changed (direct
// mode); the flush folds each changed area into the native 800 x 480 one-bit frame and, on the last area, hands it to
// the panel's driver — ssd1677 or ssd2677: a refresh by the controller's own waveform, or, for a small change where
// the SSD1677's tuning picks them, scans that a task of their own runs until every changed pixel is finished
// (STICKY_DESIGN_TOKENS §5).

#include <algorithm>
#include <atomic>
#include <cstring>

#include "board.h"
#include "board_config.h"
#include "design_tokens.h"
#include "lvgl_display.h"
#include "lvgl_display_priv.h"
#include "epaper.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "sdkconfig.h"

namespace lvgl_display {
namespace {

namespace bc = board_config;
namespace dt = design_tokens;
constexpr const char *kTag = "eink_display";

// The panel goes to standby once it has been idle this long; the driver powers it on at the next refresh.
constexpr uint32_t kStandbyAfterMs = 1000;
// The room's temperature is read again at most this often, while the glass is idle.
constexpr uint32_t kTemperatureEveryMs = 60 * 1000;
bool (*s_read_temperature)(float *celsius) = nullptr;
int64_t s_temperature_at_us = 0;

epaper::Panel *s_panel = nullptr;
lv_display_t *s_display = nullptr;
uint8_t *s_render = nullptr;  // LVGL's L8 screen, portrait, always whole
uint8_t *s_mono = nullptr;    // the native frame, one bit a pixel, 1 = white, kept between frames
uint8_t *s_gray = nullptr;    // the native frame, two bits a pixel, 0 = black … 3 = white
uint8_t *s_sent = nullptr;    // the one-bit frame on the glass, to skip a partial that changes nothing
bool s_sent_valid = false;
int64_t s_render_start_us = 0;
int64_t s_pack_us = 0;        // packing time across this frame's areas

// The partial refresh's tuning, as named presets for each controller; the first is Seeed's.
// SSD2677: the frame rate. On a V2.0 board the benchmark measured 75 Hz the same as the waveform's own rate (458 ms a
// frame) and 85 Hz faster (408 ms), and 85 Hz showed no ghosting on the glass — so 85 Hz is its default
// (STICKY_EPAPER_WAVEFORMS §4.1).
// SSD1677: the waveform kept between partials (about 3 ms a partial on a V1.0 board), or scans — InkBoy's method: each
// changed pixel gets a budget of short scans toward its colour, delivered scan after scan until it is spent, while new
// changes start at once (STICKY_EPAPER_WAVEFORMS §3.3, §3.5).
struct TuningPreset {
    const char *name;
    epaper::Tuning tuning;
};
constexpr TuningPreset kTunings2677[] = {
    {"Waveform rate", {}},
    {"75 Hz", {0x04, false, 0, 0}},
    {"85 Hz", {0x05, false, 0, 0}},
};
constexpr TuningPreset kTunings1677[] = {
    {"Seeed", {}},
    {"Keep waveform", {0, true, 0, 0}},
    {"Scan 2 x 12", {0, true, 2, 12}},
    {"Scan 3 x 10", {0, true, 3, 10}},
    {"Scan 4 x 10", {0, true, 4, 10}},
};
constexpr int kDefaultTuning2677 = 2;  // 85 Hz
constexpr int kDefaultTuning1677 = 2;  // scan 2 x 12: 85 ms a frame, 11.8 fps, clean on a V1.0 board's glass (the
                                       // person's choice; STICKY_EPAPER_WAVEFORMS §4.2)
const TuningPreset *s_tunings = kTunings1677;
int s_tuning_count = 1;
int s_tuning = 0;

std::atomic<bool> s_want_full{true};  // the first frame after boot is full (STICKY_DESIGN_TOKENS §5)
std::atomic<bool> s_want_gray{false};
// A new screen was loaded: on the SSD1677 the frame is drawn in two partials, through white, so the old page leaves
// no ghost (STICKY_EPAPER_WAVEFORMS §3.4).
std::atomic<bool> s_page_changed{false};
uint8_t *s_page_white = nullptr;  // an all-white one-bit frame, the first pass's target
Stats s_stats;
std::atomic<int64_t> s_last_refresh_us{0};  // when the last refresh or scan ended; read by the input task too
// The last full or greyscale refresh: when it ended and how long it took. The next one waits until as long again has
// passed — two never run back to back (STICKY_DESIGN_TOKENS §5).
int64_t s_flash_end_us = 0;
uint32_t s_flash_ms = 0;
bool s_standby = false;

// --- Scans ------------------------------------------------------------------------------------------------------
// A frame that changes at most this many pixels is drawn by scans; a bigger change — a new screen — by the
// controller's own partial waveform, which drives every changed pixel the whole way at once.
constexpr uint32_t kScanMaxChanged = 800 * 480 * 15 / 100;
// After this many frames drawn by scans, the next pause gets a full refresh: scans never stop for one mid-motion.
constexpr uint32_t kScanFramesBeforeFull = 150;
constexpr uint32_t kFullAfterPauseMs = 2000;
constexpr size_t kWords = epaper::kMonoBytes / 4;
constexpr size_t kRowWords = epaper::kMonoStride / 4;  // 25: a row is whole words

SemaphoreHandle_t s_engine = nullptr;   // the panel and the scan state: held by the flush, the scan task and standby
SemaphoreHandle_t s_scan_done = nullptr;
TaskHandle_t s_scan_task = nullptr;
uint32_t *s_target = nullptr;           // the frame the scans drive toward
uint32_t *s_count[4] = {};              // each pixel's scans left, four bit-planes
uint8_t *s_black = nullptr, *s_white = nullptr;  // this scan's masks
std::atomic<bool> s_scan_work{false};
// A frame waiting for the engine: the scan task steps aside after its scan until the frame has it. The tasks are not
// pinned, and a released mutex alone let the scan task, run on the other core, take it back first — frames stalled
// until every budget was spent (STICKY_EPAPER_WAVEFORMS §6).
std::atomic<bool> s_frame_waiting{false};
bool s_hold_end = false;                // one all-hold scan after the last budget is spent
bool s_scanned = false;                 // scans ran since the last image refresh: the driver's old frame is stale
uint32_t s_scan_frames = 0;             // frames drawn by scans since the last full refresh

uint8_t ScanBudget()
{
    return s_tunings[s_tuning].tuning.scan_budget;
}

// The pixels still pending, OR'd across the counter planes, into `out` (bytes).
void PendingMask(uint8_t *out)
{
    uint32_t *o = reinterpret_cast<uint32_t *>(out);
    for (size_t w = 0; w < kWords; ++w) {
        o[w] = s_count[0][w] | s_count[1][w] | s_count[2][w] | s_count[3][w];
    }
}

void ClearScans()
{
    for (uint32_t *plane : s_count) {
        std::memset(plane, 0, epaper::kMonoBytes);
    }
    s_hold_end = false;
    s_scanned = false;
    s_scan_work = false;
}

// Under s_engine: pixels a scan left part-way are drawn the whole way by the OTP's waveform, so no budget is cut
// short (STICKY_EPAPER_WAVEFORMS §2) — before a scan failure is given up, the tuning changes or the panel sleeps. A
// settle that fails too leaves the glass unknown: the next frame is full. With no scan pending there is nothing to
// settle, and s_scanned stays set: the driver's old frame is still stale until the next image refresh.
esp_err_t SettleScans()
{
    if (!s_scan_work) {
        return ESP_OK;
    }
    PendingMask(s_black);
    board::LockSharedBus();
    const esp_err_t err = s_panel->Settle(reinterpret_cast<const uint8_t *>(s_target), s_black);
    board::UnlockSharedBus();
    ClearScans();  // the settle was an image refresh: the driver's old frame is the glass again
    xSemaphoreGive(s_scan_done);  // a frame waiting on a scan that will not come now runs on, not after its timeout
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "settle failed: %s; the next frame is full", esp_err_to_name(err));
        s_want_full = true;
        s_sent_valid = false;
    }
    return err;
}

// Under s_engine: each changed pixel's budget becomes the full budget toward its new colour (InkBoy's
// drive_counters_prepare); the target becomes the frame.
void StartScans(const uint8_t *frame)
{
    const uint8_t budget = ScanBudget();
    const uint32_t *f = reinterpret_cast<const uint32_t *>(frame);
    for (size_t w = 0; w < kWords; ++w) {
        const uint32_t changed = f[w] ^ s_target[w];
        if (changed == 0) {
            continue;
        }
        for (int p = 0; p < 4; ++p) {
            s_count[p][w] = (s_count[p][w] & ~changed) | (((budget >> p) & 1) ? changed : 0);
        }
        s_target[w] = f[w];
    }
    s_hold_end = true;
    s_scanned = true;
}

// One scan: masks from the budgets, the panel's scan, then each delivered pixel's budget down by one (a bit-sliced
// decrement, InkBoy's drive_counters_complete). Returns the pixels driven; under s_engine.
esp_err_t ScanOnce(uint32_t *driven)
{
    epaper::Box box;
    box.row0 = UINT16_MAX;
    box.row1 = 0;
    box.byte0 = UINT16_MAX;
    box.byte1 = 0;
    uint32_t *black = reinterpret_cast<uint32_t *>(s_black);
    uint32_t *white = reinterpret_cast<uint32_t *>(s_white);
    uint32_t count = 0;
    for (size_t w = 0; w < kWords; ++w) {
        const uint32_t active = s_count[0][w] | s_count[1][w] | s_count[2][w] | s_count[3][w];
        black[w] = active & ~s_target[w];
        white[w] = active & s_target[w];
        if (active == 0) {
            continue;
        }
        count += static_cast<uint32_t>(__builtin_popcount(active));
        const uint16_t row = static_cast<uint16_t>(w / kRowWords);
        const uint16_t col = static_cast<uint16_t>((w % kRowWords) * 4);
        box.row0 = std::min(box.row0, row);
        box.row1 = std::max(box.row1, row);
        box.byte0 = std::min<uint16_t>(box.byte0, static_cast<uint16_t>(col + __builtin_ctz(active) / 8));
        box.byte1 = std::max<uint16_t>(box.byte1, static_cast<uint16_t>(col + (31 - __builtin_clz(active)) / 8));
    }
    if (count == 0) {
        box = epaper::Box{};  // empty: the scan clears the last masks and holds
    }
    board::LockSharedBus();
    const esp_err_t err = s_panel->Scan(s_black, s_white, box);
    board::UnlockSharedBus();
    if (err == ESP_OK) {
        for (size_t w = 0; w < kWords; ++w) {
            uint32_t borrow = black[w] | white[w];
            for (int p = 0; p < 4 && borrow != 0; ++p) {
                const uint32_t old = s_count[p][w];
                s_count[p][w] = old ^ borrow;
                borrow &= ~old;
            }
        }
    }
    *driven = count;
    return err;
}

void TakeTiming()
{
    const epaper::Timing &timing = s_panel->LastTiming();
    s_stats.temperature_c = timing.temperature_c;
    s_stats.latch_ms = timing.latch_ms;
    s_stats.stream_ms = timing.stream_ms;
    s_stats.power_on_ms = timing.power_on_ms;
    s_stats.drive_ms = timing.drive_ms;
}

// Scans while any pixel has budget left, then one all-hold scan; each finished scan wakes a waiting flush.
void ScanTask(void *)
{
    for (;;) {
        if (!s_scan_work) {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            continue;
        }
        xSemaphoreTake(s_engine, portMAX_DELAY);
        if (!s_scan_work) {
            xSemaphoreGive(s_engine);
            continue;
        }
        const int64_t start = esp_timer_get_time();
        uint32_t driven = 0;
        const esp_err_t err = ScanOnce(&driven);
        if (err != ESP_OK) {
            // Scans stop; whatever stands part-way is drawn the whole way by the OTP's waveform.
            ESP_LOGE(kTag, "scan failed: %s; settling", esp_err_to_name(err));
            SettleScans();
            ++s_stats.errors;
        } else {
            if (driven == 0) {
                s_hold_end = false;
            }
            s_scan_work = driven != 0 || s_hold_end;
            ++s_stats.scans;
        }
        TakeTiming();
        s_last_refresh_us = esp_timer_get_time();
        s_standby = false;
        s_stats.scan_ms = static_cast<uint32_t>((s_last_refresh_us - start) / 1000);
        s_stats.scan_pixels = driven;
        // Done before the engine is free: a frame that takes it next drains this give, so its own wait is for its
        // own first scan.
        xSemaphoreGive(s_scan_done);
        xSemaphoreGive(s_engine);
        while (s_frame_waiting) {
            vTaskDelay(1);
        }
    }
}

#if CONFIG_FOLLOUP_DISPLAY_FLIP_180
constexpr bool kTurned = true;
#else
constexpr bool kTurned = false;
#endif

// The portrait screen's (x, y) on the native frame: the screen's top edge is the frame's column 799, its left edge
// row 0; the 180° option turns the whole frame.
inline void ToNative(int x, int y, int *nx, int *ny)
{
    if (kTurned) {
        x = bc::kScreenWidth - 1 - x;
        y = bc::kScreenHeight - 1 - y;
    }
    *nx = bc::kPanelWidth - 1 - y;
    *ny = x;
}

// Folds one changed area into the one-bit frame. A level under 2 is black, 2 or more white: L8 under 128
// (STICKY_DESIGN_TOKENS §2.1); the two fill codes, kDitherDark and kDitherLight, draw as their dithers, locked to the
// panel's pixels (STICKY_DESIGN_TOKENS §4).
void PackMono(const uint8_t *screen, const lv_area_t &a)
{
    for (int y = a.y1; y <= a.y2; ++y) {
        const uint8_t *row = screen + static_cast<size_t>(y) * bc::kScreenWidth;
        int nx, ny;
        ToNative(a.x1, y, &nx, &ny);
        const size_t column = static_cast<size_t>(nx >> 3);
        const uint8_t bit = static_cast<uint8_t>(0x80U >> (nx & 7));
        const int step = kTurned ? -1 : 1;  // the frame's row follows x, backwards when turned
        uint8_t *cell = s_mono + static_cast<size_t>(ny) * epaper::kMonoStride + column;
        // The glass's own pixel, for the dither: x along the frame's row is ny, the column's bit is nx.
        const int column_parity = nx & 1;
        int frame_row = ny;
        for (int x = a.x1; x <= a.x2; ++x, cell += step * static_cast<int>(epaper::kMonoStride), frame_row += step) {
            const uint8_t v = row[x];
            bool white;
            if (v == kDitherDark) {
                white = column_parity == 1 && (frame_row & 1) == 1;  // one in four
            } else if (v == kDitherLight) {
                white = !(column_parity == 0 && (frame_row & 1) == 0);  // three in four
            } else {
                white = v >= 128;
            }
            *cell = white ? static_cast<uint8_t>(*cell | bit) : static_cast<uint8_t>(*cell & ~bit);
        }
    }
}

void PackGray(const uint8_t *screen)
{
    for (int y = 0; y < bc::kScreenHeight; ++y) {
        const uint8_t *row = screen + static_cast<size_t>(y) * bc::kScreenWidth;
        for (int x = 0; x < bc::kScreenWidth; ++x) {
            const uint8_t level = row[x] >> 6;  // 0x00 → 0, 0x55 → 1, 0xAA → 2, 0xFF → 3
            int nx, ny;
            ToNative(x, y, &nx, &ny);
            uint8_t &byte = s_gray[static_cast<size_t>(ny) * epaper::kGrayStride + (nx >> 2)];
            const int shift = 6 - (nx & 3) * 2;
            byte = static_cast<uint8_t>((byte & ~(0x03U << shift)) | (level << shift));
        }
    }
}

epaper::Mode ToMode(Refresh r)
{
    return r == Refresh::kGray ? epaper::Mode::kGray : r == Refresh::kFull ? epaper::Mode::kFull : epaper::Mode::kPartial;
}

uint32_t ChangedPixels()
{
    const uint32_t *a = reinterpret_cast<const uint32_t *>(s_mono);
    uint32_t changed = 0;
    for (size_t w = 0; w < kWords; ++w) {
        changed += static_cast<uint32_t>(__builtin_popcount(a[w] ^ s_target[w]));
    }
    return changed;
}

void LogRefresh(const char *kind)
{
    ESP_LOGI(kTag, "%s refresh: %lu ms — render %lu, pack %lu, latch %lu, stream %lu, power on %lu, drive %lu (%s)",
             kind, static_cast<unsigned long>(s_stats.last_ms), static_cast<unsigned long>(s_stats.render_ms),
             static_cast<unsigned long>(s_stats.pack_ms), static_cast<unsigned long>(s_stats.latch_ms),
             static_cast<unsigned long>(s_stats.stream_ms), static_cast<unsigned long>(s_stats.power_on_ms),
             static_cast<unsigned long>(s_stats.drive_ms), s_tunings[s_tuning].name);
}

// A small change by scans: the budgets start at once, and the flush waits for the next scan to finish, so the glass
// paces LVGL — the pixels then keep finishing scan after scan while the next frames are drawn.
void SendByScans()
{
    const int64_t start = esp_timer_get_time();
    while (xSemaphoreTake(s_scan_done, 0) == pdTRUE) {
    }
    StartScans(s_mono);
    s_scan_work = true;
    xSemaphoreGive(s_engine);
    xTaskNotifyGive(s_scan_task);
    xSemaphoreTake(s_scan_done, pdMS_TO_TICKS(3000));
    // s_sent_valid was set to reach here, and only a failed settle clears it: the glass is then unknown, and stays so.
    std::memcpy(s_sent, s_mono, epaper::kMonoBytes);
    ++s_stats.partial_count;
    ++s_scan_frames;
    s_stats.last = Refresh::kPartial;
    s_stats.last_ms = static_cast<uint32_t>((esp_timer_get_time() - start) / 1000);
    ESP_LOGD(kTag, "scan frame: %lu ms, first scan %lu ms, %lu pixels in it",
             static_cast<unsigned long>(s_stats.last_ms), static_cast<unsigned long>(s_stats.scan_ms),
             static_cast<unsigned long>(s_stats.scan_pixels));
}

// Waits out the rest of the last full refresh's own time after its end, so the glass never flashes twice back to back;
// the frame is held, never dropped (DESIGN → Do's and Don'ts; STICKY_DESIGN_TOKENS §5).
void WaitAfterFlash()
{
    if (s_flash_end_us == 0) {
        return;
    }
    const int64_t since_ms = (esp_timer_get_time() - s_flash_end_us) / 1000;
    if (since_ms >= static_cast<int64_t>(s_flash_ms)) {
        return;
    }
    const uint32_t wait_ms = s_flash_ms - static_cast<uint32_t>(since_ms);
    ESP_LOGI(kTag, "full refresh held %lu ms: never two back to back", static_cast<unsigned long>(wait_ms));
    vTaskDelay(pdMS_TO_TICKS(wait_ms));
}

void Send(lv_display_t *display, const uint8_t *screen)
{
    // A page change on the SSD1677 is two partials, so it needs room for both under the limit, or is drawn full.
    const bool page = s_page_changed.exchange(false) && !s_stats.ssd2677;
    const uint32_t needed = page ? 2 : 1;
    Refresh mode = Refresh::kPartial;
    if (s_want_gray.exchange(false)) {
        mode = Refresh::kGray;
    } else if (s_want_full.exchange(false) ||
               s_stats.partials + needed > static_cast<uint32_t>(dt::kRefreshPartialLimit)) {
        mode = Refresh::kFull;
    }
    const int64_t t_send = esp_timer_get_time();
    if (mode == Refresh::kGray) {
        PackGray(screen);
    }
    s_stats.render_ms = s_render_start_us != 0 ? static_cast<uint32_t>((t_send - s_render_start_us) / 1000) : 0;
    s_stats.pack_ms = static_cast<uint32_t>((s_pack_us + esp_timer_get_time() - t_send) / 1000);
    s_stats.render_ms = s_stats.render_ms > s_stats.pack_ms ? s_stats.render_ms - s_stats.pack_ms : 0;

    const bool page_pass = page && mode == Refresh::kPartial;
    if (mode != Refresh::kPartial) {
        WaitAfterFlash();
    }

    // A partial frame identical to the glass is not sent: focus moving on a tap redraws with nothing visible changed.
    if (mode == Refresh::kPartial && s_sent_valid && std::memcmp(s_mono, s_sent, epaper::kMonoBytes) == 0) {
        ++s_stats.skipped;
        return;
    }

    s_frame_waiting = true;
    xSemaphoreTake(s_engine, portMAX_DELAY);
    s_frame_waiting = false;
    if (!page_pass && mode == Refresh::kPartial && s_sent_valid && ScanBudget() != 0 && s_panel->ScanReady() &&
        ChangedPixels() <= kScanMaxChanged) {
        SendByScans();  // gives s_engine back
        return;
    }

    // An image refresh by the controller's own waveform. After scans, the driver's idea of the glass is stale, so the
    // partial says which pixels to drive: the changed ones and any a scan left part-way.
    const int64_t start = esp_timer_get_time();
    board::LockSharedBus();
    esp_err_t err = ESP_OK;
    bool scanned = s_scanned;
    if (page_pass) {
        // The first pass: every pixel not white, or left part-way by a scan, driven to white.
        if (scanned) {
            PendingMask(s_black);
            uint32_t *touched = reinterpret_cast<uint32_t *>(s_black);
            for (size_t w = 0; w < kWords; ++w) {
                touched[w] |= ~s_target[w];
            }
            err = s_panel->Settle(s_page_white, s_black);
        } else {
            err = s_panel->Refresh(epaper::Mode::kPartial, s_page_white);
        }
        scanned = false;  // the driver's frame is the white one now
    }
    if (err != ESP_OK) {
        // the failed first pass is drawn again in full, below
    } else if (mode == Refresh::kPartial && scanned) {
        PendingMask(s_black);
        uint32_t *touched = reinterpret_cast<uint32_t *>(s_black);
        const uint32_t *m = reinterpret_cast<const uint32_t *>(s_mono);
        for (size_t w = 0; w < kWords; ++w) {
            touched[w] |= m[w] ^ s_target[w];
        }
        err = s_panel->Settle(s_mono, s_black);
    } else {
        err = s_panel->Refresh(ToMode(mode), mode == Refresh::kGray ? s_gray : s_mono);
    }
    int64_t flash_start = start;
    if (err != ESP_OK && mode == Refresh::kPartial) {
        mode = Refresh::kFull;  // a failed partial is drawn again in full, from a reset
        flash_start = esp_timer_get_time();
        err = s_panel->Refresh(epaper::Mode::kFull, s_mono);
    }
    board::UnlockSharedBus();
    ClearScans();
    std::memcpy(s_target, s_mono, epaper::kMonoBytes);
    const int64_t end = esp_timer_get_time();
    if (mode != Refresh::kPartial && err == ESP_OK) {  // a failed refresh flashed nothing to space out
        s_flash_end_us = end;
        s_flash_ms = static_cast<uint32_t>((end - flash_start) / 1000);
    }

    TakeTiming();
    s_last_refresh_us = end;
    s_standby = false;
    s_stats.last = mode;
    s_stats.last_ms = static_cast<uint32_t>((end - start) / 1000);
    if (mode == Refresh::kGray || err != ESP_OK) {
        s_sent_valid = false;  // the glass is not the one-bit frame
    } else {
        std::memcpy(s_sent, s_mono, epaper::kMonoBytes);
        s_sent_valid = true;
    }
    if (err != ESP_OK) {
        ++s_stats.errors;
        s_want_full = true;
        ESP_LOGE(kTag, "refresh failed: %s", esp_err_to_name(err));
    } else if (mode == Refresh::kPartial) {
        s_stats.partials += needed;
        ++s_stats.partial_count;
    } else {
        s_stats.partials = 0;
        s_scan_frames = 0;
        if (mode == Refresh::kGray) {
            ++s_stats.gray_count;
            s_want_full = true;  // the next one-bit frame is drawn in full
        } else {
            ++s_stats.full_count;
        }
    }
    xSemaphoreGive(s_engine);
    LogRefresh(mode == Refresh::kGray ? "gray4" : mode == Refresh::kFull ? "full" : page_pass ? "page" : "partial");
    (void)display;
}

// Direct mode: one call per changed area, with the whole screen behind px_map; the frame goes out on the last.
void Flush(lv_display_t *display, const lv_area_t *area, uint8_t *px_map)
{
    const int64_t t = esp_timer_get_time();
    PackMono(px_map, *area);
    s_pack_us += esp_timer_get_time() - t;
    if (lv_display_flush_is_last(display)) {
        Send(display, px_map);
        s_pack_us = 0;
    }
    lv_display_flush_ready(display);
}

void OnScreenLoaded(lv_event_t *)
{
    s_page_changed = true;
}

void OnRenderStart(lv_event_t *)
{
    s_render_start_us = esp_timer_get_time();
}

// The SHT40's reading, given to the panel again while the glass is idle: the SSD2677's waveform slot and the
// SSD1677's scan window follow the room, not the boot (STICKY_EPAPER_WAVEFORMS §2).
void UpdateTemperature()
{
    const int64_t now = esp_timer_get_time();
    if (s_read_temperature == nullptr || s_scan_work ||
        (s_temperature_at_us != 0 && now - s_temperature_at_us < kTemperatureEveryMs * 1000LL)) {
        return;
    }
    s_temperature_at_us = now;
    float celsius = 0;
    if (!s_read_temperature(&celsius)) {
        return;
    }
    xSemaphoreTake(s_engine, portMAX_DELAY);
    s_panel->SetTemperature(celsius);
    xSemaphoreGive(s_engine);
}

void StandbyWhenIdle(lv_timer_t *)
{
    if (s_last_refresh_us != 0 && !s_scan_work) {
        UpdateTemperature();
    }
    if (s_standby || s_last_refresh_us == 0 || s_scan_work) {
        return;
    }
    const uint32_t idle_ms = static_cast<uint32_t>((esp_timer_get_time() - s_last_refresh_us) / 1000);
    // After many frames by scans, a pause gets a full refresh — the 20-partials rule, kept out of the motion.
    if (s_scan_frames >= kScanFramesBeforeFull && idle_ms >= kFullAfterPauseMs) {
        s_scan_frames = 0;
        s_want_full = true;
        lv_obj_invalidate(lv_screen_active());
        return;
    }
    if (idle_ms < kStandbyAfterMs) {
        return;
    }
    xSemaphoreTake(s_engine, portMAX_DELAY);
    board::LockSharedBus();
    const esp_err_t err = s_panel->Standby();
    board::UnlockSharedBus();
    // A failed standby is not retried: the driver resets the controller at the next refresh, which is drawn full
    // (STICKY_EPAPER_WAVEFORMS §6).
    s_standby = true;
    if (err != ESP_OK) {
        s_want_full = true;
        s_sent_valid = false;
    }
    xSemaphoreGive(s_engine);
    if (err != ESP_OK) {
        ESP_LOGW(kTag, "standby: %s; the next refresh resets the panel", esp_err_to_name(err));
    }
}

uint8_t *Allocate(size_t bytes, bool prefer_internal)
{
    void *p = prefer_internal ? heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT) : nullptr;
    return static_cast<uint8_t *>(p != nullptr ? p : heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
}

}  // namespace

esp_err_t StartDisplay(const board::Display &display)
{
    s_panel = display.panel;
    s_stats.ssd2677 = display.ssd2677;

    constexpr size_t kRenderBytes = static_cast<size_t>(bc::kScreenWidth) * bc::kScreenHeight;
    s_render = Allocate(kRenderBytes, false);
    s_mono = Allocate(epaper::kMonoBytes, true);  // written bit by bit on every frame: internal RAM when it fits
    s_gray = Allocate(epaper::kGrayBytes, false);
    s_sent = Allocate(epaper::kMonoBytes, false);
    s_target = reinterpret_cast<uint32_t *>(Allocate(epaper::kMonoBytes, false));
    for (uint32_t *&plane : s_count) {
        plane = reinterpret_cast<uint32_t *>(Allocate(epaper::kMonoBytes, false));
    }
    s_black = Allocate(epaper::kMonoBytes, true);
    s_white = Allocate(epaper::kMonoBytes, true);
    s_page_white = Allocate(epaper::kMonoBytes, false);
    ESP_RETURN_ON_FALSE(s_render && s_mono && s_gray && s_sent && s_target && s_count[0] && s_count[1] && s_count[2] &&
                            s_count[3] && s_black && s_white && s_page_white,
                        ESP_ERR_NO_MEM, kTag, "frame buffers");
    std::memset(s_mono, 0xFF, epaper::kMonoBytes);
    std::memset(s_target, 0xFF, epaper::kMonoBytes);
    std::memset(s_page_white, 0xFF, epaper::kMonoBytes);
    ClearScans();
    s_engine = xSemaphoreCreateMutex();
    s_scan_done = xSemaphoreCreateBinary();
    ESP_RETURN_ON_FALSE(s_engine && s_scan_done, ESP_ERR_NO_MEM, kTag, "scan locks");
    // Below LVGL's task (esp_lvgl_port's 4); a frame waiting for the engine gets it between two scans (s_frame_waiting),
    // so new changes start while earlier ones are still finishing.
    ESP_RETURN_ON_FALSE(xTaskCreate(ScanTask, "epaper_scan", 4096, nullptr, 3, &s_scan_task) == pdPASS, ESP_ERR_NO_MEM,
                        kTag, "scan task");

    s_display = lv_display_create(bc::kScreenWidth, bc::kScreenHeight);
    ESP_RETURN_ON_FALSE(s_display != nullptr, ESP_ERR_NO_MEM, kTag, "LVGL display");
    lv_display_set_color_format(s_display, LV_COLOR_FORMAT_L8);
    lv_display_set_buffers(s_display, s_render, nullptr, kRenderBytes, LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(s_display, Flush);
    lv_display_add_event_cb(s_display, OnRenderStart, LV_EVENT_RENDER_START, nullptr);
    lv_display_add_event_cb(s_display, OnScreenLoaded, LV_EVENT_SCREEN_LOADED, nullptr);
    lv_timer_create(StandbyWhenIdle, 250, nullptr);
    if (s_stats.ssd2677) {
        s_tunings = kTunings2677;
        s_tuning_count = static_cast<int>(sizeof(kTunings2677) / sizeof(kTunings2677[0]));
    } else {
        s_tunings = kTunings1677;
        s_tuning_count = static_cast<int>(sizeof(kTunings1677) / sizeof(kTunings1677[0]));
    }
    SetTuning(DefaultTuning());
    ESP_LOGI(kTag, "LVGL %d.%d.%d on the %s, %dx%d portrait%s", LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR,
             LVGL_VERSION_PATCH, s_panel->Name(), bc::kScreenWidth, bc::kScreenHeight, kTurned ? ", turned 180°" : "");
    return ESP_OK;
}

void FirstFrame()
{
    s_want_full = true;
}

int TuningCount()
{
    return s_tuning_count;
}

const char *TuningName(int preset)
{
    return preset >= 0 && preset < TuningCount() ? s_tunings[preset].name : "";
}

int Tuning()
{
    return s_tuning;
}

int DefaultTuning()
{
    return s_stats.ssd2677 ? kDefaultTuning2677 : kDefaultTuning1677;
}

void SetTemperatureSource(bool (*read)(float *celsius))
{
    s_read_temperature = read;
}

int64_t LastRefreshEndUs()
{
    return s_last_refresh_us;
}

esp_err_t SetTuning(int preset)
{
    ESP_RETURN_ON_FALSE(preset >= 0 && preset < TuningCount(), ESP_ERR_INVALID_ARG, kTag, "preset %d", preset);
    if (s_engine != nullptr) {
        xSemaphoreTake(s_engine, portMAX_DELAY);
        if (SettleScans() != ESP_OK) {  // the new preset may not scan, and a budget left part-way is never cut
            ESP_LOGW(kTag, "settle before the new tuning failed: the next frame is full");
        }
    }
    const esp_err_t err = s_panel->SetTuning(s_tunings[preset].tuning);
    if (s_engine != nullptr) {
        xSemaphoreGive(s_engine);
    }
    ESP_RETURN_ON_ERROR(err, kTag, "tuning");
    s_tuning = preset;
    ESP_LOGI(kTag, "partial tuning: %s", s_tunings[preset].name);
    return ESP_OK;
}

void RequestFull()
{
    s_want_full = true;
}

void RequestGray()
{
    s_want_gray = true;
}

void Redraw()
{
    lv_obj_invalidate(lv_screen_active());
    lv_refr_now(s_display);
}

bool Turned180()
{
    return kTurned;
}

esp_err_t SleepPanel()
{
    xSemaphoreTake(s_engine, portMAX_DELAY);
    SettleScans();
    board::LockSharedBus();
    const esp_err_t err = s_panel->Sleep();
    board::UnlockSharedBus();
    xSemaphoreGive(s_engine);
    s_standby = true;  // the idle timer leaves a sleeping panel alone
    s_want_full = true;
    return err;
}

Stats GetStats()
{
    return s_stats;
}

const uint8_t *GlassFrame()
{
    return s_sent_valid ? s_sent : nullptr;
}

bool Lock(uint32_t timeout_ms)
{
    return lvgl_port_lock(timeout_ms);
}

void Unlock()
{
    lvgl_port_unlock();
}

}  // namespace lvgl_display
