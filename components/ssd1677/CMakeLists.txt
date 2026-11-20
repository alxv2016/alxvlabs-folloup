# ssd1677 drives the reTerminal Sticky V1.0's controller (STICKY_HARDWARE_SPECS §4.5), behind epaper's Panel.
idf_component_register(SRCS "ssd1677.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES epaper
                       PRIV_REQUIRES esp_timer)
