# router is the screen stack, the overlay stack and the routes: each screen's and overlay's focus group, a screen's
# lifecycle, Push, Pop and GoTo by object or by id, with no transitions (APP_ARCHITECTURE §7). Its headers hand out LVGL's
# types and esp_err_t, so lvgl and esp_common are public requirements; design_tokens gives the slide-off slop; it reaches
# lvgl_display only to bind a group, to take holding OK and the read hooks, and to ask a full refresh as a full-glass overlay
# closes, and to sound the status line's news (Cue). It never requires ui or widgets: a screen styles its own root.
idf_component_register(SRCS "router.cpp"
                            "overlays.cpp"
                            "focus.cpp"
                            "touch.cpp"
                            "routes.cpp"
                            "screen.cpp"
                            "status.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES lvgl__lvgl esp_common
                       PRIV_REQUIRES design_tokens lvgl_display heap log)
