// A stand-in for FreeRTOS's task.h, for the host tests: a task is never created, and a delay returns at once.
#pragma once
#include <cstdint>
#include "freertos/FreeRTOS.h"
typedef void *TaskHandle_t;
inline BaseType_t xTaskCreate(void (*)(void *), const char *, uint32_t, void *, unsigned, TaskHandle_t *) { return pdFAIL; }
inline void vTaskDelay(TickType_t) {}
inline void vTaskDelete(TaskHandle_t) {}
