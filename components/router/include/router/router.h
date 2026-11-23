// The screen stack and the overlay stack: what is on the glass, what is under it, what stands over it, and the lifecycle
// calls between them (APP_ARCHITECTURE §7).
//
// The router knows no screen. It holds Screens, drives OnCreate, OnEnter and OnExit, binds each screen's focus group to the
// buttons through lvgl_display, and loads the screen with no transition — on e-paper the refresh is the motion (DESIGN →
// Motion). "Home" is the root, the screen SetRoot set; the router knows it only as the bottom of the stack.
//
// THE LVGL LOCK. Every call here expects it held: from an LVGL event or timer, which already hold it, or from another task
// between lvgl_display::Lock() and Unlock(). Never from an ISR.
//
// WHO CALLS IT: app_shell (Init, SetRoot for home), features (their routes and screens). Never lvgl_display, ui, board,
// devices or app_hooks: lvgl_display reaches the router only through the back handler and the read hooks it is given.

#pragma once

#include <cstddef>
#include <memory>

#include "esp_err.h"
#include "router/route.h"
#include "router/screen.h"
#include "router/status_rules.h"

namespace router {

// The deepest the stack goes: home, a screen, a sub-screen and one over that. A Push past it is
// refused, so the screens' objects, which live in internal RAM, never pile up (each push logs the internal heap). A popped
// or replaced screen's objects outlive it until LVGL's next timer pass, when they are deleted, so a GoTo, or a PopToRoot
// and a Push in one call, holds the old screens and the new one together for that moment.
inline constexpr size_t kMaxDepth = 4;

// Once, after lvgl_display::Start has the display and the buttons up, under the LVGL lock: the router takes holding OK —
// it becomes lvgl_display's back handler, running Escape.
esp_err_t Init();

// The root, once, after Init: built, loaded and entered. ESP_ERR_INVALID_STATE before Init or when a root is set.
esp_err_t SetRoot(std::unique_ptr<Screen> screen);

// Builds `screen`, covers the top one — it gets OnExit and keeps its objects and its focus — and makes the new one the top.
// ESP_ERR_INVALID_STATE with no root or at kMaxDepth, ESP_ERR_INVALID_ARG for nullptr, ESP_ERR_NO_MEM when LVGL could not
// make its root or group; nothing on the glass changes then.
esp_err_t Push(std::unique_ptr<Screen> screen);

// Replaces every screen above the root with `screen` — for what leaves the stack behind: deep sleep, power-off, a bench
// command. With nothing above the root it is a Push.
esp_err_t GoTo(std::unique_ptr<Screen> screen);

// Back one: the top screen gets OnExit, the one under it is loaded, its group bound with focus where it was — on the control
// that opened the popped screen, ringed only if it was ringed then — and it gets OnEnter. The popped screen is deleted after
// this call returns. Nothing at the root.
void Pop();

// Home: every screen above the root goes at once; only the top one gets OnExit, the rest had theirs when covered. Nothing at
// the root.
void PopToRoot();

// The screens on the stack, the root included; 0 before SetRoot.
size_t Depth();

// Holding OK, the escape chain (DESIGN → Focus and input): a control open for editing hears LV_EVENT_CANCEL and closes, putting
// back what it had when OK opened it; else the top overlay's escape when an overlay is open; else the top screen's own escape, when
// it attached one (AttachEscape); else back one screen when there is one to go back to — true each time — and at home with nothing
// open and no escape attached, nothing — false.
bool Escape();

// From a screen's OnCreate only (widgets::MakeCarousel): what holding OK does on this screen when nothing is open for editing and no
// overlay is up, in place of going back — a screen whose leaving is its own, which must work at any depth, the root included.
// `escape(user)` runs under the lock, from the hold; `user` must live as long as the screen's objects. Ignored outside OnCreate.
void AttachEscape(void (*escape)(void *user), void *user);

// --- Overlays --------------------------------------------------------------------------------------------------------
//
// An overlay stands over the top screen and takes the buttons while it is up: the select sheet, a dialog, the keyboard. It is
// not a screen: the screen under it keeps its objects and its timers and gets OnCovered, not OnExit. The router builds and
// draws nothing of an overlay; it keeps the ones that are open, in the order they opened, gives each its own focus group,
// and answers from that one list who has the buttons, whether the screen is covered, and what holding OK closes.
//
// THE RING GOES WITH FOCUS BOTH WAYS. Opening takes the ring off the control focused under the overlay — the group still
// names it — and closing puts it back only if it was showing when the overlay opened (DESIGN → Focus and input). An overlay
// does neither itself.
//
// The Overlay is the overlay's own and stays alive while it is open: a member of whatever owns it, closed before that is
// freed.

// How many overlays stand open at once: a sheet, a dialog over it, and room for one more.
inline constexpr size_t kMaxOverlays = 4;

struct Overlay {
    // A full-glass overlay's own screen object — the keyboard — which opening loads and closing replaces with the screen it
    // opened over, drawn as a full refresh (DESIGN → Motion). nullptr for one built on the screen it covers: a sheet, a dialog.
    lv_obj_t *screen = nullptr;
    // True when the overlay counts as covering the screen for Screen::OnCovered — the select sheet, a dialog and the keyboard
    // all do, though a sheet or a dialog leaves part of the page in view — so the screen may pause what it does on its own.
    bool covers_screen = true;
    // Holding OK: closes the overlay as its Cancel does, answering as Cancel answers. Must end in CloseOverlay. nullptr:
    // `dismiss` is used.
    void (*escape)(void *user) = nullptr;
    // Closes it at once, answering nothing — for what replaces the glass (DismissOverlays). Must end in CloseOverlay.
    // nullptr: the router closes it from the stack itself.
    void (*dismiss)(void *user) = nullptr;
    void *user = nullptr;

