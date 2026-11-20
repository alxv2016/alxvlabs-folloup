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

---

## 1. The shape

**`main` composes, `app_shell` sequences, features register with `app_hooks`, and every arrow points at the hooks or
down at the board — never at a feature.**

| Part | Holds | May depend on |
| --- | --- | --- |
| `main/` | `app_main`: the event loop, NVS, then `app_shell::Install()`, each feature's `Install()`, `app_shell::Run()` | `app_shell`, the features listed in `app_features.def` |
| `main/app_features.def` | One `FEATURE(<component>)` line per feature — its whole wiring | — |
| `components/app_shell` | Startup order, what stops the boot, the devices and the glass, the home screen | `app_hooks`, `board`, `devices`, `lvgl_display`, `ui` |
| `components/app_hooks` | The registry: boot steps by phase and what features add to home; sealed before the first read | Nothing of the app's |
| `components/board_config` | Every pin and size, from STICKY_HARDWARE_SPECS | Nothing of the app's |
| `components/board` | The bring-up: the power latch, the buses, the panel on either controller, charging, sleep and power-off, in the spec's order | `board_config`, `epaper`, `ssd1677`, `ssd2677` |
| `components/epaper` | The glass as both controllers present it: the `Panel` interface and the SPI bus they share | ESP-IDF's drivers |
| `components/ssd1677`, `components/ssd2677` | Each display controller's driver, in Seeed's command sequences — the refreshes are [STICKY_EPAPER_WAVEFORMS](STICKY_EPAPER_WAVEFORMS.md) | `epaper` |
| `components/devices` | The SHT40, IMU, fuel gauge, RTC, buzzer, microphone and card, read as Seeed's demos do | `board`, `board_config`, `bq27220`, `pcf8563` |
| `components/lvgl_display` | LVGL on the glass: the display, its refresh policy and the SSD1677's scans, touch and the buttons | `board`, `board_config`, `design_tokens`, `epaper`, `gt911`, LVGL |
| `components/ui` | DESIGN.md's look in LVGL: the fonts, the styles and the components | `lvgl_display`, `design_tokens`, LVGL |
| `components/design_tokens` | DESIGN.md's tokens as one header (STICKY_DESIGN_TOKENS §6) | — |
| `components/gt911`, `bq27220`, `pcf8563`, `debug_logging` | Seeed's drivers for the touch controller, the fuel gauge and the clock, as Seeed ships them under their MIT licence (`LICENSE` beside each) | ESP-IDF's drivers |
| `components/<feature>` | One feature: its `Install()` and what it registers — `hw_test`, the bench screens | `app_hooks`, `board`, `board_config`, shared components |

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
   and the buttons — and home: the shell's status bar, then each feature's `OnHomeBuilt` builder, under the LVGL lock,
   drawn as a full refresh. A display that does not come up stops the boot.
4. The `kReady` steps — features start their services and tasks.
5. The `kLate` steps — the core logs the firmware's name, version and ESP-IDF version.

## 4. Adding a feature

**A feature is a component with an `Install()` in its own namespace, and one line in `main/app_features.def`; the
`esp-idf-setup` skill holds the steps.**

- **`components/hw_test` is the shape to copy**: a `CMakeLists.txt` that requires only what the feature uses, and a
  `<name>.cpp` whose `<name>::Install()` registers with `app_hooks` — here, rows on home through `OnHomeBuilt`.
- **A screen is built from `ui`'s components** — `NewScreen`, `NewStatusBar`, `NewRow`, `NewButton` — and its
  values from `design_tokens.h`, never a literal (STICKY_DESIGN_TOKENS §6).
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
