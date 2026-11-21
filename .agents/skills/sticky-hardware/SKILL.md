---
name: sticky-hardware
description: Works on the firmware for the Seeed reTerminal Sticky — the ESP32-S3R8 board with a 3.97-inch 800 x 480 e-paper panel (SSD1677 on V1.0, SSD2677 on V2.0), GT911 touch, BQ27220 fuel gauge, BQ25616 charger, PCF8563 RTC, LSM6DS3TR-C IMU, SHT40, PDM microphone, buzzer, MicroSD on a shared SPI bus, and a PWR_HOLD/PWR_LOCK power latch — from docs/STICKY_HARDWARE_SPECS.md, with sticky_check.py (the spec's pins against board_config.h, the shared-bus and pin rules in C/C++, and the commands the display drivers may send). Use when writing or changing board bring-up, a driver or service for any of those parts, a pin, the e-paper refresh, controller detection, sleep, wake or power-off, battery or charging code; when supporting V1.0 and V2.0 in one image; when a symptom matches the spec's traps (ghosting, touch reporting 2048 x 2048, sensors failing after the display starts, BUSY timeouts on V2.0, battery current ten times too high, the board staying on after shutdown); or asked about a Sticky pin, part, rail or revision.
metadata:
  scope: project
---

# reTerminal Sticky hardware

How firmware meets the reTerminal Sticky: the rules that keep the board, the boot and the shared bus working, the
steps for bring-up, display, power and sleep work, and the check that keeps code and spec in step. The facts and their
reasons live in [STICKY_HARDWARE_SPECS](../../../docs/STICKY_HARDWARE_SPECS.md), cited `STICKY_HARDWARE_SPECS §n`;
this page condenses them in one-line rules and holds the steps. The doc wins if this skill and it disagree — fix the
skill. The project is generated from the same doc by the [`esp-idf-setup`](../esp-idf-setup/SKILL.md) skill, built and
flashed through [`build-run-test`](../build-run-test/SKILL.md), and every change runs through
[`verify-change`](../verify-change/SKILL.md).

## When to use

- Bringing the board up, or changing what `board::Init()` does or the order it does it in.
- Writing or changing a driver or service for the panel, touch, gauge, charger, RTC, IMU, SHT40, microphone, buzzer or
  MicroSD card.
- Choosing, moving or reading a pin; adding a peripheral.
- Any e-paper refresh work, and anything that must run on both V1.0 (SSD1677, glossy) and V2.0 (SSD2677, matte).
- Light sleep, deep sleep, wake sources, power-off, the power latch, charging and battery readings.
- A symptom listed in the spec's traps, or a new one on the board.

## Rules

### Do

1. **Take every pin from `board_config.h`, and change the spec's pin table first** — the code follows the doc, never
   the reverse (STICKY_HARDWARE_SPECS §3; ESP_IDF_SETUP §2).
2. **Ship one image for V1.0 and V2.0** — read the controller's ID at start-up (`0x70`, one byte; `0x07` is the
   SSD2677) and start that controller's driver; a failed read is an error (STICKY_HARDWARE_SPECS §1.2).
3. **Set BUSY's polarity, the reset pulse and the busy timeout from the detected controller** — they differ, and a
   wrong BUSY level times out or returns at once (STICKY_HARDWARE_SPECS §4.5).
4. **Initialise `SPI2_HOST` once at board start-up, with every unused data pin `-1`, and never free it** — a 0 hands
   GPIO0 to the bus and breaks the sensors (STICKY_HARDWARE_SPECS §1.2).
5. **Hold the shared bus lock for a whole transaction, and, with a card inserted, mount it before the panel starts and
   keep it mounted** — an SD transaction inside a refresh ghosts the panel (STICKY_HARDWARE_SPECS §1.2, §7).
6. **Send the SSD1677 the whole frame on every image refresh, the SSD2677 a partial's changed box through its partial
   window, and force a full refresh after 20 partials** — the SSD1677 drives whatever its RAM holds, the SSD2677's
   partial mode drives the box alone; the 20-partials rule is proven on the SSD1677, and no ghost was seen before it on
   the SSD2677 (STICKY_HARDWARE_SPECS §1.2, §4.2; STICKY_EPAPER_WAVEFORMS §3.1, §3.4).
7. **Hold the power latch first in bring-up, and pulse it with `gpio_set_level()` on pins configured once** — GPIO45
   and GPIO46 are strapping pins (STICKY_HARDWARE_SPECS §5.2, §7).
