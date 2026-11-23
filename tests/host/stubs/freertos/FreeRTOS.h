// A stand-in for FreeRTOS.h, for the host tests: the types and macros the hardware test names; no scheduler.
#pragma once
#include <cstdint>
typedef uint32_t TickType_t;
typedef int BaseType_t;
#define pdPASS 1
#define pdFAIL 0
#define portMAX_DELAY 0xFFFFFFFFu
#define pdMS_TO_TICKS(ms) static_cast<TickType_t>(ms)
