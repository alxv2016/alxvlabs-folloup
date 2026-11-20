# The GT911 touch controller's driver, by Seeed Studio for the reTerminal Sticky (MIT, LICENSE beside it), as Seeed
# ships it in Seeed-Projects/sticky-esp-idf-peripherals at commit 4b7f9a1. ESP-IDF 6's requirements are named here.
idf_component_register(SRCS "gt911.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_driver_i2c esp_driver_gpio esp_timer debug_logging)
