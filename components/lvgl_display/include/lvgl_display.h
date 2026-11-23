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
// Which partial refreshes count toward `kRefreshPartialLimit` (glass_rules.h `PartialCounting`): every one, or — while the
// keyboard is open — only those a typing press or a page switch caused, which the keyboard says with NoteTyping; the rest
// count toward a ceiling of their own, `kUncountedPartialLimit` (STICKY_DESIGN_TOKENS §5).
void SetPartialCounting(PartialCounting counting);
// The press now handled changed the typed text, or switched the keyboard's page: the next frame drawn counts toward the limit
// even when only typing counts.
void NoteTyping();
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

// The group UP, DOWN and OK move through — the one bound now; a screen adds its controls in focus order after Clear.
lv_group_t *Group();
void ClearGroup();
// Binds `group` to the buttons and makes it LVGL's default group, so a control LVGL adds to the default group as it is made
// joins it. The router calls it for each screen it shows (APP_ARCHITECTURE §7); until it first does, the group made at Start
// is bound. nullptr binds that one again.
void SetGroup(lv_group_t *group);

// Holding OK for a second runs `back` under the lock: the router's escape chain, which router::Init sets and nothing else
// does (APP_ARCHITECTURE §7). nullptr does nothing.
void SetBackHandler(void (*back)());

// Runs within kFeedbackTickMs of every accepted press — touch or button — before its frame is queued, on the input
// task, so it must be safe to call from any task.
void SetPressFeedback(void (*tick)());

// A sound a control asks for beyond the press's tick, by its word in DESIGN.md's sound tokens: a widget asks PlayCue, and the
// shell sets the player that sounds it (devices), so no widget reaches the buzzer itself (STICKY_DESIGN_TOKENS §5).
enum class Cue : uint8_t {
    kAtEnd,  // a step that can go no further: a page turned past the last or first, a slider stepped past its end
};
void SetCuePlayer(void (*play)(Cue cue));
// From LVGL's task, under the lock — a widget's event; nothing plays until the shell sets the player.
void PlayCue(Cue cue);

// Runs on every button press — LV_KEY_PREV for UP, LV_KEY_NEXT for DOWN, LV_KEY_ENTER for OK, and each step of a held UP or
// DOWN — under the lock, before LVGL moves focus. nullptr stops it.
void SetKeyObserver(void (*seen)(uint32_t key));

// Run on every read of the buttons, and of the touch panel, after the read has filled `data` and before LVGL acts on it, on
// LVGL's task: for a caller that must see each read as LVGL will — the router's focus rules. nullptr stops it.
void SetKeyReadHook(void (*hook)(lv_indev_t *indev, lv_indev_data_t *data));
void SetTouchReadHook(void (*hook)(lv_indev_t *indev, lv_indev_data_t *data));

enum class Button : uint8_t { kUp, kDown, kOk };
// Presses a button as a finger would, through the same queue, tick and OK gates as the pins: down, then up after a short
// press, or, with `hold`, after OK's back hold. For the bench console's key walks; blocks for the press's length, so call
// it from a task that may wait, never under the lock.
void PressButton(Button button, bool hold);

struct TouchPoint {
    bool pressed = false;
    int16_t raw_x = 0, raw_y = 0;  // as the GT911 reports, native landscape
    int16_t x = 0, y = 0;          // on the portrait screen
};
TouchPoint LastTouch();
bool TouchReady();
bool Turned180();  // the build option that turns the frame and touch together

}  // namespace lvgl_display
