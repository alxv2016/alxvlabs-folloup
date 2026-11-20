# ui is DESIGN.md's look in LVGL: Inter at each type role, rendered by LVGL's Tiny TTF from the embedded fonts (licence:
# fonts/OFL.txt), and the components built from the token header (STICKY_DESIGN_TOKENS §3, §6).
idf_component_register(SRCS "ui.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES lvgl__lvgl
                       PRIV_REQUIRES lvgl_display design_tokens log
                       EMBED_FILES "fonts/inter_600.ttf"
                                   "fonts/inter_700.ttf")
