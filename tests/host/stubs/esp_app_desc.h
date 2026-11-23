// A stand-in for ESP-IDF's esp_app_desc.h, for the host tests: the names and the build time the About screen shows.
#pragma once
typedef struct {
    char version[32];
    char project_name[32];
    char time[16];
    char date[16];
    char idf_ver[32];
} esp_app_desc_t;
inline const esp_app_desc_t *esp_app_get_description()
{
    static const esp_app_desc_t kDesc = {"4fd8d28-dirty", "folloup", "08:00:00", "Nov 23 2026", "v6.1.1"};
    return &kDesc;
}
