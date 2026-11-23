// A stand-in for ESP-IDF's esp_psram.h, for the host tests: the ESP32-S3R8's 8 MB.
#pragma once
#include <cstddef>
inline size_t esp_psram_get_size() { return 8u * 1024 * 1024; }
