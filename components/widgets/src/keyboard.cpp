#include "widgets/keyboard.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "design_tokens.h"
#include "internal/base.h"
#include "internal/keyboard_pages.h"
#include "lvgl_display.h"
#include "router/router.h"
#include "widgets/button.h"

namespace widgets {
namespace dt = design_tokens;
using namespace detail;  // the page tables

namespace {
// The glass's width, from its tokens: the content column and a margin each side.
constexpr int32_t kGlassWidth = dt::kSpacingContent + 2 * dt::kSpacingEdge;
constexpr int32_t kPitch = dt::kSpacingTarget + dt::kSpacingKeyGap;                            // a slot and the gap after it
constexpr int32_t kGridWidth = kKeySlots * dt::kSpacingTarget + (kKeySlots - 1) * dt::kSpacingKeyGap;  // 444
constexpr int32_t kGridLeft = (kGlassWidth - kGridWidth) / 2;                                    // 18
constexpr int32_t kGridHeight = kKeyRows * dt::kSpacingTarget + (kKeyRows - 1) * dt::kSpacingKeyGap;  // 444
// Down the body, which starts under the status bar: the field, the count line and the grid; the footer stands under it.
constexpr int32_t kFieldY = dt::kSpacingMd;                                                      // 16
constexpr int32_t kFieldHeight = dt::kSpacingTarget;
constexpr int32_t kCountY = kFieldY + kFieldHeight + dt::kSpacingSm;                             // 88
constexpr int32_t kGridY = kCountY + dt::kTypographyCaption.line_px + dt::kSpacingMd;          // 128: the count line and a gap

struct KeyboardState;
struct KeyCtx {
    KeyboardState *kb;
    const KeyDef *def;
    int row;
    int index;
};

struct KeyboardState {
    KeyboardConfig config;
    std::string title, action_words;
    bool shift = false;
    KeyPage page = KeyPage::kLower;
    bool finished = false;
    router::Overlay overlay;  // the keyboard on the router's overlay stack, full-glass: its screen, its group, holding OK

