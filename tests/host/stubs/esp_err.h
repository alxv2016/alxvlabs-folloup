// A stand-in for ESP-IDF's esp_err.h, for the host tests.
#pragma once
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_ERR_NOT_FOUND 0x105
