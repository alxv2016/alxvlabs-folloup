// The screen stack: Init, SetRoot, Push, GoTo, Pop, PopToRoot, Escape, and deleting a screen once nothing on the call stack
// is inside it (router.h; APP_ARCHITECTURE §7). The routes are routes.cpp's.

#include "router/router.h"

#include <new>
#include <utility>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "lvgl_display.h"
#include "router_internal.h"

namespace router {
namespace {

constexpr const char *kTag = "router";

struct Entry {
    std::unique_ptr<Screen> screen;
    lv_obj_t *root = nullptr;
    lv_group_t *group = nullptr;
};

Entry s_stack[kMaxDepth];
size_t s_depth = 0;
bool s_ready = false;

// A popped screen, waiting for its deletion.
struct Retired {
    std::unique_ptr<Screen> screen;
    lv_obj_t *root;
    lv_group_t *group;
};

// One deferred call deletes the whole screen, in this order: the group first — it touches only objects that still exist,
// and leaves them in no group — then the root and everything in it, then the Screen. Deferred because a pop is reached from
// an event of an object inside the screen going: that event is still on the stack.
void FreeRetired(void *user)
{
    Retired *retired = static_cast<Retired *>(user);
    if (retired->group != nullptr) {
        lv_group_delete(retired->group);
    }
    if (retired->root != nullptr) {
        lv_obj_delete(retired->root);
    }
    delete retired;  // the Screen with it
}

// The buttons are never bound to a group being deleted: the caller has bound the next screen's first.
void Retire(Entry &&entry)
{
    // Taken out of the entry first, so a failed allocation below still has all three to delete.
    lv_obj_t *root = entry.root;
    lv_group_t *group = entry.group;
    std::unique_ptr<Screen> screen = std::move(entry.screen);
    entry = Entry{};
    Retired *retired = new (std::nothrow) Retired{std::move(screen), root, group};
    if (retired != nullptr && lv_async_call(FreeRetired, retired) == LV_RESULT_OK) {
        return;
    }
    // No room to defer it all in one call: the root is queued for LVGL to delete after this event, the group and the Screen
    // go now — the group touches only objects that still exist, and no code of the Screen's is on the stack here.
    ESP_LOGE(kTag, "no memory to defer a screen's deletion: its objects queued for after this event, its group and Screen deleted now");
    if (retired != nullptr) {
        screen = std::move(retired->screen);
        delete retired;
    }
    lv_group_delete(group);
    lv_obj_delete_async(root);
}  // the Screen with `screen`

void LogHeap(size_t depth)
{
    ESP_LOGI(kTag, "push to depth %u: internal RAM %u bytes free, largest block %u", static_cast<unsigned>(depth),
             static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),
             static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)));
}

// Makes the root and the group, binds the group — so the controls OnCreate makes join it — and builds the screen. The root
// is press-locked: LVGL sets the flag on every object with a parent and a screen has none, so a press that begins on the
// background would otherwise be handed to whatever the finger slides over. A screen opens with no ring: the group still
// names the control lv_group_add_obj focused, and the ring a key press opening it put there is taken off (DESIGN → Focus
// and input). False with no memory, the group bound before restored.
bool MakeEntry(std::unique_ptr<Screen> screen, Entry *out)
{
    lv_obj_t *root = lv_obj_create(nullptr);
    lv_group_t *group = root != nullptr ? lv_group_create() : nullptr;
    if (group == nullptr) {
        ESP_LOGE(kTag, "no memory for a screen's root or group");
        if (root != nullptr) {
            lv_obj_delete(root);
        }
        return false;
    }
    lv_obj_set_press_lock(root, true);
    lv_group_set_wrap(group, true);  // UP past the first control reaches the last, DOWN past the last the first
    lv_group_set_editing(group, false);
    lvgl_display::SetGroup(group);
    screen->OnCreate(root, group);
    lv_obj_t *named = lv_group_get_focused(group);
    if (named != nullptr) {
        lv_obj_remove_state(named, LV_STATE_FOCUS_KEY);
    }
    *out = Entry{std::move(screen), root, group};
    return true;
}

Entry &Top()
{
    return s_stack[s_depth - 1];
}

// `entry` goes on the glass: its group bound and its root loaded, before its OnEnter.
void Show(const Entry &entry)
{
    lvgl_display::SetGroup(entry.group);
    lv_screen_load(entry.root);
}

