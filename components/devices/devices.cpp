#include "devices.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <dirent.h>

#include "board.h"
#include "board_config.h"
#include "bq27220.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/i2s_pdm.h"
#include "driver/ledc.h"
#include "driver/sdspi_host.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pcf8563.h"
#include "sdmmc_cmd.h"

namespace devices {
namespace {

namespace bc = board_config;
constexpr const char *kTag = "devices";

Present s_present;
i2c_master_dev_handle_t s_sht40 = nullptr, s_imu = nullptr, s_gauge = nullptr, s_rtc = nullptr;
bool s_gauge_scaled = false;

constexpr int kI2cTimeoutMs = 100;

esp_err_t AddDevice(uint8_t address, i2c_master_dev_handle_t *out)
{
    i2c_device_config_t config = {};
    config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    config.device_address = address;
    config.scl_speed_hz = bc::kI2cHz;
    return i2c_master_bus_add_device(board::SensorBus(), &config, out);
}

esp_err_t WriteReg(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t value)
{
    const uint8_t data[2] = {reg, value};
    return i2c_master_transmit(dev, data, sizeof(data), kI2cTimeoutMs);
}

esp_err_t ReadRegs(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t *out, size_t len)
{
    return i2c_master_transmit_receive(dev, &reg, 1, out, len, kI2cTimeoutMs);
}

// Sensirion's CRC-8: polynomial 0x31, initial 0xFF.
uint8_t Crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            crc = static_cast<uint8_t>((crc & 0x80) ? (crc << 1) ^ 0x31 : crc << 1);
        }
    }
    return crc;
}

bool ProbeSht40()
{
    if (AddDevice(bc::kAddrSht40, &s_sht40) != ESP_OK) {
        return false;
    }
    Ambient ambient;
    return ReadAmbient(&ambient) == ESP_OK;
}

bool ProbeImu()
{
    if (AddDevice(bc::kAddrLsm6ds3, &s_imu) != ESP_OK) {
        return false;
    }
    uint8_t who = 0;
    if (ReadRegs(s_imu, 0x0F, &who, 1) != ESP_OK || who != 0x6A) {
        ESP_LOGW(kTag, "IMU WHO_AM_I 0x%02X", who);
        return false;
    }
    // Seeed's set-up: auto-increment with block data update; accelerometer and gyro at 104 Hz, ±2 g and 250 dps.
    return WriteReg(s_imu, 0x12, 0x44) == ESP_OK && WriteReg(s_imu, 0x10, 0x40) == ESP_OK &&
           WriteReg(s_imu, 0x11, 0x40) == ESP_OK;
}

bool ProbeGauge()
{
    if (AddDevice(bc::kAddrBq27220, &s_gauge) != ESP_OK || !bq27220_probe(s_gauge)) {
        return false;
    }
    // The stock profile's DesignCapacity reads 7500 for the 750 mAh cell: current and capacity read ten times high
    // (STICKY_HARDWARE_SPECS §4.3).
    uint16_t design = 0;
    s_gauge_scaled = bq27220_read_word(s_gauge, 0x3C, design) && design == 7500;
    ESP_LOGI(kTag, "gauge design capacity raw %u%s", design, s_gauge_scaled ? ", divided by 10" : "");
    return true;
}

// PCF8563 registers, from its datasheet: Control_status_2 holds the alarm and timer flags and their interrupt enables;
// the four alarm registers are off with bit 7 set; the timer counts at 1 Hz with TD = 10.
constexpr uint8_t kRtcControl2 = 0x01;
constexpr uint8_t kRtcTimerFlag = 0x04;    // TF
constexpr uint8_t kRtcAlarmFlag = 0x08;    // AF
constexpr uint8_t kRtcTimerIrq = 0x01;     // TIE; TI_TP left 0, so RTC_INTn stays low while TF is set
constexpr uint8_t kRtcAlarmMinute = 0x09;  // to 0x0C: minute, hour, day, weekday
constexpr uint8_t kRtcAlarmOff = 0x80;     // AE
constexpr uint8_t kRtcTimerControl = 0x0E;
constexpr uint8_t kRtcTimerOn1Hz = 0x82;   // TE, TD = 0b10
constexpr uint8_t kRtcTimerIdle = 0x03;    // TE off, TD = 0b11, 1/60 Hz: the datasheet's low-current setting
constexpr uint8_t kRtcTimerCount = 0x0F;
bool s_rtc_started_board = false;