8. **Clear and disable the PCF8563's alarm and timer interrupts before releasing the latch** — `RTC_INTn` is
   `VDD_3V3_ENn`, and an asserted interrupt powers the board on, as the RTC wake shows (STICKY_HARDWARE_SPECS §5.2,
   §5.3).
9. **Drive `PWR_HOLD` and `PWR_LOCK` high through light sleep** — the board loses power on battery otherwise
   (STICKY_HARDWARE_SPECS §6).
10. **Hold every output through deep sleep and release the holds on wake** — Seeed's practice; ours holds `EP_PWR_EN`
    low, the panel unpowered — the glass keeps its image and the board wakes on both boards (STICKY_HARDWARE_SPECS
    §6).
11. **Reset the GT911 with the INT sync pulse, and again after light sleep** — without it the controller answers its ID
    but never scans (STICKY_HARDWARE_SPECS §7, §8.2).
12. **Divide the BQ27220's current and capacity by 10 when `DesignCapacity` reads 7500, and never reprogram the gauge**
    (STICKY_HARDWARE_SPECS §1.3).
13. **Create the sensor I2C bus only after boot, and give GPIO7 one owner** — GPIO0 is a boot strap, GPIO7 carries two
    interrupts (STICKY_HARDWARE_SPECS §1.1, §1.3).
14. **Enable GPIO11's internal pull-up for card detection** — the board fits none (STICKY_HARDWARE_SPECS §3.3).
15. **Leave the touch pins low or floating while `TP_3V3` is off** — their pull-ups would back-power the panel
    (STICKY_HARDWARE_SPECS §1.3).
16. **Run every display change on a V1.0 and a V2.0 board before handing it over** — this skill's gate, because the
    controllers share no command set and V2.0's behaviour is still open (STICKY_HARDWARE_SPECS §4.5, §10).
17. **Write what the board showed into the spec** — a new trap to its traps; a settled open item to its owning section,
    the item deleted (STICKY_HARDWARE_SPECS §8, §10; DOCUMENTATION_GUIDELINES §4.3).

### Never

- **Hard-code a GPIO number outside `board_config.h`** — this skill's rule, which `sticky_check.py code` enforces — or
  pick a pin without checking the spec, where every GPIO is taken.
- **Build per revision or choose the controller at compile time** — one image runs on both.
- **Call `spi_bus_free()` or a second `spi_bus_initialize()`** from the panel or the card.
- **Write a windowed image refresh on the SSD1677** — stale RAM outside the window lights up on the next refresh; its
  scans upload a window only because each clears the last scan's masks with it (STICKY_EPAPER_WAVEFORMS §3.3). The
  SSD2677's partial window is its own: partial mode drives the box alone (STICKY_EPAPER_WAVEFORMS §3.1).
- **Send an OTP-programming command or a voltage to either controller, or the SSD2677 a command neither Seeed's driver
  nor its partial window (`0x83`) sends** — `sticky_check.py otp` enforces it (STICKY_EPAPER_WAVEFORMS §2).
- **Write the BQ27220's data memory** — the stock profile is read, scaled, and left alone.
- **Add a pull to GPIO0, GPIO3, GPIO45 or GPIO46, or drive them before boot completes.**
- **Plan on native USB or external JTAG** — GPIO19 and GPIO20 carry the microphone, GPIO39–42 the charger and touch.
- **Claim power-off, an RTC wake or a V2.0 behaviour works from reading code** — those are open items until the
  person sees them on the board.
- **Erase the flash, write below the app or burn an eFuse without the person** — the guard asks, and an eFuse burn is
  theirs to run; flashing the app needs no yes, the person's choice.

## Procedure

### Bringing the board up

1. Generate or read `board::Init()` against the order in STICKY_HARDWARE_SPECS §7: the latch, power and the sensor bus,
   the buzzer, the card, the panel (detect, then its driver), touch, the IMU and SHT40, Wi-Fi, the microphone, the
   buttons.
2. Take each step's pins, levels and delays from STICKY_HARDWARE_SPECS §3 and §4; a value the spec lacks is a question
   for the person, written to the spec before the code.
3. `python3 .agents/skills/sticky-hardware/scripts/sticky_check.py` — 0 hits.
4. Build and flash through `build-run-test`, with the person's yes to flash; read the boot log for the detected
   controller, the GT911's resolution (480 x 800) and the sensors.

