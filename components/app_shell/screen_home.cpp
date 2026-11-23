#include "screen_home.h"

#include "app_hooks.h"
#include "ui.h"
#include "lvgl.h"

namespace screen_home {

void Build(lv_obj_t *home, lv_group_t *, void *)
{
    ui::StyleScreen(home);  // the glass, a column, no scrolling
    ui::NewStatusBar(home, "FolloUp");

    // The shell's own content on home goes here; then each feature's, in registration order.
    app_hooks::BuildHome(home);
}

}  // namespace screen_home
