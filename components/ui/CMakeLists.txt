# ui is DESIGN.md's look in LVGL: Bitcount Single at each type role, compiled at one bit a pixel (fonts/bitcount, licence
# fonts/bitcount/OFL.txt), and the components built from the token header (STICKY_DESIGN_TOKENS §3, §6).
# Regenerate the Bitcount files with fonts/bitcount/make_fonts.sh.
idf_component_register(SRCS "ui.cpp"
                            "line_box.cpp"
                            "fonts/bitcount/bitcount_400_30.c"
                            "fonts/bitcount/bitcount_600_20.c"
                            "fonts/bitcount/bitcount_600_30.c"
                            "fonts/bitcount/bitcount_600_40.c"
                            "fonts/bitcount/bitcount_600_50.c"
                            "fonts/bitcount/bitcount_600_60.c"
                            "fonts/bitcount/bitcount_600_70.c"
                            "fonts/bitcount/bitcount_600_80.c"
                            "fonts/bitcount/bitcount_600_100.c"
                            "fonts/bitcount/bitcount_600_120.c"
                            "fonts/bitcount/bitcount_600_160.c"
                            "fonts/bitcount/bitcount_700_20.c"
                            "fonts/bitcount/bitcount_700_30.c"
                            "fonts/bitcount/bitcount_700_40.c"
                            "fonts/bitcount/bitcount_700_50.c"
                            "fonts/bitcount/bitcount_700_60.c"
                            "fonts/bitcount/bitcount_700_70.c"
                            "fonts/bitcount/bitcount_700_80.c"
                            "fonts/bitcount/bitcount_700_100.c"
                            "fonts/bitcount/bitcount_700_120.c"
                            "fonts/bitcount/bitcount_700_160.c"
                       INCLUDE_DIRS "include"
                       REQUIRES lvgl__lvgl
                       PRIV_REQUIRES lvgl_display design_tokens log)
# The compiled fonts include "lvgl.h" by this name, as the component manager lays LVGL out.
target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_LVGL_H_INCLUDE_SIMPLE=1)