bool ProbeRtc()
{
    if (AddDevice(bc::kAddrPcf8563, &s_rtc) != ESP_OK || !pcf8563_probe(s_rtc)) {
        return false;
    }
    pcf8563_disable_clkout(s_rtc);  // Seeed's: the clock output is not used
    uint8_t status = 0;
    if (ReadRegs(s_rtc, kRtcControl2, &status, 1) == ESP_OK) {
        s_rtc_started_board = (status & (kRtcTimerFlag | kRtcAlarmFlag)) != 0;
    }
    ClearRtcWake();
    return true;
}

// --- The sound player ----------------------------------------------------------------------------------------------
//
// One task owns the LEDC channel — ESP-IDF's ledc_set_duty and ledc_update_duty are not thread-safe — and plays one sound
// at a time from sound_rules.h's FrameAt: woken at once by a request, then every kStepMs while a sound plays, asleep
// while none does. A request replaces the one before it under a spinlock and notifies the task, so Play, Tick and Tone
// return at once from any task, and a new sound cuts the one playing: a tick never waits behind a melody. Each wake
// reads the time since the sound began, so a late wake never stretches a sound (STICKY_HARDWARE_SPECS §4.2).

// Above the input watcher (5), which ticks on every press, and LVGL's task (4): the tick reaches the coil within a
// context switch of the press, even while a frame renders.
constexpr UBaseType_t kSoundTaskPriority = 6;
constexpr uint32_t kSoundTaskStack = 2560;

struct SoundRequest {
    sound::Sound sound = {};
    sound::Note tone = {};  // Tone's one note, kept by value: the caller's stack is gone by the time it plays
    bool is_tone = false;
};

portMUX_TYPE s_sound_lock = portMUX_INITIALIZER_UNLOCKED;
SoundRequest s_sound_request;
uint32_t s_sound_requests = 0;  // counts requests, so the task sees a new one even when it is the same sound again
TaskHandle_t s_sound_task = nullptr;

void SetBuzzer(uint32_t hz, uint32_t duty, uint32_t *hz_now, uint32_t *duty_now)
{
    // A rest leaves the timer where it is (hz 0); a pitch the timer cannot divide to is skipped, and the last one plays on.
    if (hz != 0 && hz != *hz_now && ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, hz) == ESP_OK) {
        *hz_now = hz;
    }
    if (duty != *duty_now) {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        *duty_now = duty;
    }
}

void SoundTask(void *)
{
    uint32_t seen = 0;
    sound::Note tone = {};
    sound::Sound playing = {};
    int64_t started_us = 0;
    uint32_t hz_now = bc::kBuzzerHz;
    uint32_t duty_now = 0;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, playing.count > 0 ? pdMS_TO_TICKS(sound::kStepMs) : portMAX_DELAY);
        taskENTER_CRITICAL(&s_sound_lock);
        const uint32_t requests = s_sound_requests;
        const SoundRequest request = s_sound_request;
        taskEXIT_CRITICAL(&s_sound_lock);
        if (requests != seen) {
            seen = requests;
            if (request.is_tone) {
                tone = request.tone;
                playing = {&tone, 1};
            } else {
                playing = request.sound;
            }
            started_us = esp_timer_get_time();
        }
        const uint32_t elapsed_ms = static_cast<uint32_t>((esp_timer_get_time() - started_us) / 1000);
        const sound::Frame frame = sound::FrameAt(playing, elapsed_ms, bc::kBuzzerDuty);
        SetBuzzer(frame.hz, frame.duty, &hz_now, &duty_now);
        if (frame.done) {
            playing = {};
        }
    }
}

