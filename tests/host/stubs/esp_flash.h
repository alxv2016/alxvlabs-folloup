// A stand-in for ESP-IDF's esp_flash.h, for the host tests: the Sticky's 32 MB (STICKY_HARDWARE_SPECS §4).
#pragma once
#include <cstdint>
#include "esp_err.h"
typedef struct esp_flash_t esp_flash_t;
inline esp_err_t esp_flash_get_size(esp_flash_t *, uint32_t *size)
{
    *size = 32u * 1024 * 1024;
    return ESP_OK;
}
