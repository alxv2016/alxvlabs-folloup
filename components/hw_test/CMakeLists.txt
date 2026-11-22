# hw_test is the bench feature: a row on home per part of the board, and a screen that exercises it on V1.0 and V2.0
# alike. It meets the shell only through app_hooks (APP_ARCHITECTURE §2).
idf_component_register(SRCS "hw_test.cpp"
                       PRIV_REQUIRES app_hooks board devices design_tokens lvgl_display ui widgets lvgl__lvgl esp_app_format esp_hw_support esp_timer
                                     esp_psram spi_flash esp_driver_uart freertos log)
