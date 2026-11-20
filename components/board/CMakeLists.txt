# board brings the hardware up — the latch, the buses and the devices, in the order the hardware spec gives
# (STICKY_HARDWARE_SPECS §7). The shell calls it first; a feature never brings hardware up itself (APP_ARCHITECTURE
# §2). board.h hands out the panel's handle and the I2C buses, so those requirements are public.
idf_component_register(SRCS "board.cpp"
                            "board_display.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES epaper esp_driver_i2c
                       PRIV_REQUIRES board_config ssd1677 ssd2677 log esp_driver_gpio esp_driver_spi esp_adc esp_hw_support
                                     esp_system)
