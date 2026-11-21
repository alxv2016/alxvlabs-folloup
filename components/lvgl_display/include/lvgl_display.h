// LVGL on the e-paper glass — its display and input drivers: a 480 x 800 portrait display kept in 8-bit grey, folded
// onto the native 800 x 480 frame and refreshed by the controller's driver (ssd1677 or ssd2677); the GT911 and the
// three buttons as inputs. Every frame is drawn as a partial refresh will show it (STICKY_DESIGN_TOKENS §1, §5). The
// look — fonts, styles, components — is ui's.
//
// Call LVGL only while holding Lock(), from any task but the LVGL task's own callbacks, which already hold it.

#pragma once

#include <cstdint>

#include "board.h"
#include "esp_err.h"
#include "glass_rules.h"  // kDitherDark, kDitherLight, FillCode
#include "lvgl.h"

namespace lvgl_display {

// Starts LVGL's task and tick, the display on the panel, the touch panel and the buttons, then builds the first screen
// with `first_screen` under the same lock — ui's start belongs there — and draws it as one full refresh. `touch_ok` is false when the GT911 did not
// answer: the buttons still work.
esp_err_t Start(const board::Display &display, bool *touch_ok, void (*first_screen)());

bool Lock(uint32_t timeout_ms = 0);  // 0: wait as long as it takes
void Unlock();

// --- The refresh -------------------------------------------------------------------------------------------------

enum class Refresh : uint8_t { kNone, kPartial, kFull, kGray };

// The next frame is drawn as a full refresh, then partials resume (STICKY_DESIGN_TOKENS §5).
void RequestFull();
// The next frame is drawn in four greys as a full refresh; the frame after it is a full one-bit refresh, since the
// SSD1677's greyscale write leaves the driver's one-bit copy stale.
void RequestGray();
// Redraws the whole screen now, under the lock the caller holds.
void Redraw();
// The partial refresh's tuning, as named presets for the controller fitted — the SSD2677's frame rates, the SSD1677's
// kept waveform and its scans; preset 0 is Seeed's, and the default is DefaultTuning().
int TuningCount();
const char *TuningName(int preset);
int Tuning();
int DefaultTuning();
esp_err_t SetTuning(int preset);
// Where the room's temperature comes from: read again about once a minute while the glass is idle, and given to the
// panel — the SSD2677's waveform slot, the SSD1677's scan window. Returns false when there is no reading.
void SetTemperatureSource(bool (*read)(float *celsius));

// Puts the controller into deep sleep: the frame on the glass stays. The next refresh wakes it with a reset.
esp_err_t SleepPanel();

struct Stats {
    bool ssd2677 = false;
    Refresh last = Refresh::kNone;
    uint32_t last_ms = 0;      // the last refresh, from the call to the panel's return
    // Where the last refresh's time went, in ms: LVGL's render, our rotation and packing, then the driver's waveform
    // latch, its stream to the controller, powering the drive on, and the drive itself.
    uint32_t render_ms = 0, pack_ms = 0, latch_ms = 0, stream_ms = 0, power_on_ms = 0, drive_ms = 0;
    uint32_t skipped = 0;      // partial frames identical to the glass, not sent
    uint32_t scans = 0;        // SSD1677 scans run
    uint32_t scan_ms = 0;      // the last scan, from its masks to the panel's return
    uint32_t scan_pixels = 0;  // the pixels it drove
    uint32_t partials = 0;     // partials since the last full refresh; a V1.0 page change counts two
    uint32_t full_count = 0;
    uint32_t partial_count = 0;
    uint32_t gray_count = 0;
    uint32_t errors = 0;
    int temperature_c = -1;    // the waveform temperature the driver used; -1 when unread
};
Stats GetStats();

// The one-bit frame on the glass now, native 800 x 480, 1 = white; nullptr when the glass shows something else — a
// grey frame, or nothing yet. Read it under Lock().
const uint8_t *GlassFrame();

// --- Input -------------------------------------------------------------------------------------------------------

// The group UP, DOWN and OK move through; a screen adds its controls in focus order after Clear.
lv_group_t *Group();
void ClearGroup();

// Holding OK for a second goes back: the handler the screen set runs under the lock. nullptr does nothing.
void SetBackHandler(void (*back)());

// Runs within kFeedbackTickMs of every accepted press — touch or button — before its frame is queued, on the input
// task, so it must be safe to call from any task.
void SetPressFeedback(void (*tick)());

// Runs on every button press — LV_KEY_PREV for UP, LV_KEY_NEXT for DOWN, LV_KEY_ENTER for OK — under the lock, before
// LVGL moves focus. nullptr stops it.
void SetKeyObserver(void (*seen)(uint32_t key));

struct TouchPoint {
    bool pressed = false;
    int16_t raw_x = 0, raw_y = 0;  // as the GT911 reports, native landscape
    int16_t x = 0, y = 0;          // on the portrait screen
};
TouchPoint LastTouch();
bool TouchReady();
bool Turned180();  // the build option that turns the frame and touch together

}  // namespace lvgl_display
