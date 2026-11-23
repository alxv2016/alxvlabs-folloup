// A stand-in for ESP-IDF's esp_chip_info.h, for the host tests: an ESP32-S3 at revision 0.2.
#pragma once
#include <cstdint>
typedef struct {
    uint16_t revision;
} esp_chip_info_t;
inline void esp_chip_info(esp_chip_info_t *out) { out->revision = 2; }
