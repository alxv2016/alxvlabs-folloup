// MakeScreen: a Screen from free functions (screen.h).

#include <new>

#include "router/screen.h"

namespace router {
namespace {

class FunctionScreen final : public Screen {
public:
    FunctionScreen(CreateFn create, LifecycleFn enter, LifecycleFn exit, CoveredFn covered, void *user)
        : create_(create), enter_(enter), exit_(exit), covered_(covered), user_(user)
    {
    }

    void OnCreate(lv_obj_t *root, lv_group_t *group) override
    {
        if (create_ != nullptr) {
            create_(root, group, user_);
        }
    }
    void OnEnter() override
    {
        if (enter_ != nullptr) {
            enter_(user_);
        }
    }
    void OnExit() override
    {
        if (exit_ != nullptr) {
            exit_(user_);
        }
    }
    void OnCovered(bool covered) override
    {
        if (covered_ != nullptr) {
            covered_(covered, user_);
        }
    }

private:
    CreateFn create_;
    LifecycleFn enter_;
    LifecycleFn exit_;
    CoveredFn covered_;
    void *user_;
};

}  // namespace

std::unique_ptr<Screen> MakeScreen(CreateFn create, LifecycleFn enter, LifecycleFn exit, CoveredFn covered, void *user)
{
    return std::unique_ptr<Screen>(new (std::nothrow) FunctionScreen(create, enter, exit, covered, user));
}

}  // namespace router
