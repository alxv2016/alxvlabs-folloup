# Seeed's debug-log macros, which the gt911 driver uses (MIT, LICENSE beside it). Its Kconfig switch is not carried, so
# the extra touch logs stay off; ESP_LOG levels still apply.
idf_component_register(INCLUDE_DIRS "include"
                       REQUIRES log)
