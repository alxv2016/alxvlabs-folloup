// A screen as an address: an id anything can push, and the factory that builds the screen (APP_ARCHITECTURE §7).
//
// A route is how a screen is reached without its caller knowing what it is: a home row, a bench command and another screen
// push "hw.display" alike, and none of them includes the screen's code. A feature registers its table with AddRoutes in a
// kBoardUp boot step (APP_ARCHITECTURE §3), before anything navigates.
//
// THE ID IS THE CONTRACT. Lowercase, unique across the app, dotted for a screen that belongs to another: "hw.display",
// "hw.display.grays". It stays when the screen's title or code changes.
//
// A PARAM IS THE ONE ARGUMENT A SCREEN TAKES — a string, valid only for the factory's call; nullptr for none.

#pragma once

#include <cstddef>
#include <memory>

#include "router/screen.h"

namespace router {

// Builds the screen for `param`, or returns nullptr to refuse a param it does not understand: the navigation then fails with
// ESP_ERR_INVALID_ARG and nothing on the glass changes.
using RouteFactory = std::unique_ptr<Screen> (*)(const char *param);

struct Route {
    const char *id;
    RouteFactory make;
};

// What a feature registers: its routes, a static table that outlives the firmware.
struct RouteTable {
    const Route *routes;
    size_t count;
};

// A factory for a screen that takes no argument: it refuses any param but none.
template <std::unique_ptr<Screen> (*Make)()>
std::unique_ptr<Screen> NoParam(const char *param)
{
    if (param != nullptr && param[0] != '\0') {
        return nullptr;
    }
    return Make();
}

}  // namespace router