### Display work

1. Keep both drivers behind one panel interface; the detection and the per-controller values come from
   STICKY_HARDWARE_SPECS §4.5.
2. Feed the SSD2677 a waveform temperature — the SHT40's at start-up, or the controller's own (STICKY_HARDWARE_SPECS
   §4.5).
3. Run the change on a V1.0 and a V2.0 board: a full refresh, partials up to the forced full refresh, greyscale, sleep
   and wake. The person judges the glass on each; record their word with `verify.py signoff`. What V2.0 shows settles
   its open items (STICKY_HARDWARE_SPECS §10).

### Adding a peripheral or moving a pin

1. Read STICKY_HARDWARE_SPECS §1.1 and §3 — find the pin the change gives up, or plan an I/O expander.
2. Change the spec's pin table, then `board_config.h`, in the same change.
3. `sticky_check.py pins` — 0 hits.

### Sleep and power-off

1. Follow STICKY_HARDWARE_SPECS §5.3 for the order before the latch release and §6 for the sleep holds and wake
   sources.
2. Test on battery alone — USB keeps the rail up through D2, so on the bench a power-off never cuts the rail and a
   latch let go in light sleep goes unseen (STICKY_HARDWARE_SPECS §5.3, §6).
3. What the person sees settles the matching open item in STICKY_HARDWARE_SPECS §10: write the result into the owning
   section and delete the row; the `open-items-curator` agent proposes it.

### A symptom on the board

1. Find it in STICKY_HARDWARE_SPECS §8 and apply its fix.
2. A new one: find the cause, then add it to §8 as symptom → cause → fix, with its source tag, and record the lesson
   with `verify.py finding`.

## Checks

```sh
python3 .agents/skills/sticky-hardware/scripts/sticky_check.py          # pins, code and otp — 0 hits (baseline 0)
python3 .agents/skills/sticky-hardware/scripts/sticky_check.py pins     # the spec's pin table against board_config.h
python3 .agents/skills/sticky-hardware/scripts/sticky_check.py code     # the shared-bus and pin rules in C/C++
python3 .agents/skills/sticky-hardware/scripts/sticky_check.py otp      # the commands the display drivers send
python3 .agents/skills/sticky-hardware/scripts/sticky_check.py --test   # the script's own cases
```

`verify.py run` runs it when the spec, `components/`, `main/` or this skill change. Until the firmware exists, each
check says it was skipped and passes.

- [ ] (script) **Pins** — `python3 .agents/skills/sticky-hardware/scripts/sticky_check.py pins` prints `sticky pins —
  0 hits (baseline 0)`: every `| SIGNAL | GPIOn |` row under the spec's `## 3.` has its `k<Signal>` constant in
  `components/board_config/include/board_config.h` at the same GPIO, the header has no pin the spec lacks, each signal
  has one GPIO, and firmware with C/C++ has the header at all.
- [ ] (script) **Code** — `python3 .agents/skills/sticky-hardware/scripts/sticky_check.py code` prints `sticky code —
  0 hits (baseline 0)`: no `spi_bus_free()`; one `spi_bus_initialize()`; every `spi_bus_config_t` sets
  `quadwp_io_num`, `quadhd_io_num` and `data4_io_num`–`data7_io_num` to `-1`; no `GPIO_NUM_<n>` outside
  `board_config.h`.
- [ ] (script) **OTP** — `python3 .agents/skills/sticky-hardware/scripts/sticky_check.py otp` prints `sticky otp —
  0 hits (baseline 0)`: `components/ssd2677` sends only Seeed's SSD2677 commands and `0x83`, `components/ssd1677` never sends
  `0x30`, `0x36`, `0x2A`, `0x08`, `0x03`, `0x04` or `0x2C`, and `components/epaper` sends only `0x70`
  (STICKY_EPAPER_WAVEFORMS §2).
- [ ] (script) **Self-test** — `python3 .agents/skills/sticky-hardware/scripts/sticky_check.py --test` prints
  `sticky_check self-test: PASS`.
- [ ] Bring-up follows STICKY_HARDWARE_SPECS §7, and each step's values match STICKY_HARDWARE_SPECS §3 and §4.
- [ ] A display change ran on a V1.0 and a V2.0 board, and the person signed off on each — this skill's gate
  (STICKY_HARDWARE_SPECS §4.5, §10).
