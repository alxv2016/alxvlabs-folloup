---
name: build-run-test
description: Builds, runs, tests and screenshots this project, and takes evidence from the target — the install, the dev server or simulator, the build through the quiet wrapper (quiet.py prints the result and the errors, not the log), the tests, screenshots at the sizes that matter, and what may run on the target without asking. Use when building, starting the app, running tests, taking a screenshot, setting up a fresh clone, or when the build or the app will not start.
metadata:
  scope: project
---

# Build, run and test

How to build the FolloUp firmware, run it on a reTerminal Sticky, test it and take evidence from it. The tools and
versions are in AGENTS.md → Environment; the checks `verify.py run` runs are `CHECKS` in `.agents/project.py`. The docs
win if this skill and a doc disagree — fix the skill.

## When to use

- A fresh clone: installing ESP-IDF and fetching the managed components.
- Building the firmware, flashing a board, reading its console.
- Taking a picture of the glass, or other evidence from a board.
- The build fails, the board will not boot, or something works on one revision only.

## Rules

### Do

1. **Run long commands through the quiet wrapper** — `python3 .agents/skills/build-run-test/scripts/quiet.py --
   <command>`; it prints the result and the error lines and keeps the log in `.cache/harness/logs/`
   (AGENT_HARNESS §7.2).
2. **Run what may run without asking** — `reconfigure`, `build`, `size`, the layering and Sticky checks, and reading a
   board's console with `idf.py -p <port> monitor` when the person has the board on that port.
3. **Flash the app as the task needs, without asking** — say what is flashed, to which port, and what you will check;
   "build only" or "don't flash" from the person wins. Erasing, a write below the app and an eFuse are the person's:
   the guard asks (HARNESS §2).
4. **A request to look authorises looking only** — reading the console is not permission to flash.
5. **Report what was seen apart from what is suspected** — the console's lines, and the person's words about the
   glass.
6. **Mark a temporary debug line with `DEBUG-TEMP`**, and take it out before the hand-over — the pre-commit hook and
   `verify.py finish` refuse it.
7. **On a regression, build the last known-good commit first**, then bisect.
8. **Test on a V1.0 and a V2.0 board** when the change touches the glass, sleep or power — the controllers differ
   (STICKY_HARDWARE_SPECS §4.5).

### Never

- **Paste a full build log into the chat** — the wrapper's summary and the log's path.
- **Stop a process you did not start** — the person's serial monitor may be the one holding the board's port.
- **Install ESP-IDF yourself** — tell the person the `eim` command (the esp-idf-setup skill).

## Procedure

### A fresh clone

```sh
python3 .agents/skills/esp-idf-setup/scripts/esp_idf.py doctor   # ESP-IDF 6.1.1 present? Else: eim install -i v6.1.1
. ~/.espressif/tools/activate_idf_v6.1.1.sh                       # the line doctor prints; idf.py is a shell function
idf.py set-target esp32s3                                          # once; writes sdkconfig from sdkconfig.defaults
idf.py reconfigure                                                 # fetches LVGL and esp_lvgl_port, writes the lock
git config core.hooksPath .agents/githooks                         # the person runs this once per clone
python3 .agents/skills/verify-change/scripts/verify.py doctor
```

### Build and test

```sh
python3 .agents/skills/build-run-test/scripts/quiet.py -- python "$IDF_PATH/tools/idf.py" build
python3 .agents/skills/esp-idf-setup/scripts/esp_idf.py check       # the layering, E1–E5
python3 .agents/skills/sticky-hardware/scripts/sticky_check.py      # pins and the shared bus
bash tests/host/run.sh                                              # the host tests, a few seconds, no board
```

The host tests check the firmware's pure logic with the computer's own compiler: logic that needs no hardware goes in
a header with no ESP-IDF or LVGL include (`components/lvgl_display/include/glass_rules.h`), the firmware calls it,
and a `tests/host/<name>_test.cpp` checks it. Each test file is one program, built into `build/host/`. A
`tests/host/<name>_lvgl_test.cpp` also links LVGL's own sources, to drive LVGL's real input and scroll events (the
swipe, `components/widgets/src/internal/swipe.cpp`; `ui::FitLineBox`, `components/ui/line_box.cpp`): it needs
`managed_components/lvgl__lvgl`, which a firmware build or `idf.py reconfigure` fetches, and the first run compiles LVGL
into `build/host/lvgl` once. The firmware sources such a test exercises are named on its `// host-sources:` line: a `.cpp`
is compiled with the test, a `.c` (a font) once into `build/host/c`.

After a change to `sdkconfig.defaults`: `rm sdkconfig && idf.py reconfigure`, then build.

### Run on the target

The bench scripts in `tests/bench/` run with ESP-IDF's Python (`~/.espressif/tools/python/v6.1/venv/bin/python`, which
has pyserial and esptool). Each finds the Sticky by its CH343P (`1a86:55d3`), or takes `--port`, and borrows the port
from the agent deck's bridge for its run — never stop the bridge. The `bench-runner` agent runs these steps.

