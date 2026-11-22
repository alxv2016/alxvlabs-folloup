// A stand-in for ESP-IDF's esp_check.h, for the host tests.
#pragma once
#include "esp_err.h"
#define ESP_RETURN_ON_FALSE(a, err, tag, fmt, ...) do { (void)(tag); if (!(a)) { return (err); } } while (0)
