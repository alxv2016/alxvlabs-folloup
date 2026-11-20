# epaper is the glass as every controller presents it: the Panel interface and the SPI bus the controllers share. The
# controllers are their own components — ssd1677 and ssd2677 (STICKY_HARDWARE_SPECS §4.5). epaper.h hands out SPI and
# GPIO types, so those requirements are public.
idf_component_register(SRCS "epaper_bus.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_driver_spi esp_driver_gpio
                       PRIV_REQUIRES esp_timer log)
