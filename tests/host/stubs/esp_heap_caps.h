// A stand-in for ESP-IDF's esp_heap_caps.h, for the host tests: the router logs the internal heap on each push, and the
// computer has none to report.
#pragma once
#include <cstddef>
#include <cstdint>
#define MALLOC_CAP_INTERNAL (1 << 11)
inline size_t heap_caps_get_free_size(uint32_t) { return 0; }
inline size_t heap_caps_get_largest_free_block(uint32_t) { return 0; }