void Ask(const SoundRequest &request)
{
    if (s_sound_task == nullptr) {
        return;
    }
    taskENTER_CRITICAL(&s_sound_lock);
    s_sound_request = request;
    ++s_sound_requests;
    taskEXIT_CRITICAL(&s_sound_lock);
    xTaskNotifyGive(s_sound_task);
}

bool InitBuzzer()
{
    static_assert(sound::kResonanceHz == bc::kBuzzerHz && design_tokens::kSoundTick[0].hz == bc::kBuzzerHz,
                  "the press tick sounds where Seeed drives the buzzer");
    ledc_timer_config_t timer = {};
    timer.speed_mode = LEDC_LOW_SPEED_MODE;
    timer.duty_resolution = LEDC_TIMER_10_BIT;
    timer.timer_num = LEDC_TIMER_0;
    timer.freq_hz = bc::kBuzzerHz;
    // The 80 MHz APB clock, named rather than chosen: every pitch from sound::kLowestHz to kHighestHz divides from it at
    // 10 bits, so ledc_set_freq reaches each note.
    timer.clk_cfg = LEDC_USE_APB_CLK;
    if (ledc_timer_config(&timer) != ESP_OK) {
        return false;
    }
    ledc_channel_config_t channel = {};
    channel.gpio_num = bc::kBuzzerPwm;
    channel.speed_mode = LEDC_LOW_SPEED_MODE;
    channel.channel = LEDC_CHANNEL_0;
    channel.timer_sel = LEDC_TIMER_0;
    channel.duty = 0;
    if (ledc_channel_config(&channel) != ESP_OK) {
        return false;
    }
    return xTaskCreate(SoundTask, "buzzer", kSoundTaskStack, nullptr, kSoundTaskPriority, &s_sound_task) == pdPASS;
}

}  // namespace

esp_err_t Init()
{
    s_present.sht40 = ProbeSht40();
    s_present.imu = ProbeImu();
    s_present.gauge = ProbeGauge();
    s_present.rtc = ProbeRtc();
    s_present.buzzer = InitBuzzer();
    ESP_LOGI(kTag, "SHT40 %s, IMU %s, gauge %s, RTC %s, buzzer %s", s_present.sht40 ? "ok" : "absent",
             s_present.imu ? "ok" : "absent", s_present.gauge ? "ok" : "absent", s_present.rtc ? "ok" : "absent",
             s_present.buzzer ? "ok" : "absent");
    return ESP_OK;
}

Present Found()
{
    return s_present;
}

esp_err_t ReadAmbient(Ambient *out)
{
    ESP_RETURN_ON_FALSE(s_sht40 != nullptr && out != nullptr, ESP_ERR_INVALID_STATE, kTag, "SHT40");
    const uint8_t command = 0xFD;  // high repeatability
    ESP_RETURN_ON_ERROR(i2c_master_transmit(s_sht40, &command, 1, kI2cTimeoutMs), kTag, "SHT40 measure");
    vTaskDelay(pdMS_TO_TICKS(10));
    uint8_t data[6] = {};
    ESP_RETURN_ON_ERROR(i2c_master_receive(s_sht40, data, sizeof(data), kI2cTimeoutMs), kTag, "SHT40 read");
    ESP_RETURN_ON_FALSE(Crc8(data, 2) == data[2] && Crc8(data + 3, 2) == data[5], ESP_ERR_INVALID_CRC, kTag,
                        "SHT40 CRC");
    const uint16_t t = static_cast<uint16_t>((data[0] << 8) | data[1]);
    const uint16_t rh = static_cast<uint16_t>((data[3] << 8) | data[4]);
    out->temperature_c = -45.0f + 175.0f * t / 65535.0f;
    out->humidity_pct = std::fmin(100.0f, std::fmax(0.0f, -6.0f + 125.0f * rh / 65535.0f));
    return ESP_OK;
}