    // The router's own, set by OpenOverlay: the overlay's focus group, the group and screen it opened over, and whether the
    // ring showed there.
    lv_group_t *group = nullptr;
    lv_group_t *beneath = nullptr;
    lv_obj_t *beneath_screen = nullptr;
    bool ring_showed = false;
};

// The overlay has opened: it goes on top, with a new group bound to the buttons and made LVGL's default group — call it
// BEFORE building the overlay's controls, which then join the group as they are made — and its screen loaded when it has
// one. Returns the group; nullptr when kMaxOverlays are open or there is no memory, and nothing changes.
lv_group_t *OpenOverlay(Overlay *overlay);

// The overlay has closed: it comes off the stack wherever it is, its group is deleted, the group under it is bound with the
// ring back if it showed, and a full-glass overlay's screen gives way to the one it opened over, drawn full. Call it before
// deleting the overlay's objects. A no-op for an overlay that is not open.
void CloseOverlay(Overlay *overlay);

// Closes every open overlay, topmost first, through each one's `dismiss`. Push, GoTo, Pop and PopToRoot call it first:
// an overlay belongs to the screen it stands on.
void DismissOverlays();

// Whether any overlay is open.
bool OverlayOpen();

// --- Focus -----------------------------------------------------------------------------------------------------------
//
// THE RING IS THE BUTTONS' (DESIGN → Focus and input). With no ring on the glass, the first press of UP, DOWN or OK shows it —
// on the focused control, or the first in view when that is off the glass — and moves and presses nothing; a touch shows no
// ring and drops the one showing, and the place stays. The router does both from the read hooks Init installs. A widget that
// moves focus by hand goes through these, so the ring follows the same rule everywhere. All under the LVGL lock.

// Focuses `obj` in its group; ringed when the buttons are the input being read — or, with none being read, were the last
// input given — and not when a finger is. A no-op for an object in no group.
void Focus(lv_obj_t *obj);
// Focuses `obj` and rings it whichever input is acting: a ring the screen places for the person. A no-op unless `obj`'s group
// is the one bound to the buttons, or while `obj` is hidden.
void Ring(lv_obj_t *obj);
// Takes the ring off the focused control of the bound group, which goes on naming it; not while a control is open for editing.
void ClearRing();
// Hides or shows `obj`. Hiding it when the focused control is `obj` or inside it hands focus to `next` first — ringed only if
// the focused control was — or, with no `next`, takes the ring and the focus state off it, its group still naming it. Showing
// moves nothing.
void SetHiddenHandingFocus(lv_obj_t *obj, bool hidden, lv_obj_t *next);
// Deletes `obj`, handing focus to `next` first as SetHiddenHandingFocus does when the focused control is `obj` or inside it.
void DeleteHandingFocus(lv_obj_t *obj, lv_obj_t *next);
// Keeps `obj` in `*slot` until LVGL deletes it, when `*slot` becomes nullptr — for a pointer kept across events. Holding another,
// or nullptr, lets go of the last. A slot is written only through Hold and stays at one address, and must outlive the object it
// holds — or let go of it first (Hold(slot, nullptr)): the object's deletion writes the slot.
void Hold(lv_obj_t **slot, lv_obj_t *obj);

// SLIDE-OFF: a finger that lifts off a pressed control, beyond its hit area and `spacing.md`, presses nothing — the control
// hears LV_EVENT_PRESS_LOST and LVGL forgets the contact (WCAG 2.5.2; DESIGN → Focus and input). The rule holds for every
// button and every focus stop (the rows, the keys); this opts in another — the range slider, whose lift sets its value. A
// contact a scroll has taken is the scroll's, and never cancelled here.
void CancelOnSlideOff(lv_obj_t *obj);

// --- The status line ------------------------------------------------------------------------------------------------
//
// News that asks nothing, as plain words in the band a screen reserves under its title (DESIGN → Components → The kit). The
// router keeps the state — which line is up, which is waiting — and draws it into the band of the screen on top, which
// widgets::MakeStatusLine built and handed to AttachStatusLine from OnCreate. A screen with no band has no news: Say there says
// nothing, logs it for the developer, and returns 0 — a line never seen sounds nothing either. All under the LVGL lock.
//
// The line leaves with the screen: Push, Pop, GoTo and PopToRoot clear it, and nothing else does but a replacement or its
// caller's Clear — never a timer. While an overlay is open the band holds still; news said then is drawn, and sounded, as the
// last overlay closes. The rules are status_rules.h's, and tested on the computer.

// Says `words` as news of `kind`, replacing the line showing; the sound of success and error plays as the words are set
// (lvgl_display::Cue). The same words and kind again draw nothing and keep the id, but sound their kind again: a press hears
// its outcome. The words are copied, and cut at kStatusBytes. Returns the line's id, for Clear; 0 when nothing was said: no band,
// no words, or no screen entered — Say from OnEnter or later, never from OnCreate (the screen is not on top yet) or OnExit (it
// has left).
StatusId Say(StatusKind kind, const char *words);
// Retracts the line `id` named, if it is still the one wanted: a stale id, a line replaced since, or one said on another screen
// does nothing. The band goes white in one partial refresh (or when the last overlay closes).
void Clear(StatusId id);
// From a screen's OnCreate only (widgets::MakeStatusLine): `words` is the band's label and `rule` the divider that shows only
// while news does — both the screen's own objects. Not hidden by the router otherwise; ignored outside OnCreate.
void AttachStatusLine(lv_obj_t *words, lv_obj_t *rule);

// --- Routes (route.h) ------------------------------------------------------------------------------------------------

// Makes a table's routes reachable by id. An id already taken, an entry with no id or no factory, rejects the whole table,
// logged: ESP_ERR_INVALID_ARG. ESP_ERR_NO_MEM when the registry is full. Needs no Init: a kBoardUp step calls it.
esp_err_t AddRoutes(const RouteTable &table);
// The route registered under `id`, or nullptr.
const Route *FindRoute(const char *id);
// Push and GoTo by id: the route's factory builds the screen for `param`. ESP_ERR_NOT_FOUND for an id nobody registered,
// ESP_ERR_INVALID_ARG when the factory refused `param`, both logged; else as Push and GoTo.
esp_err_t PushRoute(const char *id, const char *param = nullptr);
esp_err_t GoToRoute(const char *id, const char *param = nullptr);

}  // namespace router
