# App Architecture

How the firmware is put together: `main` composes the app, `app_shell` sequences it, features plug in through
`app_hooks`, and `board` brings the hardware up from the hardware spec ([STICKY_HARDWARE_SPECS](STICKY_HARDWARE_SPECS.md)). People read it to know where
new code goes; coding agents read it before they add a feature, a driver or a dependency. Source of truth: the code
in `main/` and `components/`; this doc says why it is shaped so.

**Stability.** Code comments cite this doc's `§n`. Do not renumber; add new material under the next free number.

| § | Section | Look here for |
| --- | --- | --- |
| 1 | [The shape](#1-the-shape) | The parts, and which may depend on which |
| 2 | [Rules](#2-rules) | Where code goes, and what never goes where |
| 3 | [Boot order](#3-boot-order) | What `app_shell::Run()` does, phase by phase |
| 4 | [Adding a feature](#4-adding-a-feature) | The steps, and what the line in `app_features.def` does |
| 5 | [Dependencies and versions](#5-dependencies-and-versions) | ESP-IDF, the managed components, the lock file |
| 6 | [Decisions](#6-decisions) | Why the shell is a component, why a feature is one line |
| 7 | [Screens, routes and focus](#7-screens-routes-and-focus) | The screen stack, a screen's lifecycle, routes, who may call the router |

---

## 1. The shape

**`main` composes, `app_shell` sequences, features register with `app_hooks`, and every arrow points at the hooks or
down at the board — never at a feature.**

| Part | Holds | May depend on |
| --- | --- | --- |
| `main/` | `app_main`: the event loop, NVS, then `app_shell::Install()`, each feature's `Install()`, `app_shell::Run()` | `app_shell`, the features listed in `app_features.def` |
| `main/app_features.def` | One `FEATURE(<component>)` line per feature — its whole wiring | — |
| `components/app_shell` | Startup order, what stops the boot, the devices and the glass, the home screen as the router's root | `app_hooks`, `board`, `devices`, `lvgl_display`, `router`, `ui` |
| `components/app_hooks` | The registry: boot steps by phase and what features add to home; sealed before the first read | Nothing of the app's |
| `components/board_config` | Every pin and size, from STICKY_HARDWARE_SPECS | Nothing of the app's |
| `components/board` | The bring-up: the power latch, the buses, the panel on either controller, charging, sleep and power-off, in the spec's order | `board_config`, `epaper`, `ssd1677`, `ssd2677` |
| `components/epaper` | The glass as both controllers present it: the `Panel` interface and the SPI bus they share | ESP-IDF's drivers |
| `components/ssd1677`, `components/ssd2677` | Each display controller's driver, in Seeed's command sequences — the refreshes are [STICKY_EPAPER_WAVEFORMS](STICKY_EPAPER_WAVEFORMS.md) | `epaper` |
| `components/devices` | The SHT40, IMU, fuel gauge, RTC, buzzer, microphone and card, read as Seeed's demos do; the buzzer's note player, a task of its own that alone drives the LEDC channel, playing DESIGN.md's six sound words by `sound::Word` (`sounds.h`) or a table in `sound_rules.h`'s format (STICKY_HARDWARE_SPECS §4.2) | `board`, `board_config`, `bq27220`, `pcf8563`, `design_tokens` — the sounds' note type is the tokens' |
| `components/lvgl_display` | LVGL on the glass: the display, its refresh policy and the SSD1677's scans, touch and the buttons; the press's tick and a widget's sound cue (`PlayCue`), each through a function the shell sets, so no widget reaches the buzzer | `board`, `board_config`, `design_tokens`, `epaper`, `gt911`, LVGL |
| `components/ui` | DESIGN.md's look in LVGL — the theme: the fonts, the styles, the focus ring, and the first components | `lvgl_display`, `design_tokens`, LVGL |
| `components/widgets` | DESIGN.md's kit as LVGL objects, one header and source per widget under `include/widgets/` and `src/`, built from `ui`'s theme as [the agent deck's](STICKY_DESIGN_TOKENS.md#9-the-ring-outside-and-the-ported-widgets) widgets are from its theme, with the deck's three status marks, and the choice card's check, at one bit under `marks/`: `MakeX(parent, …)` for a styled widget, `X MakeX(…)` with `SetX` / `UpdateX` for a composite; the select sheet, the dialog and the keyboard open as overlays on the router's stack (§7) | `ui`, `design_tokens`, `lvgl_display`, `router`, LVGL |
| `components/router` | The screen stack, the overlay stack and the routes: each screen's root and each screen's and overlay's focus group, a screen's lifecycle, Push, Pop and GoTo by object or by id, with no transitions (§7) | `lvgl_display`, `design_tokens`, LVGL, ESP-IDF's `esp_common`, `heap` and `log` |
| `components/design_tokens` | DESIGN.md's tokens as one header (STICKY_DESIGN_TOKENS §6) | — |
| `components/gt911`, `bq27220`, `pcf8563`, `debug_logging` | Seeed's drivers for the touch controller, the fuel gauge and the clock, as Seeed ships them under their MIT licence (`LICENSE` beside each) | ESP-IDF's drivers |
| `components/<feature>` | One feature: its `Install()` and what it registers — `hw_test`, the bench screens, each a route on the router's stack | `app_hooks`, `board`, `board_config`, shared components — `router` among them |

`MINIMAL_BUILD` is on: a component is built only when something requires it, so a feature that is not listed is not
built.

## 2. Rules

**Each part keeps to its job; `verify.py run` (or the `esp-idf-setup` skill's check) fails the layering rules
E1–E5.**

### Do

1. **Register, never act, in `Install()`** — boot steps and home builders with `app_hooks`; no hardware, no tasks,
   nothing drawn.
2. **Bring hardware up in `board`**, in the spec's order, and return the first error so the shell stops the boot.
3. **Take every pin and size from `board_config.h`**, and change the spec before the header.
4. **Start a feature's tasks and services in a `kReady` boot step**, when the board and the shell are up.

### Never

- **Name a feature in `app_shell` or `app_hooks`** — not in their sources, not in their `CMakeLists.txt` (E1, E2).
- **Require a component from `main` other than `app_shell`, ESP-IDF's own and the listed features** (E3).
- **Include a feature's header from outside the feature** — features meet only through `app_hooks` (E4).
- **List a feature that is not a component** (E5).
- **Put driver logic, protocol handling or a feature's loop in the shell.**

## 3. Boot order

**`app_shell::Run()` seals the hooks, then runs each phase's steps in registration order — the core's first.**

1. `app_hooks::Seal()` — a registration after this is logged and ignored.
2. `board::Init()` — the power latch first, then the shared SPI bus, both I2C buses, the charger and the buttons —
   then the `kBoardUp` steps, the core's `devices::Init()` first.
3. `board::InitDisplay()` with the SHT40's temperature for the SSD2677's waveform — `lvgl_display` reads it again
   about once a minute while the glass is idle, through the source the shell gives it; the board reads the controller's
   ID and creates the `ssd1677` or `ssd2677` driver. Then `lvgl_display::Start()` — LVGL's task, the display, touch
   and the buttons — then, under the same LVGL lock, `router::Init()` and home as the router's root: the shell's status
   bar, then each feature's `OnHomeBuilt` builder, drawn as a full refresh (§7). A display that does not come up stops
   the boot.
4. The `kReady` steps — features start their services and tasks.
5. The `kLate` steps — the core logs the firmware's name, version and ESP-IDF version.

## 4. Adding a feature

**A feature is a component with an `Install()` in its own namespace, and one line in `main/app_features.def`; the
`esp-idf-setup` skill holds the steps.**

- **`components/hw_test` is the shape to copy**: a `CMakeLists.txt` that requires only what the feature uses, and a
  `<name>.cpp` whose `<name>::Install()` registers with `app_hooks` — here, rows on home through `OnHomeBuilt`.
- **A screen is a route on the router's stack** (§7), built into the root the router makes — styled by `ui::StyleScreen`
  — from `ui`'s components, `NewStatusBar`, `NewRow`, `NewButton`, and its values from `design_tokens.h`, never a literal
  (STICKY_DESIGN_TOKENS §6). Its routes are registered in a `kBoardUp` boot step (§3, §7).
- **No header unless another component needs one** — and never another feature, which would break E4.
- **The line is the whole wiring**: `FEATURE(<name>)` makes the component a requirement of `main` and calls its
  `Install()`; deleting the line removes the feature and, under `MINIMAL_BUILD`, everything only it required (§1).

## 5. Dependencies and versions

**ESP-IDF is pinned to the v6.1.1 series; managed components are resolved once and locked.**

- **ESP-IDF** — `idf: '>=6.1.1,<6.2'` in `main/idf_component.yml`. A newer release is adopted on purpose, with
  its migration notes read, never by drifting.
- **Managed components** — declared in `main/idf_component.yml`, resolved into `dependencies.lock` (committed) and
  downloaded into `managed_components/` (not committed). Change a version with `idf.py update-dependencies`.
- **`sdkconfig.defaults`** is the source; `sdkconfig` is generated from it and not committed.
- **LVGL 9 and `espressif/esp_lvgl_port` 2** are the managed components, by major version: LVGL 9.6.0 costs the
  SSD1677's scans 1–5 ms a frame against 9.5.0, and the person keeps the newer LVGL
  ([STICKY_EPAPER_WAVEFORMS §4.2](STICKY_EPAPER_WAVEFORMS.md#42-the-v10-board-ssd1677)). The port runs LVGL's task, tick and lock
  only; the display, touch and the buttons are `lvgl_display`'s, since the port has no e-paper path.
- **Seeed's touch, gauge and clock drivers are kept as Seeed ships them** — `components/gt911`, `bq27220`, `pcf8563`
  and `debug_logging`, from Seeed's `sticky-esp-idf-peripherals` at `4b7f9a1`, with Seeed's MIT `LICENSE` beside each;
  each `CMakeLists.txt` is ours, naming ESP-IDF 6's requirements, since 6's `driver` component no longer brings
  `esp_driver_gpio` and `esp_driver_spi` with it. The display drivers are the firmware's own (`epaper`, `ssd1677`,
  `ssd2677`), written from Seeed's sequences.

## 6. Decisions

- **`app_shell` is a component, not a file in `main/`.** `main/` exports no headers and can include anything in the
  build, so a shell there has no declared dependency surface. As a component, every new dependency is written into
  its `CMakeLists.txt`, and growth shows in review.
- **A feature is one line.** The same `FEATURE(<name>)` lines are read by the preprocessor (to declare and call each
  `Install()`) and by CMake (to require the component), so wiring a feature cannot drift from building it.
- **`app_hooks` tables are fixed-size.** No allocation at registration; an overflow is logged by name, and the fix
  is to raise the size beside the table.

## 7. Screens, routes and focus

**A screen is a `router::Screen` on one stack: the router makes its root and its focus group, drives its lifecycle, and loads
it with no transition — on e-paper the refresh is the motion (DESIGN → Motion).** Home is the root; a feature pushes its
screens by route id, and Back and holding OK pop. Over the top screen stand its overlays — a select sheet, a dialog, the
keyboard — each with its own focus group. `components/router` holds it: `router.h`, `screen.h` and `route.h`.

### The lifecycle

| Call | What the router does |
| --- | --- |
| `Init()` | Once, after `lvgl_display::Start` has the display and the buttons up: holding OK becomes the router's, through `lvgl_display::SetBackHandler`, and the router's read hooks see every key and touch read (`SetKeyReadHook`, `SetTouchReadHook`) |
| `SetRoot(screen)` | Once: home, built, loaded and entered — the stack's bottom, never popped |
| `Push(screen)` | Builds the new screen (`OnCreate`), then the top one gets `OnExit`, then the new one is loaded and gets `OnEnter`; the screen under it keeps its objects and its focus |
| `Pop()` | The top gets `OnExit`, the one under it is loaded, its group bound, and gets `OnEnter`; the popped root, group and `Screen` are deleted from an `lv_async_call`, after the event that popped them |
| `PopToRoot()` | Home, in one step; only the top screen gets `OnExit` — the rest had theirs when they were covered |
| `GoTo(screen)` | Replaces every screen above home — deep sleep, power-off, a bench command; with nothing above home it is a `Push` |
| `Escape()` | Holding OK, the escape chain, in order: a control open for editing in the bound group hears `LV_EVENT_CANCEL` and editing goes off — it puts back what it had when OK opened it (`ToggleEditingOnKeyClick`'s `opened` keeps it: the range slider's value, the text view's page); else the top overlay closes as its Cancel does; else `Pop` when there is a screen above home; else nothing |

- **A screen change closes the overlays first**: `Push`, `GoTo`, `Pop` and `PopToRoot` begin with `DismissOverlays`, since an
  overlay belongs to the screen it stands on.
- **What `OnEnter` starts, `OnExit` stops** — a timer, a key observer, a benchmark's preset. A screen is exited whenever it
  stops being the top one, covered or popped, and entered again when it comes back; only the top screen runs.
- **`OnCreate` builds and starts nothing.** A factory may build a screen that a full stack then refuses.
- **The stack stops at `router::kMaxDepth`, four** — home, a screen, a sub-screen and one over it.
  LVGL allocates with the C library's `malloc` (`CONFIG_LV_USE_STDLIB_MALLOC`), whose blocks under 16 KB come from internal
  RAM (`CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL`, 16384), so screens kept under one another fill it: each push logs the internal
  heap's free bytes and its largest block, and a push past four is refused. A popped or replaced screen's objects outlive it
  until LVGL's next timer pass, so a `GoTo`, or a `PopToRoot` and a `Push` in one call (the bench's page walk), holds the old
  screens and the new one together for that moment.
- **Every root is press-locked** (`lv_obj_set_press_lock`): LVGL sets the flag on every object with a parent, and a screen has
  none, so a press that began on the background would otherwise be handed to whatever the finger slides over.

### Focus

- **Each screen has its own group**, bound to the buttons and made LVGL's default group by `lvgl_display::SetGroup` while the
  screen is built and while it is on top — so `lvgl_display::Group()` is that group, and a control made in `OnCreate` joins it.
  Each group wraps.
- **A screen opens with no ring**: after `OnCreate` the router takes `LV_STATE_FOCUS_KEY` off the control the group names, which
  a key press opening the screen would ring (DESIGN → Focus and input).
- **Focus comes back to the opener on every pop**: the covered screen's group is left as it was, so its focused control is the
  one that opened the screen above, ringed only if it was ringed then.
- **The ring is the buttons'** (DESIGN → Focus and input). The key read hook (`focus.cpp`) sees each read after `ReadKeys` has
  filled it: on a press of UP, DOWN or OK when the bound group is not editing and its focused control wears no
  `LV_STATE_FOCUS_KEY` on the glass (`lv_obj_is_visible`, read after `lv_obj_update_layout` so a screen just pushed counts as
  laid out), it rings the focused control, or the first in view when that one is off the glass, and reports the key up until
  it comes up, so nothing moves or is pressed. The tick and the key observer have run already, in `ReadKeys`; a held UP or
  DOWN steps on, since each step is a release and a fresh press. The touch read hook (`touch.cpp`): a contact that begins
  takes the ring off and leaves the place. Each records which input was given last — a button press, a hold of OK
  (`OnHold`), or a touch — which `Focus` reads when no input is being read. `tests/host/focus_rules_lvgl_test.cpp` drives
  both through LVGL's keypad and pointer.
- **Slide-off** (`touch.cpp`): the control a contact pressed is found as LVGL finds it when the contact begins; at the lift,
  if no scroll took the contact and the finger is past the control's hit area (`lv_obj_get_click_area`) and `spacing.md`,
  the control hears `LV_EVENT_PRESS_LOST` and `lv_indev_reset` makes LVGL forget the contact, so no `LV_EVENT_RELEASED` and
  no `LV_EVENT_CLICKED` reach it (WCAG 2.5.2). It holds for every `lv_button` and every focus stop — the rows, the keys —
  and for what `CancelOnSlideOff` opts in (the range slider), marked with `LV_OBJ_FLAG_USER_1`. Decided at the lift, not
  as the finger moves, so a drag a list view's swipe or the page has not yet claimed stays theirs. The root's press lock
  stays: a press keeps its control however far the finger slides.
- **A widget that moves focus by hand uses the helpers** in `router.h`, so the ring follows one rule:

  | Helper | What it does |
  | --- | --- |
  | `Focus(obj)` | Moves the place to `obj`; ringed when the buttons are the input being read, or with none being read were the last given — the sheet's chosen row, the dialog's first button, the keyboard's first key |
  | `Ring(obj)` | Moves the place and rings it whatever the input, in the bound group only |
  | `ClearRing()` | Takes the ring off the bound group's focused control and keeps the place; not while editing |
  | `SetHiddenHandingFocus(obj, hidden, next)` | Hiding the focused control, or one inside `obj`, hands the place to `next` first, ringed only if it was — the text input's cross |
  | `DeleteHandingFocus(obj, next)` | The same, then deletes `obj` |
  | `Hold(&slot, obj)` | Keeps `obj` in `slot` until LVGL deletes it, when the slot becomes nullptr; the slot outlives the object, or lets go of it first |
| `CancelOnSlideOff(obj)` | Opts a control that is neither a button nor a focus stop into slide-off (below) |

- **Holding OK is the router's alone** — `Init` sets `lvgl_display::SetBackHandler` once and nothing else sets it; what an
  overlay does on a hold is its `escape`, which the chain calls.

### Overlays

**An overlay stands over the top screen and takes the buttons while it is open; it is not a screen, so the screen under it
keeps its objects and timers and hears `OnCovered`, never `OnExit`.** The router keeps the open ones in the order they
opened, up to `router::kMaxOverlays` (four); it builds and draws nothing of them.

| Call | What the router does |
| --- | --- |
| `OpenOverlay(&overlay)` | Notes the group bound now and whether its focused control wore the ring, takes the ring off it (the group still names it), makes the overlay's group — wrapping, bound to the buttons and LVGL's default — and loads `overlay.screen` when it has one; the first overlay that covers tells the screen `OnCovered(true)`. Called before the overlay builds, so its controls join its group as they are made |
| `CloseOverlay(&overlay)` | Off the stack wherever it is; the group it opened over is bound again, the ring put back on its focused control only if it showed when the overlay opened, and the overlay's group deleted. A full-glass overlay (`overlay.screen`) loads the screen it opened over and calls `lvgl_display::RequestFull` — closing the keyboard is a full refresh (DESIGN → Motion). The last covering overlay to close tells the screen `OnCovered(false)`. Called before the overlay deletes its objects |
| `DismissOverlays()` | Each open overlay's `dismiss`, topmost first, answering nothing; one whose `dismiss` leaves it open is closed by the router |
| `OverlayOpen()` | Whether any is open |

- **One closed from under another** hands what it stood on to the one above it, so a dialog over a sheet over a page always
  closes back to the sheet, and the sheet to the page.
- **The widgets on it** are the select sheet and the dialog, built on the screen they cover behind a full-glass catcher that
  eats a touch outside them, and the keyboard, a full-glass overlay with its own screen; each holds its `router::Overlay` in
  its own state and closes it from that state's delete event too, so a screen deleted under it never leaves it on the stack.
- **A covered screen stops what it does on its own**: hw_test's Sensors and Battery and power screens pause their timer in
  `OnCovered(true)` and, uncovered, resume it and read at once (`lv_timer_ready`).

### Routes

- **A route is an id and a factory** (`route.h`): `{"hw.display", router::NoParam<DisplayScreen>}`. Ids are lowercase, unique
  and dotted under the screen they belong to (`hw.display.grays`); a factory returns nullptr to refuse a param, and
  `NoParam` refuses any.
- **A feature registers its table in a `kBoardUp` boot step** (§3), so its routes exist before home is drawn; a table with
  a taken or empty id is refused whole. `PushRoute` and `GoToRoute` answer `ESP_ERR_NOT_FOUND` for an unknown id and
  `ESP_ERR_INVALID_ARG` for a refused param, and the glass does not change.

### Who calls it

- **app_shell** calls `Init` and `SetRoot` for home; **features** push and pop their screens and register their routes;
  **widgets** open and close their overlays. The router reaches `lvgl_display` to bind a group, take holding OK and ask a full
  refresh, and never `ui` or `widgets`: a screen styles its own root, an overlay builds its own objects.
- **Never `lvgl_display`, `ui`, `board`, `devices` or `app_hooks`** — `lvgl_display` reaches the router only through the
  back handler and the read hooks it is given (`SetKeyReadHook`, `SetTouchReadHook`), and `app_hooks` carries no router
  type (E2).
- **Every call holds the LVGL lock** — from an LVGL event or timer, which hold it, or between `lvgl_display::Lock()` and
  `Unlock()` on another task, as the bench console's page walk does. The router takes no lock of its own.