esp_err_t ReadMotion(Motion *out)
{
    ESP_RETURN_ON_FALSE(s_present.imu && out != nullptr, ESP_ERR_INVALID_STATE, kTag, "IMU");
    uint8_t raw[12] = {};
    ESP_RETURN_ON_ERROR(ReadRegs(s_imu, 0x22, raw, sizeof(raw)), kTag, "IMU read");  // gyro X Y Z, then accel X Y Z
    for (int axis = 0; axis < 3; ++axis) {
        const int16_t g = static_cast<int16_t>(raw[axis * 2] | (raw[axis * 2 + 1] << 8));
        const int16_t a = static_cast<int16_t>(raw[6 + axis * 2] | (raw[6 + axis * 2 + 1] << 8));
        out->gyro_dps[axis] = g * 0.00875f;
        out->accel_g[axis] = a * 0.000061f;
    }
    return ESP_OK;
}

esp_err_t ReadBattery(Battery *out)
{
    ESP_RETURN_ON_FALSE(s_present.gauge && out != nullptr, ESP_ERR_INVALID_STATE, kTag, "gauge");
    uint16_t mv = 0, soc = 0, soh = 0, remaining = 0, full = 0, cycles = 0;
    int16_t ma = 0;
    float temp = 0;
    bool ok = bq27220_read_voltage_mv(s_gauge, mv);
    ok = bq27220_read_current_ma(s_gauge, ma) && ok;
    ok = bq27220_read_state_of_charge(s_gauge, soc) && ok;
    bq27220_read_state_of_health(s_gauge, soh);
    bq27220_read_remaining_capacity_mah(s_gauge, remaining);
    bq27220_read_full_charge_capacity_mah(s_gauge, full);
    bq27220_read_temperature_c(s_gauge, temp);
    bq27220_read_cycle_count(s_gauge, cycles);
    ESP_RETURN_ON_FALSE(ok, ESP_FAIL, kTag, "gauge read");
    const int divide = s_gauge_scaled ? 10 : 1;
    out->voltage_mv = mv;
    out->current_ma = ma / divide;
    out->percent = soc;
    out->health_pct = soh;
    out->remaining_mah = remaining / divide;
    out->full_mah = full / divide;
    out->temperature_c = temp;
    out->cycles = cycles;
    out->scaled = s_gauge_scaled;
    return ESP_OK;
}

esp_err_t ReadClock(std::tm *out)
{
    ESP_RETURN_ON_FALSE(s_present.rtc && out != nullptr, ESP_ERR_INVALID_STATE, kTag, "RTC");
    return pcf8563_get_time(s_rtc, *out) ? ESP_OK : ESP_FAIL;
}

bool RtcStartedBoard()
{
    return s_rtc_started_board;
}

esp_err_t ClearRtcWake()
{
    ESP_RETURN_ON_FALSE(s_rtc != nullptr, ESP_ERR_INVALID_STATE, kTag, "no RTC");
    ESP_RETURN_ON_ERROR(WriteReg(s_rtc, kRtcTimerControl, kRtcTimerIdle), kTag, "RTC timer off");
    for (uint8_t reg = kRtcAlarmMinute; reg < kRtcAlarmMinute + 4; ++reg) {
        ESP_RETURN_ON_ERROR(WriteReg(s_rtc, reg, kRtcAlarmOff), kTag, "RTC alarm off");
    }
    return WriteReg(s_rtc, kRtcControl2, 0x00);  // flags cleared, interrupts disabled
}

esp_err_t SetRtcWake(uint8_t seconds)
{
    ESP_RETURN_ON_FALSE(seconds > 0, ESP_ERR_INVALID_ARG, kTag, "a wake needs a count");
    ESP_RETURN_ON_ERROR(ClearRtcWake(), kTag, "RTC clear");
    ESP_RETURN_ON_ERROR(WriteReg(s_rtc, kRtcTimerCount, seconds), kTag, "RTC count");
    ESP_RETURN_ON_ERROR(WriteReg(s_rtc, kRtcControl2, kRtcTimerIrq), kTag, "RTC timer interrupt");
    return WriteReg(s_rtc, kRtcTimerControl, kRtcTimerOn1Hz);
}