esp_err_t CheckPush(const std::unique_ptr<Screen> &screen, size_t depth_after)
{
    if (!s_ready || s_depth == 0) {
        return ESP_ERR_INVALID_STATE;
    }
    if (screen == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    if (depth_after > kMaxDepth) {
        ESP_LOGW(kTag, "push refused: the stack is at its depth of %u", static_cast<unsigned>(kMaxDepth));
        return ESP_ERR_INVALID_STATE;
    }
    return ESP_OK;
}

// Holding OK, from lvgl_display's keypad read (an lv_async_call, under the lock). The hold is the buttons' input, though LVGL
// never saw an OK go down: a focus made after it rings as after a press (focus.cpp's Focus).
void OnHold()
{
    detail::NoteKeyInput(true);
    Escape();
}

}  // namespace

namespace detail {

Screen *TopScreen()
{
    return s_depth > 0 ? Top().screen.get() : nullptr;
}

}  // namespace detail

esp_err_t Init()
{
    if (s_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    s_ready = true;
    lvgl_display::SetBackHandler(OnHold);
    lvgl_display::SetKeyReadHook(detail::KeyReadHook);      // the first press with no ring only shows it (focus.cpp)
    lvgl_display::SetTouchReadHook(detail::TouchReadHook);  // a touch drops the ring (touch.cpp)
    return ESP_OK;
}

esp_err_t SetRoot(std::unique_ptr<Screen> screen)
{
    if (!s_ready || s_depth != 0) {
        return ESP_ERR_INVALID_STATE;
    }
    if (screen == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    Entry entry;
    if (!MakeEntry(std::move(screen), &entry)) {
        lvgl_display::SetGroup(nullptr);
        return ESP_ERR_NO_MEM;
    }
    s_stack[s_depth++] = std::move(entry);
    Show(Top());
    Top().screen->OnEnter();
    return ESP_OK;
}

esp_err_t Push(std::unique_ptr<Screen> screen)
{
    const esp_err_t check = CheckPush(screen, s_depth + 1);
    if (check != ESP_OK) {
        return check;
    }
    DismissOverlays();
    LogHeap(s_depth + 1);
    Entry entry;
    if (!MakeEntry(std::move(screen), &entry)) {
        lvgl_display::SetGroup(Top().group);
        return ESP_ERR_NO_MEM;
    }
    Top().screen->OnExit();
    s_stack[s_depth++] = std::move(entry);
    Show(Top());
    Top().screen->OnEnter();
    return ESP_OK;
}

esp_err_t GoTo(std::unique_ptr<Screen> screen)
{
    if (s_depth < 2) {
        return Push(std::move(screen));
    }
    const esp_err_t check = CheckPush(screen, 2);
    if (check != ESP_OK) {
        return check;
    }
    DismissOverlays();
    LogHeap(2);
    Entry entry;
    if (!MakeEntry(std::move(screen), &entry)) {
        lvgl_display::SetGroup(Top().group);
        return ESP_ERR_NO_MEM;
    }
    Top().screen->OnExit();
    // Bound and loaded first, so the buttons and the glass never point into a screen being retired.
    Show(entry);
    while (s_depth > 1) {
        Retire(std::move(s_stack[--s_depth]));
    }
    s_stack[s_depth++] = std::move(entry);
    Top().screen->OnEnter();
    return ESP_OK;
}

void Pop()
{
    if (!s_ready || s_depth < 2) {
        return;
    }
    DismissOverlays();
    Entry top = std::move(s_stack[--s_depth]);
    top.screen->OnExit();
    Show(Top());
    Top().screen->OnEnter();
    Retire(std::move(top));
}

void PopToRoot()
{
    if (!s_ready || s_depth < 2) {
        return;
    }
    DismissOverlays();
    Top().screen->OnExit();
    Show(s_stack[0]);
    while (s_depth > 1) {
        Retire(std::move(s_stack[--s_depth]));
    }
    Top().screen->OnEnter();
}

size_t Depth()
{
    return s_depth;
}

bool Escape()
{
    // First, a control open for editing — a range slider, a text view being read: it hears LV_EVENT_CANCEL, which puts back
    // what it had when OK opened it, and closes.
    if (lv_group_t *group = lvgl_display::Group(); group != nullptr && lv_group_get_editing(group)) {
        if (lv_obj_t *focused = lv_group_get_focused(group); focused != nullptr) {
            lv_obj_send_event(focused, LV_EVENT_CANCEL, nullptr);
        }
        lv_group_set_editing(group, false);
        return true;
    }
    if (detail::EscapeTopOverlay()) {
        return true;
    }
    if (s_depth > 1) {
        Pop();
        return true;
    }
    return false;
}

}  // namespace router
