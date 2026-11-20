// What lvgl_display's own files share; nothing outside it includes this.

#pragma once

#include "board.h"
#include "esp_err.h"

namespace lvgl_display {

esp_err_t StartDisplay(const board::Display &display);
void FirstFrame();  // the next frame is the first the glass shows
esp_err_t StartInput(bool *touch_ok);
int64_t LastRefreshEndUs();  // when the last refresh or scan ended, from esp_timer

}  // namespace lvgl_display
