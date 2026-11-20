#include "lvgl_display.h"

#include "lvgl_display_priv.h"
#include "esp_check.h"
#include "esp_lvgl_port.h"

namespace lvgl_display {

esp_err_t Start(const board::Display &display, bool *touch_ok, void (*first_screen)())
{
    // esp_lvgl_port runs LVGL's task, tick and lock; the display and inputs are ours, since it has no e-paper path.
    lvgl_port_cfg_t port = ESP_LVGL_PORT_INIT_CONFIG();
    port.task_stack = 8192;  // the flush packs and sends the frame on this task
    ESP_RETURN_ON_ERROR(lvgl_port_init(&port), "lvgl_display", "LVGL port");

    if (!Lock()) {
        return ESP_ERR_TIMEOUT;
    }
    esp_err_t err = StartDisplay(display);
    if (err == ESP_OK) {
        err = StartInput(touch_ok);
    }
    // The first screen under the same lock, so LVGL's task never draws the empty one before it.
    if (err == ESP_OK && first_screen != nullptr) {
        first_screen();
        FirstFrame();
        lv_refr_now(nullptr);
    }
    Unlock();
    return err;
}

}  // namespace lvgl_display