    lv_obj_t *screen = nullptr;
    lv_obj_t *count = nullptr;
    lv_obj_t *action = nullptr;
    lv_obj_t *cancel = nullptr;
    lv_obj_t *field_frame = nullptr;  // the field's frame, the walk's first stop, when a field joined
    lv_obj_t *field_stop = nullptr;   // the field's own button (the eye or the cross), the walk's second stop, when the field has one
    lv_obj_t *rows[kKeyRows] = {};
    lv_obj_t *keys[kKeyRows][kMaxKeysInRow] = {};
    int key_count[kKeyRows] = {};
    KeyCtx key_ctx[kKeyRows][kMaxKeysInRow] = {};
};

KeyboardState *s_active = nullptr;

constexpr int32_t SlotX(int slot)
{
    return slot * kPitch;
}
constexpr int32_t SlotsWidth(int slots)
{
    return slots * dt::kSpacingTarget + (slots - 1) * dt::kSpacingKeyGap;
}

// --- The text ------------------------------------------------------------------------------------------------------------

// What has been typed: the target's, which is the field component's when it lands.
const char *Typed(const KeyboardState *kb)
{
    const char *text = kb->config.target.text != nullptr ? kb->config.target.text(kb->config.target.user) : nullptr;
    return text != nullptr ? text : "";
}

// Everything the text changes here: the count line, and whether the action may be pressed.
void Refresh(KeyboardState *kb)
{
    const int n = static_cast<int>(std::strlen(Typed(kb)));
    char count[64];
    if (n < kb->config.min_length) {
        std::snprintf(count, sizeof count, kb->config.min_length == 1 ? "%d of %d character needed" : "%d of %d characters needed", n,
                      kb->config.min_length);
    } else {
        std::snprintf(count, sizeof count, n == 1 ? "%d character" : "%d characters", n);
    }
    lv_label_set_text(kb->count, count);
    lv_obj_set_state(kb->action, LV_STATE_DISABLED, n < kb->config.min_length);
}

// A press that changes the text, or switches the page, counts its frame toward the partial limit; a ring move does not (and
// adds to the ceiling on the uncounted ones instead, glass_rules.h `kUncountedPartialLimit`).
void Changed(KeyboardState *kb)
{
    lvgl_display::NoteTyping();
    Refresh(kb);
}

void TypeChar(KeyboardState *kb, char c)
{
    if (static_cast<int>(std::strlen(Typed(kb))) >= kb->config.max_length) {
        return;  // the press ticked; the field is full
    }
    if (kb->config.target.append == nullptr) {
        return;
    }
    kb->config.target.append(c, kb->config.target.user);
    Changed(kb);
}

// Closes: off the overlay stack — the screen it opened over comes back, drawn full (the router asks it: DESIGN → Motion), focus
// on what opened it — its screen deleted after this event, then the answer, when there is one to give. What was typed is still
// in the target: the field in the slot goes with the screen, after the answer.
void Close(KeyboardState *kb, bool answer, bool accepted)
{
    if (kb->finished) {
        return;
    }
    kb->finished = true;
    lvgl_display::SetPartialCounting(lvgl_display::PartialCounting::kEvery);
    router::CloseOverlay(&kb->overlay);
    lv_obj_delete_async(kb->screen);  // its delete event frees `kb`
    if (answer && kb->config.done != nullptr) {
        kb->config.done(accepted, kb->config.user);
    }
}

void Finish(KeyboardState *kb, bool accepted)
{
    Close(kb, true, accepted);
}

// Holding OK cancels at once: there is no row level to leave (DESIGN → Focus and input).
void EscapeOverlay(void *user)
{
    Finish(static_cast<KeyboardState *>(user), false);
}

// What replaces the glass closes it with no answer.
void DismissOverlay(void *user)
{
    Close(static_cast<KeyboardState *>(user), false, false);
}

// --- The keys and the walk ----------------------------------------------------------------------------------------------

void SwitchPage(KeyboardState *kb, KeyPage page);

void Press(KeyboardState *kb, const KeyDef &def)
{
    switch (def.kind) {
    case KeyKind::kChar:
        TypeChar(kb, def.label[0]);
        break;
    case KeyKind::kSpace:
        TypeChar(kb, ' ');
        break;
    case KeyKind::kDelete:
        if (Typed(kb)[0] != '\0' && kb->config.target.erase != nullptr) {
            kb->config.target.erase(kb->config.target.user);
            Changed(kb);
        }
        break;
    case KeyKind::kShift:
        kb->shift = !kb->shift;
        SwitchPage(kb, kb->shift ? KeyPage::kUpper : KeyPage::kLower);
        break;
    case KeyKind::kToLetters:
        SwitchPage(kb, kb->shift ? KeyPage::kUpper : KeyPage::kLower);
        break;
    case KeyKind::kToNumbers:
        SwitchPage(kb, KeyPage::kNumbers);
        break;
    case KeyKind::kToSymbols:
        SwitchPage(kb, KeyPage::kSymbols);
        break;
    }
}

// Every key acts on release, on the key (WCAG 2.5.2): LVGL sends CLICKED only when the finger lifts on the key it went down on,
// and a finger that slides off sends PRESS_LOST instead, which cancels the press. OK on the focused key arrives as the same click.
// The tick already sounded on contact, in the input watcher.
void OnKeyClicked(lv_event_t *e)
{
    KeyCtx *k = static_cast<KeyCtx *>(lv_event_get_user_data(e));
    Press(k->kb, *k->def);
}

lv_obj_t *MakeKey(lv_obj_t *row, const KeyboardState *kb, const KeyDef &def, KeyCtx *ctx)
{
    const bool chrome = def.kind != KeyKind::kChar;
    const bool chosen = def.kind == KeyKind::kShift && kb->shift;
    lv_obj_t *key = Bare(row);
    lv_obj_add_flag(key, LV_OBJ_FLAG_CLICKABLE);  // a touch on a key types it on release; the walk makes it a focus stop
    lv_obj_set_size(key, SlotsWidth(def.slots), dt::kSpacingTarget);
    lv_obj_set_style_radius(key, dt::kRoundedMd, 0);
    Outline(key, dt::kSpacingStrokeDefault);
    lv_obj_set_style_border_opa(key, chrome ? LV_OPA_COVER : LV_OPA_TRANSP, 0);  // a character key is the light dither, no edge
    lv_obj_set_style_bg_opa(key, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(key, chosen ? ui::Level(dt::kColorSurfaceChosen)
                                    : chrome ? ui::Level(dt::kColorBackground)
                                             : ui::Fill(dt::kColorBand),
                              0);
    lv_obj_set_style_text_color(key, ui::Level(chosen ? dt::kColorTextOnFill : dt::kColorTextPrimary), 0);
    lv_obj_set_flex_flow(key, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(key, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(key, dt::kSpacingSm, 0);
    lv_obj_t *word = lv_label_create(key);
    lv_label_set_text(word, def.label);
    lv_obj_set_style_text_font(word, ui::Font(ui::Type::kLabel), 0);
    ui::RingOutside(key);  // the one focus ring, `spacing.stroke.focus`; a key is never open for editing
    lv_obj_add_event_cb(key, OnKeyClicked, LV_EVENT_CLICKED, ctx);
    return key;
}

// The six rows made again for a page: each as wide as its keys, hidden when it has none.
void BuildKeys(KeyboardState *kb)
{
    const RowDef *rows = PageRows(kb->page);
    for (int r = 0; r < kKeyRows; ++r) {
        lv_obj_t *row = kb->rows[r];
        lv_obj_clean(row);
        const RowDef &def = rows[r];
        kb->key_count[r] = def.count;
        if (def.count == 0) {
            lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        lv_obj_remove_flag(row, LV_OBJ_FLAG_HIDDEN);
        const int first = RowFirstSlot(def);
        lv_obj_set_pos(row, kGridLeft + SlotX(first), r * kPitch);
        lv_obj_set_size(row, SlotX(RowEndSlot(def) - 1) + dt::kSpacingTarget - SlotX(first), dt::kSpacingTarget);
        for (int i = 0; i < def.count; ++i) {
            KeyCtx *ctx = &kb->key_ctx[r][i];
            *ctx = KeyCtx{kb, &def.keys[i], r, i};
            lv_obj_t *key = MakeKey(row, kb, def.keys[i], ctx);
            lv_obj_set_x(key, SlotX(def.keys[i].slot) - SlotX(first));
            kb->keys[r][i] = key;
        }
    }
}

// The walk, in the order of the glass: the field, its button, every key row by row left to right, Cancel, the action, and round
// again. No row level and no editing: UP and DOWN step through the keys, OK presses the focused one.
void BuildWalk(KeyboardState *kb)
{
    lv_group_t *group = kb->overlay.group;
    lv_group_set_editing(group, false);
    lv_group_remove_all_objs(group);
    if (kb->field_frame != nullptr) {
        lv_group_add_obj(group, kb->field_frame);
    }
    if (kb->field_stop != nullptr) {
        lv_group_add_obj(group, kb->field_stop);
    }
    for (int r = 0; r < kKeyRows; ++r) {
        for (int i = 0; i < kb->key_count[r]; ++i) {
            lv_group_add_obj(group, kb->keys[r][i]);
        }
    }
    lv_group_add_obj(group, kb->cancel);
    lv_group_add_obj(group, kb->action);
}

// A page switch keeps the place: the same row and the nearest slot of it when a key had the focus, so a switch never loses where
// the ring was; the focus stays on another stop when that had it.
void SwitchPage(KeyboardState *kb, KeyPage page)
{
    lv_obj_t *had = lv_group_get_focused(kb->overlay.group);
    int row = -1;
    int slot = 0;
    for (int r = 0; r < kKeyRows && row < 0; ++r) {
        for (int i = 0; i < kb->key_count[r]; ++i) {
            if (kb->keys[r][i] == had) {
                row = r;
                slot = kb->key_ctx[r][i].def->slot;
                break;
            }
        }
    }
    kb->page = page;
    lv_group_remove_all_objs(kb->overlay.group);  // the old keys go with their group places; none is left to be deleted while focused
    BuildKeys(kb);
    BuildWalk(kb);
    lvgl_display::NoteTyping();  // a page switch counts as one partial, as a typing press does
    lv_obj_t *to = had;
    if (row >= 0) {
        to = nullptr;
        int best = 99;
        for (int i = 0; i < kb->key_count[row]; ++i) {
            const int distance = std::abs(kb->key_ctx[row][i].def->slot - slot);
            if (distance < best) {
                best = distance;
                to = kb->keys[row][i];
            }
        }
        if (to == nullptr) {  // the page has no keys in that row: the nearest row above that has
            for (int r = row - 1; r >= 0 && to == nullptr; --r) {
                if (kb->key_count[r] > 0) {
                    to = kb->keys[r][0];
                }
            }
        }
    }
    if (to != nullptr) {
        router::Focus(to);
    }
}

void OnActionClicked(lv_event_t *e)
{
    Finish(static_cast<KeyboardState *>(lv_event_get_user_data(e)), true);
}

void OnCancelClicked(lv_event_t *e)
{
    Finish(static_cast<KeyboardState *>(lv_event_get_user_data(e)), false);
}

void OnScreenDelete(lv_event_t *e)
{
    KeyboardState *kb = static_cast<KeyboardState *>(lv_event_get_user_data(e));
    if (!kb->finished) {
        router::CloseOverlay(&kb->overlay);  // deleted some other way than closing: off the overlay stack first
    }
    if (s_active == kb) {
        s_active = nullptr;
        // a screen deleted any other way than Finish leaves the rule as it was — unless a newer keyboard has set its own
        lvgl_display::SetPartialCounting(lvgl_display::PartialCounting::kEvery);
    }
    delete kb;
}

}  // namespace

Keyboard OpenKeyboard(const KeyboardConfig &config)
{
    if (s_active != nullptr && !s_active->finished) {
        return Keyboard{nullptr, nullptr};  // one at a time
    }
    KeyboardState *kb = new KeyboardState{};
    lv_obj_t *screen = ui::NewScreen();
    // A full-glass overlay: opening loads its screen and binds its group before a key is made, so the keys join it.
    kb->overlay.screen = screen;
    kb->overlay.escape = EscapeOverlay;
    kb->overlay.dismiss = DismissOverlay;
    kb->overlay.user = kb;
    if (router::OpenOverlay(&kb->overlay) == nullptr) {
        lv_obj_delete(screen);
        delete kb;
        return Keyboard{nullptr, nullptr};
    }
    kb->config = config;
    kb->title = config.title != nullptr ? config.title : "";
    kb->action_words = config.action != nullptr ? config.action : "Done";
    kb->config.title = kb->title.c_str();
    kb->config.action = kb->action_words.c_str();

    kb->screen = screen;
    ui::NewStatusBar(screen, kb->title.c_str());
    lv_obj_t *body = Bare(screen);
    lv_obj_set_width(body, lv_pct(100));
    lv_obj_set_flex_grow(body, 1);
    LetRingOut(body);

    // The field's place: a slot the content wide, empty. The count line under it says how much is typed, and why the action waits.
    lv_obj_t *slot = Bare(body);
    lv_obj_set_pos(slot, dt::kSpacingEdge, kFieldY);
    lv_obj_set_size(slot, dt::kSpacingContent, kFieldHeight);
    LetRingOut(slot);
    kb->count = Line(body, ui::Type::kCaption, "", true);
    lv_obj_set_pos(kb->count, dt::kSpacingEdge, kCountY);
    lv_obj_set_width(kb->count, dt::kSpacingContent);

    // The grid: six rows of keys, each key a focus stop of its own — the rows are only where the keys stand.
    lv_obj_t *grid = Bare(body);
    lv_obj_set_pos(grid, 0, kGridY);
    lv_obj_set_size(grid, kGlassWidth, kGridHeight);
    LetRingOut(grid);
    for (int r = 0; r < kKeyRows; ++r) {
        lv_obj_t *row = Bare(grid);
        LetRingOut(row);  // a key's ring reaches past its row
        kb->rows[r] = row;
    }
    BuildKeys(kb);

    // The footer: Cancel and the one action, half buttons `spacing.md` apart.
    lv_obj_t *footer = Bare(screen);
    lv_obj_set_size(footer, lv_pct(100), dt::kSpacingFooter);
    lv_obj_set_style_pad_hor(footer, dt::kSpacingEdge, 0);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(footer, dt::kSpacingMd, 0);
    LetRingOut(footer);
    constexpr int32_t kHalf = (dt::kSpacingContent - dt::kSpacingMd) / 2;
    kb->cancel = MakeButton(footer, "Cancel", ButtonVariant::kOutline, kHalf);
    kb->action = MakeButton(footer, kb->action_words.c_str(), ButtonVariant::kAction, kHalf);
    lv_obj_add_event_cb(kb->cancel, OnCancelClicked, LV_EVENT_CLICKED, kb);
    lv_obj_add_event_cb(kb->action, OnActionClicked, LV_EVENT_CLICKED, kb);
    LinkDisabledReason(kb->action, kb->count);  // the count line says why the action waits

    Refresh(kb);
    lv_obj_add_event_cb(screen, OnScreenDelete, LV_EVENT_DELETE, kb);
    s_active = kb;
    lvgl_display::SetPartialCounting(lvgl_display::PartialCounting::kTypingOnly);  // only the presses that type count toward the full refresh
    BuildWalk(kb);
    router::Focus(kb->keys[0][0]);  // opens on the first key, so typing starts at once, ringed when the buttons opened it; UP reaches the field
    return Keyboard{screen, slot};
}

void SetKeyboardTarget(const Keyboard &keyboard, const KeyboardTarget &target)
{
    KeyboardState *kb = s_active;
    if (kb != nullptr && kb->screen == keyboard.screen) {
        kb->config.target = target;
        Refresh(kb);
    }
}

void KeyboardJoinField(const Keyboard &keyboard, lv_obj_t *field, lv_obj_t *button)
{
    KeyboardState *kb = s_active;
    if (kb == nullptr || kb->screen != keyboard.screen) {
        return;
    }
    kb->field_frame = field;
    kb->field_stop = button;
    BuildWalk(kb);  // the field, its button, the keys, Cancel, the action: made again whole, so a second call changes nothing
    router::Focus(kb->keys[0][0]);  // the keyboard still opens on the first key
}

void KeyboardTextChanged(lv_obj_t *screen)
{
    KeyboardState *kb = s_active;  // nullptr once a keyboard's screen is deleted; a closed one waiting for its deletion is ignored
    if (kb != nullptr && !kb->finished && kb->screen == screen) {
        Refresh(kb);
    }
}


}  // namespace widgets
