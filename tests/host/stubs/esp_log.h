// A stand-in for ESP-IDF's esp_log.h, for the host tests: logs nothing, and takes the tag and every argument, so none is left
// unused.
#pragma once
namespace host_stub {
template <typename... Args>
inline void LogNothing(const char *, const char *, const Args &...) {}
}  // namespace host_stub
#define ESP_LOGI(tag, ...) ::host_stub::LogNothing(tag, __VA_ARGS__)
#define ESP_LOGW(tag, ...) ::host_stub::LogNothing(tag, __VA_ARGS__)
#define ESP_LOGE(tag, ...) ::host_stub::LogNothing(tag, __VA_ARGS__)
