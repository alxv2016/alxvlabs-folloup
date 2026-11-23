// A stand-in for ESP-IDF's driver/uart.h, for the host tests: no UART, so the hardware test's console never starts.
#pragma once
#include <cstddef>
#include <cstdint>
#include "esp_err.h"
typedef int uart_port_t;
#define UART_NUM_0 0
inline esp_err_t uart_driver_install(uart_port_t, int, int, int, void *, int) { return ESP_ERR_NOT_FOUND; }
inline int uart_read_bytes(uart_port_t, void *, uint32_t, uint32_t) { return -1; }
