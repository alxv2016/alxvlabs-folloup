#include <cstring>

#include "epaper.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace epaper {
namespace {

constexpr const char *kTag = "epaper";
constexpr size_t kChunkBytes = 4096;  // two of these, in DMA-capable internal RAM

esp_err_t AddDevice(spi_host_device_t host, int hz, spi_device_handle_t *out)
{
    spi_device_interface_config_t config = {};
    config.clock_speed_hz = hz;
    config.mode = 0;
    config.spics_io_num = -1;  // the select is ours: one pin for both devices
    config.flags = SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_3WIRE;
    config.queue_size = 2;
    return spi_bus_add_device(host, &config, out);
}

}  // namespace

uint32_t ElapsedMs(int64_t since_us)
{
    return static_cast<uint32_t>((esp_timer_get_time() - since_us) / 1000);
}

esp_err_t Bus::Init(const BusConfig &config)
{
    config_ = config;
    // The board has already set these outputs idle; only their direction is (re)asserted here.
    for (gpio_num_t pin : {config.cs, config.dc, config.rst}) {
        ESP_RETURN_ON_ERROR(gpio_set_direction(pin, GPIO_MODE_OUTPUT), kTag, "control pin %d", pin);
    }
    gpio_set_level(config.cs, 1);
    gpio_set_level(config.dc, 1);
    gpio_set_level(config.rst, 1);

    gpio_config_t in = {};
    in.pin_bit_mask = 1ULL << config.busy;
    in.mode = GPIO_MODE_INPUT;
    ESP_RETURN_ON_ERROR(gpio_config(&in), kTag, "BUSY pin");

    ESP_RETURN_ON_ERROR(AddDevice(config.host, config.read_hz, &read_), kTag, "read device");
    ESP_RETURN_ON_ERROR(AddDevice(config.host, config.write_hz, &write_), kTag, "write device");
    for (uint8_t *&chunk : chunks_) {
        chunk = static_cast<uint8_t *>(heap_caps_malloc(kChunkBytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
        ESP_RETURN_ON_FALSE(chunk != nullptr, ESP_ERR_NO_MEM, kTag, "stream buffers");
    }
    return ESP_OK;
}

esp_err_t Bus::Send(spi_device_handle_t device, const uint8_t *data, size_t len)
{
    while (len > 0) {
        const size_t n = len > kChunkBytes ? kChunkBytes : len;
        std::memcpy(chunks_[0], data, n);  // DMA reads from internal RAM
        spi_transaction_t t = {};
        t.length = n * 8;
        t.tx_buffer = chunks_[0];
        ESP_RETURN_ON_ERROR(spi_device_transmit(device, &t), kTag, "transmit");
        data += n;
        len -= n;
    }
    return ESP_OK;
}

esp_err_t Bus::Command(uint8_t command)
{
    gpio_set_level(config_.cs, 0);
    gpio_set_level(config_.dc, 0);
    const esp_err_t err = Send(write_, &command, 1);
    gpio_set_level(config_.dc, 1);
    gpio_set_level(config_.cs, 1);
    return err;
}

esp_err_t Bus::Data(const uint8_t *data, size_t len)
{
    gpio_set_level(config_.cs, 0);
    gpio_set_level(config_.dc, 1);
    const esp_err_t err = Send(write_, data, len);
    gpio_set_level(config_.cs, 1);
    return err;
}

esp_err_t Bus::Write(uint8_t command, std::initializer_list<uint8_t> data)
{
    ESP_RETURN_ON_ERROR(Command(command), kTag, "command 0x%02X", command);
    return data.size() == 0 ? ESP_OK : Data(data.begin(), data.size());
}

esp_err_t Bus::Read(uint8_t *out)
{
    gpio_set_level(config_.cs, 0);
    gpio_set_level(config_.dc, 1);
    spi_transaction_t t = {};
    t.flags = SPI_TRANS_USE_RXDATA;
    t.rxlength = 8;
    const esp_err_t err = spi_device_transmit(read_, &t);
    gpio_set_level(config_.cs, 1);
    ESP_RETURN_ON_ERROR(err, kTag, "read");
    *out = t.rx_data[0];
    return ESP_OK;
}

esp_err_t Bus::Stream(size_t len, const std::function<void(uint8_t *, size_t, size_t)> &fill)
{
    gpio_set_level(config_.cs, 0);
    gpio_set_level(config_.dc, 1);
    spi_transaction_t t[2] = {};
    esp_err_t err = ESP_OK;
    size_t offset = 0;
    int queued = -1;  // the chunk on the wire, or -1
    for (int i = 0; offset < len && err == ESP_OK; i ^= 1) {
        const size_t n = len - offset > kChunkBytes ? kChunkBytes : len - offset;
        fill(chunks_[i], offset, n);  // while the other chunk is still sending
        if (queued >= 0) {
            spi_transaction_t *done = nullptr;
            err = spi_device_get_trans_result(write_, &done, portMAX_DELAY);
            queued = -1;
        }
        if (err == ESP_OK) {
            t[i] = {};
            t[i].length = n * 8;
            t[i].tx_buffer = chunks_[i];
            err = spi_device_queue_trans(write_, &t[i], portMAX_DELAY);
            queued = err == ESP_OK ? i : -1;  // a chunk that never queued has no result to wait for
        }
        offset += n;
    }
    if (queued >= 0) {
        spi_transaction_t *done = nullptr;
        const esp_err_t last = spi_device_get_trans_result(write_, &done, portMAX_DELAY);
        err = err == ESP_OK ? last : err;
    }
    gpio_set_level(config_.cs, 1);
    return err;
}

void Bus::Reset(uint32_t low_ms, uint32_t high_ms)
{
    gpio_set_level(config_.rst, 0);
    vTaskDelay(pdMS_TO_TICKS(low_ms));
    gpio_set_level(config_.rst, 1);
    vTaskDelay(pdMS_TO_TICKS(high_ms));
}

esp_err_t Bus::WaitReady(int busy_level, uint32_t timeout_ms)
{
    const int64_t start = esp_timer_get_time();
    while (gpio_get_level(config_.busy) == busy_level) {
        if (ElapsedMs(start) > timeout_ms) {
            ESP_LOGE(kTag, "BUSY timeout after %lu ms", static_cast<unsigned long>(timeout_ms));
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(1);
    }
    return ESP_OK;
}

esp_err_t ReadId(Bus &bus, uint8_t *id)
{
    bus.Reset(20, 10);  // the SSD2677's timing, which the probe assumes
    ESP_RETURN_ON_ERROR(bus.Command(0x70), kTag, "ID command");
    return bus.Read(id);
}

}  // namespace epaper
