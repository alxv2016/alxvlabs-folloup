// A stand-in for ESP-IDF's esp_mac.h, for the host tests: a fixed MAC address.
#pragma once
#include <cstdint>
#include "esp_err.h"
typedef enum { ESP_MAC_WIFI_STA } esp_mac_type_t;
inline esp_err_t esp_read_mac(uint8_t *mac, esp_mac_type_t)
{
    const uint8_t kMac[6] = {0x24, 0x58, 0x7C, 0x01, 0x02, 0x03};
    for (int i = 0; i < 6; ++i) {
        mac[i] = kMac[i];
    }
    return ESP_OK;
}
