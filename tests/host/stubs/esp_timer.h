// A stand-in for ESP-IDF's esp_timer.h, for the host tests: a clock that never moves.
#pragma once
#include <cstdint>
inline int64_t esp_timer_get_time() { return 0; }
