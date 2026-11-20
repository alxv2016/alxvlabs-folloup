# app_hooks is where the core and the features plug in: each Install() registers here, and app_shell runs what was
# registered as it boots. It requires no feature and not the shell — every arrow points at it (APP_ARCHITECTURE §1).
idf_component_register(SRCS "app_hooks.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES lvgl__lvgl
                       PRIV_REQUIRES log)