- [ ] A power or sleep change ran on battery alone (STICKY_HARDWARE_SPECS §5.3, §6).
- [ ] What the board showed is in the spec: a new trap in its traps, a closed item out of its open items
  (STICKY_HARDWARE_SPECS §8, §10).

## Traps

- **The sensors work until the display starts, then fail** → an unused SPI data pin was left at 0 and claimed GPIO0 →
  set it to `-1` (STICKY_HARDWARE_SPECS §8.1).
- **Every refresh times out, or returns at once, on V2.0 only** → BUSY was set for the SSD1677 → set it from the
  detected controller (STICKY_HARDWARE_SPECS §8.1).
- **The GT911 answers `911` but reports 2048 x 2048 and no touches** → the reset lacks the INT sync pulse → add it
  (STICKY_HARDWARE_SPECS §8.2).
- **The board stays on after shutdown on USB** → D2 feeds `PWR_EN` from `VIN_5V`, so the rail cannot be cut → expected:
  it enters deep sleep; test a power-off on battery alone, where whether the rail collapses is still open
  (STICKY_HARDWARE_SPECS §5.3, §8.3, §10).
- More, by where the symptom shows, in STICKY_HARDWARE_SPECS §8.

## Sources

| Topic | Where |
| --- | --- |
| What breaks the board, the boot or the bus | [STICKY_HARDWARE_SPECS §1](../../../docs/STICKY_HARDWARE_SPECS.md#1-critical-constraints) |
| The chip, memory, console, panel, battery, revisions | [STICKY_HARDWARE_SPECS §2](../../../docs/STICKY_HARDWARE_SPECS.md#2-firmware-facts) |
| Every pin, its level and its pulls | [STICKY_HARDWARE_SPECS §3](../../../docs/STICKY_HARDWARE_SPECS.md#3-pins) |
| Buses, devices, addresses, parts | [STICKY_HARDWARE_SPECS §4](../../../docs/STICKY_HARDWARE_SPECS.md#4-buses-and-devices) |
| The SSD1677 and the SSD2677 side by side | [STICKY_HARDWARE_SPECS §4.5](../../../docs/STICKY_HARDWARE_SPECS.md#45-display-controllers) |
| How each controller is refreshed: waveforms, the SSD1677's scans, presets, measured times, the panel's safety rules | [STICKY_EPAPER_WAVEFORMS](../../../docs/STICKY_EPAPER_WAVEFORMS.md) |
| Rails, the latch, power-off, charging | [STICKY_HARDWARE_SPECS §5](../../../docs/STICKY_HARDWARE_SPECS.md#5-power) |
| Sleep and wake | [STICKY_HARDWARE_SPECS §6](../../../docs/STICKY_HARDWARE_SPECS.md#6-sleep-and-wake) |
| Bring-up order | [STICKY_HARDWARE_SPECS §7](../../../docs/STICKY_HARDWARE_SPECS.md#7-bring-up-order) |
| Traps | [STICKY_HARDWARE_SPECS §8](../../../docs/STICKY_HARDWARE_SPECS.md#8-traps) |
| The enclosure and its controls | [STICKY_HARDWARE_SPECS §9](../../../docs/STICKY_HARDWARE_SPECS.md#9-enclosure-and-controls) |
| What is still unverified | [STICKY_HARDWARE_SPECS §10](../../../docs/STICKY_HARDWARE_SPECS.md#10-open-items) |
| The schematic and the block diagram | [STICKY_assets/](../../../STICKY_assets/) |
| The 3D model, kept out of git | [Seeed's download](https://files.seeedstudio.com/wiki/reterminal_sticky/res/reTerminal_Sticky_3D.stp), linked in STICKY_HARDWARE_SPECS §9 |
| Seeed's ESP-IDF demos, the reference for both controllers | [sticky-esp-idf-peripherals](https://github.com/Seeed-Projects/sticky-esp-idf-peripherals), [sticky-esp-idf-dashboard](https://github.com/Seeed-Projects/sticky-esp-idf-dashboard) |
| Setting up the ESP-IDF project from the spec | [`esp-idf-setup`](../esp-idf-setup/SKILL.md) |
| The design system on the glass, and how the firmware implements it | [DESIGN.md](../../../DESIGN.md), [STICKY_DESIGN_TOKENS](../../../docs/STICKY_DESIGN_TOKENS.md) |
