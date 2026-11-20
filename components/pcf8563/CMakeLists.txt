# The PCF8563 real-time clock's driver, by Seeed Studio for the reTerminal Sticky (MIT, LICENSE beside it), as Seeed
# ships it in Seeed-Projects/sticky-esp-idf-peripherals at commit 4b7f9a1.
idf_component_register(SRCS "pcf8563.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_driver_i2c)
