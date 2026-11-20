# lvgl_display puts LVGL on the e-paper glass: the display and its refresh policy over the controller's driver
# (ssd1677 or ssd2677, through epaper), the GT911 and the buttons as inputs (STICKY_DESIGN_TOKENS §5). lvgl_display.h
# hands out board::Display and LVGL's types, so board and lvgl are public requirements.
idf_component_register(SRCS "lvgl_display.cpp"
                            "display.cpp"
                            "input.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES board lvgl__lvgl
                       PRIV_REQUIRES board_config design_tokens epaper gt911 espressif__esp_lvgl_port esp_driver_gpio
                                     esp_timer log)
