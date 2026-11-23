// A stand-in for ESP-IDF's esp_system.h, for the host tests: the reset reason the hardware test's power screen reads.
#pragma once
typedef enum { ESP_RST_UNKNOWN, ESP_RST_POWERON, ESP_RST_EXT, ESP_RST_SW, ESP_RST_PANIC, ESP_RST_INT_WDT, ESP_RST_TASK_WDT,
               ESP_RST_WDT, ESP_RST_DEEPSLEEP } esp_reset_reason_t;
inline esp_reset_reason_t esp_reset_reason() { return ESP_RST_POWERON; }
