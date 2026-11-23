// The inputs: the GT911 as a pointer, and UP, DOWN and OK as a keypad that moves the focus ring
// (DESIGN → Focus and input). A task of their own watches them, apart from LVGL's: a refresh holds LVGL's task for up
// to 1.6 s, and a press made inside it would wait, or be missed. The watcher ticks at the press and queues each press
// and release, as PressButton does for the bench console; LVGL replays the queue when it next reads
// (STICKY_DESIGN_TOKENS §5). A held UP or DOWN steps again by key_rules.h's KeyRepeat, not by LVGL's own long-press
// repeat, which is off.

#include <algorithm>
#include <atomic>

#include "board.h"
#include "board_config.h"
#include "driver/gpio.h"
#include "key_rules.h"
#include "lvgl_display.h"
#include "lvgl_display_priv.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "gt911.h"
#include "sdkconfig.h"

namespace lvgl_display {
namespace {

namespace bc = board_config;
constexpr const char *kTag = "eink_input";

// How often the watcher reads the buttons and the touch panel; LVGL's own read was as often.
constexpr uint32_t kWatchMs = 10;

struct KeyEvent {
    uint32_t key;       // LV_KEY_PREV (UP), LV_KEY_NEXT (DOWN), LV_KEY_ENTER (OK)
    bool pressed;
    int64_t since_us;   // when this press began
    int64_t at_us;      // when this event happened
};
QueueHandle_t s_key_events = nullptr;
QueueHandle_t s_touch_events = nullptr;  // TouchPoint, one per change
std::atomic<bool> s_ok_held{false};
std::atomic<int64_t> s_ok_since{0};
// UP and DOWN as the watcher sees them now — index 0 UP, 1 DOWN — so a step is never sent for a key that has come up while
// its release waits in the queue. Since is written before held, as OK's are.
std::atomic<bool> s_step_held[2] = {};
std::atomic<int64_t> s_step_since[2] = {};

GT911 s_touch;
bool s_touch_ready = false;
TouchPoint s_last;
lv_indev_t *s_keys = nullptr;
lv_group_t *s_boot_group = nullptr;  // the group until SetGroup binds a screen's
lv_group_t *s_group = nullptr;       // the group the keypad moves through now
void (*s_back)() = nullptr;
using ReadHook = void (*)(lv_indev_t *indev, lv_indev_data_t *data);
ReadHook s_key_hook = nullptr;
ReadHook s_touch_hook = nullptr;
std::atomic<void (*)()> s_tick{nullptr};  // called from the input task: any task may run it
void (*s_key_seen)(uint32_t key) = nullptr;

// Every button's press, before LVGL turns UP and DOWN into focus moves that no object sees.
void KeySeen(uint32_t key)
{
    if (s_key_seen != nullptr) {
        s_key_seen(key);
    }
}

// UP's and DOWN's slot in s_step_held; -1 for OK.
int StepSlot(uint32_t key)
{
    return key == LV_KEY_PREV ? 0 : key == LV_KEY_NEXT ? 1 : -1;
}

// The button an LVGL key comes from, as key_rules.h names it.
Key KeyOf(uint32_t key)
{
    return key == LV_KEY_PREV ? Key::kUp : key == LV_KEY_NEXT ? Key::kDown : Key::kOk;
}

void Tick()
{
    void (*tick)() = s_tick;
    if (tick != nullptr) {
        tick();
    }
}

// Seeed's mapping from the GT911's native 800 x 480 to the portrait screen: x across the short edge, y inverted.
int16_t Scale(int value, int source_max, int target_max)
{
    value = std::clamp(value, 0, source_max);
    return static_cast<int16_t>((value * target_max + source_max / 2) / source_max);
}

// The touch panel as the watcher last saw it, in its own coordinates and the screen's.
TouchPoint ReadPanel(const TouchPoint &last)
{
    GTPoint points[GT911::kMaxTouchPoints] = {};
    const int8_t count = s_touch.read_points(points, GT911::kMaxTouchPoints);
    TouchPoint now = last;
    if (count == GT911::kNoUpdate || count < 0) {
        return now;  // no new sample, or a read that failed: the last report stands
    }
    if (count <= 0) {
        now.pressed = false;
        return now;
    }
    now.pressed = true;
    now.raw_x = static_cast<int16_t>(points[0].x);
    now.raw_y = static_cast<int16_t>(points[0].y);
    int16_t x = Scale(points[0].x, bc::kPanelWidth, bc::kScreenWidth - 1);
    int16_t y = Scale(bc::kPanelHeight - std::min<int>(points[0].y, bc::kPanelHeight), bc::kPanelHeight,
                      bc::kScreenHeight - 1);
#if CONFIG_FOLLOUP_DISPLAY_FLIP_180
    x = static_cast<int16_t>(bc::kScreenWidth - 1 - x);
    y = static_cast<int16_t>(bc::kScreenHeight - 1 - y);
#endif
    now.x = x;
    now.y = y;
    return now;
}

// The watcher: reads the buttons and the touch panel every kWatchMs whatever LVGL is doing, ticks on each press, and
// queues each change for LVGL to replay. The only reader of the GT911 and the button pins.
void WatchTask(void *)
{
    const gpio_num_t pins[3] = {bc::kButtonUp, bc::kButtonDown, bc::kPwrButton};
    const uint32_t keys[3] = {LV_KEY_PREV, LV_KEY_NEXT, LV_KEY_ENTER};
    bool was[3] = {};
    int64_t since[3] = {};
    TouchPoint touch;
    for (;;) {
        const int64_t now = esp_timer_get_time();
        for (int i = 0; i < 3; ++i) {
            const bool down = gpio_get_level(pins[i]) == 0;
            if (down == was[i]) {
                continue;
            }
            const int64_t began = down ? now : since[i];
            const KeyEvent e = {keys[i], down, began, now};
            if (xQueueSend(s_key_events, &e, 0) != pdTRUE) {
                continue;  // the queue is full: the change is read again next poll, never lost
            }
            was[i] = down;
            since[i] = began;
            if (keys[i] == LV_KEY_ENTER) {
                s_ok_since = began;  // before held: a reader that sees held sees this press's start
                s_ok_held = down;
            } else {
                s_step_since[StepSlot(keys[i])] = began;
                s_step_held[StepSlot(keys[i])] = down;
            }
            if (down) {
                Tick();
            }
        }
        if (s_touch_ready) {
            const TouchPoint next = ReadPanel(touch);
            const bool edge = next.pressed != touch.pressed;
            const bool moved = next.pressed && (next.x != touch.x || next.y != touch.y);
            // A press needs room for its release too, and so does a move, so a release always finds a place: the GT911
            // reports a lift once. A change that finds no room waits for the next poll.
            const UBaseType_t room = uxQueueSpacesAvailable(s_touch_events);
            if ((edge && (next.pressed ? room > 1 : room > 0)) || (!edge && moved && room > 1)) {
                xQueueSend(s_touch_events, &next, 0);
                if (edge && next.pressed) {
                    Tick();
                    ESP_LOGI(kTag, "touch raw (%d, %d) → screen (%d, %d)", next.raw_x, next.raw_y, next.x, next.y);
                }
                touch = next;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(kWatchMs));
    }
}

// LVGL's touch read: the watcher's changes in order, one a read, so a tap made during a refresh still presses and
// releases.
void ReadTouchNow(lv_indev_data_t *data)
{
    TouchPoint next;
    if (xQueueReceive(s_touch_events, &next, 0) == pdTRUE) {
        s_last = next;
        data->continue_reading = uxQueueMessagesWaiting(s_touch_events) > 0;
    }
    data->point.x = s_last.x;
    data->point.y = s_last.y;
    data->state = s_last.pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

void ReadTouch(lv_indev_t *indev, lv_indev_data_t *data)
{
    ReadTouchNow(data);
    if (ReadHook hook = s_touch_hook; hook != nullptr) {
        hook(indev, data);
    }
}

void GoBack(void *)
{
    if (s_back != nullptr) {
        s_back();
    }
}

// UP and DOWN as LVGL takes them: focus moves, or — while a control OK opened for editing has them, a range slider, a text
// view being read — that control's own keys, so they step it rather than move the focus; OK closes it (DESIGN → Components →
// The kit).
uint32_t KeyFor(uint32_t key)
{
    if (!lv_group_get_editing(Group())) {
        return key;
    }
    if (key == LV_KEY_PREV) {
        return LV_KEY_UP;
    }
    return key == LV_KEY_NEXT ? static_cast<uint32_t>(LV_KEY_DOWN) : key;
}

// LVGL's keypad read, from the watcher's queue. UP and DOWN press and release as they happened, each moving focus
// once; held, they step again by KeyRepeat — after `motion.key-repeat-delay`, then every `motion.key-repeat-period`, each
// step a release and a press that ticks as a press does, and only once the last step's frame is on the glass
// (key_rules.h; DESIGN → Focus and input). OK presses on release, so a hold can go back without pressing first, and never
// repeats. An OK pressed before the frame the last OK caused has been drawn — a second OK during a refresh — is dropped
// (DESIGN → Motion → Press); an OK that causes no frame opens the way again once kNoFrameUs passes.
void ReadKeysNow(lv_indev_data_t *data)
{
    static uint32_t key = LV_KEY_ENTER;
    static bool key_down = false;
    static bool enter_pending = false;
    static bool enter_sent = false;
    static OkHold ok_hold;               // the OK press a hold has already used to go back
    static int64_t enter_sent_at = 0;    // when the last OK reached LVGL
    static int64_t ok_gate = 0;          // an OK pressed before this is a second one, and dropped
    static bool awaiting_frame = false;  // the last OK's frame has not been drawn yet
    static KeyRepeat repeat;             // the held UP or DOWN
    static uint32_t repeat_key = LV_KEY_NEXT;
    static bool repeat_press_next = false;  // a step's release went out: its press is the next read
    constexpr int64_t kNoFrameUs = 100 * 1000;  // longer than LVGL's render period: the OK caused no frame

    if (awaiting_frame) {
        const int64_t end = LastRefreshEndUs();
        if (end > enter_sent_at) {
            ok_gate = end;  // the frame landed: an OK pressed from now on acts on it
            awaiting_frame = false;
        } else if (esp_timer_get_time() - enter_sent_at > kNoFrameUs) {
            ok_gate = enter_sent_at;
            awaiting_frame = false;
        }
    }

    // A hold still down: back once it has lasted kBackHoldUs, whether or not LVGL saw it begin. Held is read first:
    // the watcher writes the press's start before it.
    const bool held = s_ok_held;
    const int64_t since = s_ok_since;
    if (ok_hold.WhileDown(held, since, esp_timer_get_time())) {
        lv_async_call(GoBack, nullptr);  // after this read: going back deletes the screen the group points into
    }

    if (enter_pending) {
        data->key = LV_KEY_ENTER;
        data->state = enter_sent ? LV_INDEV_STATE_RELEASED : LV_INDEV_STATE_PRESSED;
        if (enter_sent) {
            enter_pending = false;
            enter_sent_at = esp_timer_get_time();
            awaiting_frame = true;
        }
        enter_sent = !enter_sent;
        data->continue_reading = true;
        return;
    }

    if (repeat_press_next) {
        // The step's press: a move as the first press was, with its tick, as every press has (DESIGN → Focus and input;
        // STICKY_DESIGN_TOKENS §5).
        // No event is taken between the two reads, so the key is still the held one.
        repeat_press_next = false;
        KeySeen(repeat_key);
        Tick();
        key = KeyFor(repeat_key);
        data->key = key;
        data->state = LV_INDEV_STATE_PRESSED;
        return;
    }

    KeyEvent e;
    if (xQueueReceive(s_key_events, &e, 0) == pdTRUE) {
        if (e.pressed) {
            KeySeen(e.key);
            if (KeyRepeats(KeyOf(e.key))) {
                repeat.Press(e.since_us, esp_timer_get_time());  // the last step is timed from when LVGL acts on it
                repeat_key = e.key;
            }
        } else if (e.key == repeat_key) {
            repeat.Release();  // the release stops the stepping at once
        }
        if (e.key == LV_KEY_ENTER) {
            if (!e.pressed && !ok_hold.Used(e.since_us) && !awaiting_frame && e.since_us >= ok_gate) {
                if (ok_hold.AtRelease(e.since_us, e.at_us)) {
                    lv_async_call(GoBack, nullptr);  // a hold that ended before LVGL could see it
                } else {
                    enter_pending = true;
                    enter_sent = false;
                }
            }
        } else {
            if (e.pressed) {
                key = KeyFor(e.key);
            }
            key_down = e.pressed;
        }
        data->continue_reading = true;
    } else if (key_down && repeat.held) {
        // The key is still down, by the watcher's word as well as the queue's: a step, when key_rules.h says one is due — a
        // release now and the press on the next read, in this same read cycle.
        const int slot = StepSlot(repeat_key);
        const bool still = slot >= 0 && s_step_held[slot] && s_step_since[slot] == repeat.since_us;
        if (still && repeat.Step(esp_timer_get_time(), LastRefreshEndUs(), RefreshBusy())) {
            repeat_press_next = true;
            data->key = key;
            data->state = LV_INDEV_STATE_RELEASED;
            data->continue_reading = true;
            return;
        }
    }
    data->key = key;
    data->state = key_down ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

void ReadKeys(lv_indev_t *indev, lv_indev_data_t *data)
{
    ReadKeysNow(data);
    if (ReadHook hook = s_key_hook; hook != nullptr) {
        hook(indev, data);
    }
}

}  // namespace

esp_err_t StartInput(bool *touch_ok)
{
    s_boot_group = lv_group_create();
    s_group = s_boot_group;
    lv_group_set_default(s_group);
    lv_group_set_wrap(s_group, true);

    s_key_events = xQueueCreate(32, sizeof(KeyEvent));
    s_touch_events = xQueueCreate(32, sizeof(TouchPoint));
    ESP_RETURN_ON_FALSE(s_key_events != nullptr && s_touch_events != nullptr, ESP_ERR_NO_MEM, kTag, "input queues");

    s_keys = lv_indev_create();
    lv_indev_set_type(s_keys, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(s_keys, ReadKeys);
    lv_indev_set_group(s_keys, s_group);
    // LVGL's own long-press repeat is off: it stepped every 100 ms after 400 ms and turned editing off on each repeat of
    // UP and DOWN. KeyRepeat steps instead, paced by the glass (key_rules.h).
    lv_indev_set_long_press_time(s_keys, UINT16_MAX);
    lv_indev_set_long_press_repeat_time(s_keys, UINT16_MAX);

    board::PowerTouch(true);
    // The GT911 driver tries its other address first, and the I2C driver logs that miss as an error: quiet it for the
    // probe, so the boot log's errors are real ones (tests/bench/boot_check.py).
    esp_log_level_set("i2c.master", ESP_LOG_NONE);
    s_touch_ready = s_touch.begin(bc::kTpInt, bc::kTpRst, bc::kPanelWidth, bc::kPanelHeight, board::TouchBus());
    esp_log_level_set("i2c.master", ESP_LOG_WARN);
    if (s_touch_ready) {
        lv_indev_t *touch = lv_indev_create();
        lv_indev_set_type(touch, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(touch, ReadTouch);
        ESP_LOGI(kTag, "GT911 at 0x%02X", s_touch.address());
    } else {
        ESP_LOGW(kTag, "GT911 did not answer: buttons only");
    }
    if (touch_ok != nullptr) {
        *touch_ok = s_touch_ready;
    }
    // Above LVGL's task (esp_lvgl_port's 4), so a press is read and ticked while a refresh holds LVGL's.
    ESP_RETURN_ON_FALSE(xTaskCreate(WatchTask, "eink_input", 3072, nullptr, 5, nullptr) == pdPASS, ESP_ERR_NO_MEM, kTag,
                        "input task");
    return ESP_OK;
}

lv_group_t *Group()
{
    return s_group;
}

void SetGroup(lv_group_t *group)
{
    s_group = group != nullptr ? group : s_boot_group;
    if (s_keys != nullptr) {
        lv_indev_set_group(s_keys, s_group);
    }
    lv_group_set_default(s_group);  // a control LVGL adds to the default group as it is made joins this one
}

void SetKeyReadHook(void (*hook)(lv_indev_t *indev, lv_indev_data_t *data))
{
    s_key_hook = hook;
}

void SetTouchReadHook(void (*hook)(lv_indev_t *indev, lv_indev_data_t *data))
{
    s_touch_hook = hook;
}

void ClearGroup()
{
    // A screen that goes while a control is open for editing must not leave the group editing: its next screen's UP and
    // DOWN would reach the focused control as keys instead of moving the focus (DESIGN → Focus and input).
    lv_group_set_editing(s_group, false);
    lv_group_remove_all_objs(s_group);
}

void SetBackHandler(void (*back)())
{
    s_back = back;
}

void SetPressFeedback(void (*tick)())
{
    s_tick = tick;
}

void SetKeyObserver(void (*seen)(uint32_t key))
{
    s_key_seen = seen;
}

void PressButton(Button button, bool hold)
{
    constexpr int64_t kPressUs = 80 * 1000;      // a quick press, well under the back hold
    constexpr int64_t kPastHoldUs = 100 * 1000;  // a hold that clearly passes it
    if (s_key_events == nullptr) {
        return;
    }
    const uint32_t key = button == Button::kUp ? LV_KEY_PREV : button == Button::kDown ? LV_KEY_NEXT : LV_KEY_ENTER;
    const bool ok = key == LV_KEY_ENTER;
    const int64_t began = esp_timer_get_time();
    const KeyEvent down = {key, true, began, began};
    xQueueSend(s_key_events, &down, portMAX_DELAY);
    if (ok) {
        s_ok_since = began;  // as the watcher: the start before held, so a reader that sees held sees this press's start
        s_ok_held = true;
    } else {
        s_step_since[StepSlot(key)] = began;
        s_step_held[StepSlot(key)] = true;
    }
    Tick();
    const int64_t length = ok && hold ? kBackHoldUs + kPastHoldUs : kPressUs;
    vTaskDelay(pdMS_TO_TICKS(length / 1000));
    if (ok) {
        s_ok_held = false;
    } else {
        s_step_held[StepSlot(key)] = false;
    }
    const KeyEvent up = {key, false, began, esp_timer_get_time()};
    xQueueSend(s_key_events, &up, portMAX_DELAY);
}

TouchPoint LastTouch()
{
    return s_last;
}

bool TouchReady()
{
    return s_touch_ready;
}

}  // namespace lvgl_display