void Play(const sound::Sound &sound)
{
    SoundRequest request;
    request.sound = sound;
    Ask(request);
}

void Silence()
{
    Ask(SoundRequest{});
}

void Tone(uint32_t ms)
{
    SoundRequest request;
    request.tone = {sound::Wave::kPulse50, static_cast<uint16_t>(bc::kBuzzerHz), 0,
                    static_cast<uint16_t>(std::min<uint32_t>(ms, UINT16_MAX)), 255};
    request.is_tone = true;
    Ask(request);
}

void Play(sound::Word word)
{
    Play(sound::SoundOf(word));
}

void Tick()
{
    Play(sound::Word::kTick);
}

// The microphone's channel while a listener has it open: StartListening to StopListening.
i2s_chan_handle_t s_mic = nullptr;

esp_err_t StartListening(uint32_t settle_ms)
{
    ESP_RETURN_ON_FALSE(s_mic == nullptr, ESP_ERR_INVALID_STATE, kTag, "the microphone is open already");
    // PDM_EN is an output since board::Init drove it low: set its level, never configure it again — gpio_config on a pin
    // already driven logs "conflict found" each time.
    gpio_set_level(bc::kPdmEn, 1);
    vTaskDelay(pdMS_TO_TICKS(bc::kMicPowerSettleMs));

    i2s_chan_handle_t rx = nullptr;
    i2s_chan_config_t chan = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    esp_err_t err = i2s_new_channel(&chan, nullptr, &rx);
    i2s_pdm_rx_config_t pdm = {};
    pdm.clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(16000);
    pdm.slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO);
    pdm.gpio_cfg.clk = bc::kPdmClk;
    pdm.gpio_cfg.din = bc::kPdmData;
    if (err == ESP_OK) {
        err = i2s_channel_init_pdm_rx_mode(rx, &pdm);
    }
    if (err == ESP_OK) {
        err = i2s_channel_enable(rx);
        if (err != ESP_OK) {
            i2s_del_channel(rx);
        }
    } else if (rx != nullptr) {
        i2s_del_channel(rx);
    }
    if (err != ESP_OK) {
        gpio_set_level(bc::kPdmEn, 0);
        ESP_RETURN_ON_ERROR(err, kTag, "PDM open");
    }
    s_mic = rx;
    // The microphone's power-up and the filter's start come out as a slow swing far louder than a room: read and drop
    // them, so no level holds them (STICKY_HARDWARE_SPECS §4.2).
    int16_t samples[512];
    size_t dropped = 0;
    const size_t settle = 16 * std::max<uint32_t>(settle_ms, bc::kMicLevelSettleMs);  // 16 samples a millisecond
    while (dropped < settle && err == ESP_OK) {
        size_t got = 0;
        err = i2s_channel_read(s_mic, samples, sizeof(samples), &got, 1000);
        dropped += got / sizeof(int16_t);
    }
    if (err != ESP_OK) {
        StopListening();
    }
    return err;
}

