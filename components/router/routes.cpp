// The route registry: the tables features add, finding a route by id, and PushRoute and GoToRoute, which build the screen with
// its factory and hand it to Push and GoTo (route.h; APP_ARCHITECTURE §7).

#include <cstring>
#include <utility>

#include "esp_log.h"
#include "router/router.h"

namespace router {
namespace {

constexpr const char *kTag = "router";

// A handful of tables of a few routes each, searched in order: a navigation is one press. Raise the size here when a
// feature's table is refused for room.
constexpr size_t kMaxRouteTables = 8;
RouteTable s_tables[kMaxRouteTables] = {};
size_t s_table_count = 0;

// The screen for `id` and `param`, or nullptr with the reason in `err`, logged.
std::unique_ptr<Screen> Build(const char *id, const char *param, esp_err_t *err)
{
    const Route *route = FindRoute(id);
    if (route == nullptr) {
        ESP_LOGW(kTag, "no route '%s'", id != nullptr ? id : "(null)");
        *err = ESP_ERR_NOT_FOUND;
        return nullptr;
    }
    std::unique_ptr<Screen> screen = route->make(param);
    if (screen == nullptr) {
        ESP_LOGW(kTag, "route '%s' refused param '%s'", id, param != nullptr ? param : "(null)");
        *err = ESP_ERR_INVALID_ARG;
        return nullptr;
    }
    *err = ESP_OK;
    return screen;
}

}  // namespace

esp_err_t AddRoutes(const RouteTable &table)
{
    if (table.routes == nullptr && table.count > 0) {
        return ESP_ERR_INVALID_ARG;
    }
    for (size_t i = 0; i < table.count; ++i) {
        const Route &route = table.routes[i];
        if (route.id == nullptr || route.make == nullptr) {
            ESP_LOGE(kTag, "route %u of a table has no id or no factory: table rejected", static_cast<unsigned>(i));
            return ESP_ERR_INVALID_ARG;
        }
        bool taken = FindRoute(route.id) != nullptr;
        for (size_t j = 0; j < i && !taken; ++j) {
            taken = std::strcmp(table.routes[j].id, route.id) == 0;
        }
        if (taken) {
            ESP_LOGE(kTag, "route id '%s' is taken: table rejected", route.id);
            return ESP_ERR_INVALID_ARG;
        }
    }
    if (s_table_count == kMaxRouteTables) {
        ESP_LOGE(kTag, "no room for another route table: raise kMaxRouteTables");
        return ESP_ERR_NO_MEM;
    }
    s_tables[s_table_count++] = table;
    return ESP_OK;
}

const Route *FindRoute(const char *id)
{
    if (id == nullptr) {
        return nullptr;
    }
    for (size_t t = 0; t < s_table_count; ++t) {
        for (size_t i = 0; i < s_tables[t].count; ++i) {
            if (std::strcmp(s_tables[t].routes[i].id, id) == 0) {
                return &s_tables[t].routes[i];
            }
        }
    }
    return nullptr;
}

esp_err_t PushRoute(const char *id, const char *param)
{
    esp_err_t err = ESP_OK;
    std::unique_ptr<Screen> screen = Build(id, param, &err);
    return screen != nullptr ? Push(std::move(screen)) : err;
}

esp_err_t GoToRoute(const char *id, const char *param)
{
    esp_err_t err = ESP_OK;
    std::unique_ptr<Screen> screen = Build(id, param, &err);
    return screen != nullptr ? GoTo(std::move(screen)) : err;
}

}  // namespace router
