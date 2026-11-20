#include "screen_home.h"

#include "app_hooks.h"
#include "ui.h"
#include "lvgl.h"

namespace screen_home {

void Show()
{
    lv_obj_t *home = ui::NewScreen();  // the glass, a column, no scrolling
    ui::NewStatusBar(home, "FolloUp");

    // The shell's own content on home goes here; then each feature's, in registration order.
    app_hooks::BuildHome(home);

    lv_screen_load(home);
}

}  // namespace screen_home
