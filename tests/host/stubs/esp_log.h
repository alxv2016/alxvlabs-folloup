// A stand-in for ESP-IDF's esp_log.h, for the host tests: logs nothing, and uses the tag so none is left unused.
#pragma once
#define ESP_LOGI(tag, ...) ((void)(tag))
#define ESP_LOGW(tag, ...) ((void)(tag))
#define ESP_LOGE(tag, ...) ((void)(tag))