esp_err_t ReadLevel(uint32_t ms, float *rms, int *peak)
{
    ESP_RETURN_ON_FALSE(rms != nullptr && peak != nullptr, ESP_ERR_INVALID_ARG, kTag, "outputs");
    ESP_RETURN_ON_FALSE(s_mic != nullptr, ESP_ERR_INVALID_STATE, kTag, "the microphone is not open");
    double sum = 0, sum_sq = 0;
    size_t count = 0;
    int max_abs = 0;
    esp_err_t err = ESP_OK;
    int16_t samples[512];
    const size_t wanted = 16 * ms;  // 16 samples a millisecond
    while (count < wanted && err == ESP_OK) {
        size_t got = 0;
        err = i2s_channel_read(s_mic, samples, sizeof(samples), &got, 1000);
        for (size_t i = 0; i < got / sizeof(int16_t); ++i) {
            sum += samples[i];
            sum_sq += static_cast<double>(samples[i]) * samples[i];
            max_abs = std::max(max_abs, std::abs(static_cast<int>(samples[i])));
        }
        count += got / sizeof(int16_t);
    }
    ESP_RETURN_ON_ERROR(err, kTag, "PDM read");
    ESP_RETURN_ON_FALSE(count > 0, ESP_ERR_INVALID_SIZE, kTag, "no samples");
    const double mean = sum / count;
    *rms = static_cast<float>(std::sqrt(std::fmax(0.0, sum_sq / count - mean * mean)));
    *peak = max_abs;
    return ESP_OK;
}

void StopListening()
{
    if (s_mic != nullptr) {
        i2s_channel_disable(s_mic);
        i2s_del_channel(s_mic);
        s_mic = nullptr;
    }
    gpio_set_level(bc::kPdmEn, 0);
}

esp_err_t ListenLevel(uint32_t ms, float *rms, int *peak)
{
    ESP_RETURN_ON_FALSE(rms != nullptr && peak != nullptr, ESP_ERR_INVALID_ARG, kTag, "outputs");
    ESP_RETURN_ON_ERROR(StartListening(bc::kMicLevelSettleMs), kTag, "microphone");
    const esp_err_t err = ReadLevel(ms, rms, peak);
    StopListening();
    return err;
}

esp_err_t ProbeCard(Card *out)
{
    ESP_RETURN_ON_FALSE(out != nullptr, ESP_ERR_INVALID_ARG, kTag, "output");
    *out = Card{};
    // Card detect is low with a card in, pulled up by board::Init (STICKY_HARDWARE_SPECS §3.3).
    out->inserted = gpio_get_level(bc::kSdDetect) == 0;
    if (!out->inserted) {
        return ESP_ERR_NOT_FOUND;
    }
    constexpr const char *kMount = "/sdcard";
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = bc::kSharedSpiHost;
    sdspi_device_config_t slot = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot.gpio_cs = bc::kSdD3;
    slot.host_id = static_cast<spi_host_device_t>(bc::kSharedSpiHost);
    esp_vfs_fat_sdmmc_mount_config_t mount = {};
    mount.format_if_mount_failed = false;  // never format someone's card
    mount.max_files = 3;
    mount.allocation_unit_size = 16 * 1024;

    sdmmc_card_t *card = nullptr;
    board::LockSharedBus();  // the panel waits while the card has the bus (STICKY_HARDWARE_SPECS §1.2)
    esp_err_t err = esp_vfs_fat_sdspi_mount(kMount, &host, &slot, &mount, &card);
    if (err == ESP_OK) {
        out->mounted = true;
        out->capacity_mib = static_cast<uint64_t>(card->csd.capacity) * card->csd.sector_size / (1024 * 1024);
        std::strncpy(out->name, card->cid.name, sizeof(out->name) - 1);
        if (DIR *dir = opendir(kMount)) {
            while (readdir(dir) != nullptr && out->entries < 99) {
                ++out->entries;
            }
            closedir(dir);
        }
        // Unmount removes the card's device from the bus; the bus itself stays up for the panel.
        const esp_err_t unmount = esp_vfs_fat_sdcard_unmount(kMount, card);
        if (unmount != ESP_OK) {
            ESP_LOGE(kTag, "unmount: %s", esp_err_to_name(unmount));
        }
    }
    gpio_set_level(bc::kSdD3, 1);  // deselect, as Seeed's demo leaves it
    board::UnlockSharedBus();
    ESP_RETURN_ON_ERROR(err, kTag, "mount");
    ESP_LOGI(kTag, "card %s: %llu MiB, %d entries in the root", out->name,
             static_cast<unsigned long long>(out->capacity_mib), out->entries);
    return ESP_OK;
}

}  // namespace devices
