# devices reads the Sticky's sensors, plays the buzzer's sounds (a note player on LEDC, sound_rules.h), listens on the
# microphone and probes the card, the way Seeed's drivers and demos do (STICKY_HARDWARE_SPECS §4). It needs board's buses
# up first; devices.h exposes only plain values, and its sounds in design_tokens' note format (sounds.h), so
# design_tokens is its one public requirement.
idf_component_register(SRCS "devices.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES design_tokens
                       PRIV_REQUIRES board board_config bq27220 pcf8563 esp_driver_i2c esp_driver_gpio
                                     esp_driver_ledc esp_driver_i2s esp_driver_sdspi sdmmc fatfs esp_timer log)