```sh
python tests/bench/flash.py                         # the build in build/, as idf.py flash writes it; no yes needed
python tests/bench/boot_check.py --log <SP>/boot.log  # resets, reads 12 s: exit 0 clean, 1 a fault, 2 no boot
python tests/bench/console_word.py help             # the firmware's console: bench, anim, pages [n], stop, speed [n], key up|down|ok|hold [n], shot
#   key: a press lasts 80 ms and never repeats; key hold closes an open edit (restoring it), then the top overlay (dialog,
#   sheet, keyboard), else goes back
#   one screen (a Display sub-screen returns to Display);
#   bench and anim replace every screen above Home; with no ring on the glass (a screen just opened, or after a touch)
#   the first key up|down|ok only shows the ring — count one extra press per fresh screen (APP_ARCHITECTURE §7)
python tests/bench/console_word.py --listen 8 key down 3   # presses a button through the real input path, to reach a screen
python tests/bench/console_word.py --listen 120 bench  # the refresh benchmark: one `animation` line a preset
python tests/bench/screenshot.py -o <SP>/home.png   # the one-bit frame on the glass, as a portrait PNG
```

The health check is the boot check's exit 0, with the panel line naming the controller — `SSD1677 (V1.0, glossy)` or
`SSD2677 (V2.0, matte)` — every device `ok`, the first full refresh, and the console ready. A person at a terminal can
use `idf.py -p /dev/cu.wchusbserial* monitor` once the bridge has lent the port. On battery alone a reset turns the
board off — the power latch lets go — so check it on USB.

Record every flash with `verify.py target --where "<V1.0 | V2.0> on <port>" --build "<commit> + working tree"
--health-exit <boot_check's exit>` — each one, not only the last: `finish` refuses two flashes of changed firmware with
no finding or decision recorded between ([`verify-change`](../verify-change/SKILL.md) Do 13).

### Screenshots

`tests/bench/screenshot.py` takes the frame the firmware sent to the glass, 480 x 800 portrait, black and white — not
the ink: ghosting, contrast and pixels left part-way are seen only on the glass, by the person or a photo. A grey frame
is not sent. To hold a state for a shot or a photo: `console_word.py anim` runs the benchmark's bar without end on the
current speed, `stop` freezes it. For a ghosting run, `console_word.py --listen 255 "pages 40"` opens each home row but
the benchmark and goes back, 3 s on each — 40 rows by default, one walk at a time; `stop` ends it — while the person
watches the glass before each full refresh.

## Checks

- [ ] (script) The build passes through `quiet.py` with no new warning in the project's own sources
- [ ] (script) `esp_idf.py check` and `sticky_check.py` report 0 hits
- [ ] (script) `bash tests/host/run.sh` exits 0 — every check of every host test passes
- [ ] No `DEBUG-TEMP` line left — `verify.py finish` shows none

## Traps

- **A run is recorded against the wrong board** → `flash.py` and `boot_check.py` take whichever Sticky is attached, and
  the two boards swap on one cable → take the board and port for `verify.py target` from that run's own output — the
  port `flash.py` printed and `boot_check.py`'s panel line (SSD1677 V1.0, SSD2677 V2.0) — never from the run before.
- **A build fails with `empty filename in #include` in `sdkconfig.h` after LVGL's version changed** → `sdkconfig` keeps
  the other version's Kconfig values → build a version experiment in its own folder with its own settings
  (`idf.py -B <folder> -D SDKCONFIG=<folder>/sdkconfig build`), and flash that folder with `flash.py --build-dir`.
- **A benchmark moves by a few milliseconds a frame with no change to the drivers** → `main/idf_component.yml` takes
  LVGL and the port by major version, and a fresh build picks the newest → compare `dependencies.lock`'s versions
  first (LVGL 9.6.0 against 9.5.0: 1–5 ms a frame, STICKY_EPAPER_WAVEFORMS §4.2).
- **`idf.py: not found` from a script or the wrapper** → `idf.py` is a shell function the activation script defines,
  not a file on `PATH` → run `python "$IDF_PATH/tools/idf.py"` instead.
- **The component manager refuses the build on ESP-IDF 6.1** → `main/idf_component.yml` pins 6.1.1 or later in the
  6.1 series → `eim install -i v6.1.1` and activate it; never loosen the pin to build. With 6.1.1 installed, 6.1's
  own Python checks fail (`esp-idf-monitor`, `esp-coredump`): compare builds within 6.1.1, by the managed components'
  versions.
- **`'CONFIG_…' was not declared`** → a Kconfig bool that is off is undefined, not 0 → test it with `#if`, never use it
  as a value.
- **A bench script cannot open the port, or reads `device reports readiness to read but returned no data`** → the agent
  deck's bridge holds it, and its plugin restarts the bridge if it is stopped → let the scripts borrow the port
  (`boardport.bridge_released`); never kill the bridge.
- **Console commands sent right after a flash do nothing** → the board is still booting → wait for the boot check's
  `console: ready`, or send `help` first and read its answer.
- **A command kept in a shell variable runs as one word under zsh** (`$W speed 3`) → zsh does not split variables → use a
  shell function or write the command out.
- **The shot is clean but the glass is not** → the shot is the frame sent, not the ink → judge the glass by eye or a
  photo (STICKY_EPAPER_WAVEFORMS §6).

## Sources

| Topic | Where |
| --- | --- |
| The loop these steps sit in | [`verify-change`](../verify-change/SKILL.md) |
| Quiet output, token use | [AGENT_HARNESS §7.2, §18](../../../docs/AGENT_HARNESS.md#72-inside-a-skill) |
| What asks the person | [HARNESS §2](../../HARNESS.md#2-the-guard) |
| ESP-IDF's install and versions | [`esp-idf-setup`](../esp-idf-setup/SKILL.md) |
| The board's bring-up and traps | [`sticky-hardware`](../sticky-hardware/SKILL.md) |
