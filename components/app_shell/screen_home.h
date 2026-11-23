// The root screen. The router calls Build under the LVGL lock, into the root it made, with home's focus group bound: it styles
// the root as the glass, and lets each feature add to it (app_hooks::OnHomeBuilt) — a control made then joins home's group.

#pragma once

#include "lvgl.h"

namespace screen_home {

void Build(lv_obj_t *home, lv_group_t *group, void *user);

}  // namespace screen_home
