# devices reads the Sticky's sensors, sounds the buzzer, listens on the microphone and probes the card, the way
# Seeed's drivers and demos do (STICKY_HARDWARE_SPECS §4). It needs board's buses up first; devices.h exposes only
# plain values, so every requirement is private.
idf_component_register(SRCS "devices.cpp"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES board board_config bq27220 pcf8563 esp_driver_i2c esp_driver_gpio
                                     esp_driver_ledc esp_driver_i2s esp_driver_sdspi sdmmc fatfs esp_timer log)
