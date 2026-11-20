#!/usr/bin/env python3
"""Resets the Sticky over its serial port and checks its boot log — the verify step's boot check.

    boot_check.py [--port PORT] [--seconds 12] [--no-reset] [--log FILE]

Resets the chip (DTR released, RTS pulsed: tests/bench/boardport.py), reads the log for --seconds and prints what it
found: the boots, the reset reason, PSRAM, the board up, the display controller and revision, the devices, touch, the
first refresh, the firmware's identity line, the console, and every error line. --no-reset only listens. Lines the
chip prints while the port is closed are lost, so the port opens before the reset. A line that never arrived is
"not seen", never a pass. Adapted from claude-agent-deck-3. Needs pyserial (ESP-IDF's Python has it).

On battery alone a reset turns the board off — the power latch lets go (STICKY_HARDWARE_SPECS §5.2): check it on USB.

Exit status: 0 clean, 1 a fault (a second boot, a panic, PSRAM not 8 MB, an error line, a watchdog or brownout reset,
a boot with no app line, no panel, a failed refresh), 2 no boot seen.
"""

import argparse
import os
import re
import sys
import time

import serial

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from boardport import add_arguments, bridge_released, find_port, open_port, reset  # noqa: E402

ANSI = re.compile(r"\x1b\[[0-9;]*m")
ROM_MARK = "ESP-ROM:"
APP_START = re.compile(r"app_shell: (\S+) (\S+), ESP-IDF (\S+)")       # app_shell's LogIdentity
BOARD_UP = "board: board up"
PANEL = re.compile(r"board_display: panel: (SSD\d+) \(([^)]*)\), ID (0x[0-9A-Fa-f]+)")
DEVICES = re.compile(r"devices: (SHT40 .*)")
TOUCH = re.compile(r"eink_input: (GT911 at 0x[0-9A-Fa-f]+|GT911 did not answer.*)")
FIRST_REFRESH = re.compile(r"eink_display: full refresh: (\d+) ms")
REFRESH_FAILED = "eink_display: refresh failed"
CONSOLE = "hw_test: console ready"
PSRAM = re.compile(r"Found (\d+)MB PSRAM device")
RESET_REASON = re.compile(r"rst:(0x[0-9a-f]+) \(([A-Z0-9_]+)\)")
NORMAL_RESETS = ("POWERON", "DEEPSLEEP_RESET", "RTC_SW_SYS_RESET", "SW_SYS_RESET")
FAULT_RESET = re.compile(r"WDT|BROWN_OUT|GLITCH")
PANIC = ("Guru Meditation", "abort() was called", "Backtrace:", "Stack canary watchpoint", "stack overflow")
ERROR_LINE = re.compile(r"^E \(\d+\)")
BENIGN_ERRORS = ("gpio_install_isr_service", "GPIO isr service already installed")


def read_lines(port, s, seconds, log):
    """Yields lines for `seconds`, reopening the port if it drops."""
    end = time.time() + seconds
    buf = b""
    try:
        while time.time() < end:
            try:
                chunk = s.read(4096)
            except (serial.SerialException, OSError):
                try:
                    s.close()
                except Exception:
                    pass
                s = open_port(port, wait_s=3)
                continue
            buf += chunk
            while b"\n" in buf:
                raw, buf = buf.split(b"\n", 1)
                line = ANSI.sub("", raw.decode(errors="replace")).rstrip("\r")
                if log:
                    log.write(line + "\n")
                yield line
    finally:
        s.close()


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    add_arguments(p)
    p.add_argument("--seconds", type=float, default=12)
    p.add_argument("--no-reset", action="store_true")
    p.add_argument("--log")
    args = p.parse_args()
    port = find_port(args.port)
    with bridge_released(args.seconds + 15):
        return check(port, args)


def check(port, args):
    s = open_port(port)
    if not args.no_reset:
        reset(s)
    log = open(args.log, "w") if args.log else None

    boots, apps, psram, board_up, console = 0, [], None, False, False
    panel, devices, touch, refresh, failed = None, None, None, None, []
    reasons, panics, errors = [], [], []
    for line in read_lines(port, s, args.seconds, log):
        if ROM_MARK in line:
            boots += 1
        if m := APP_START.search(line):
            apps.append("%s %s, ESP-IDF %s" % m.groups())
        if m := PSRAM.search(line):
            psram = int(m.group(1))
        board_up |= BOARD_UP in line
        console |= CONSOLE in line
        if m := PANEL.search(line):
            panel = "%s (%s), ID %s" % m.groups()
        if m := DEVICES.search(line):
            devices = m.group(1)
        if m := TOUCH.search(line):
            touch = m.group(1)
        if refresh is None and (m := FIRST_REFRESH.search(line)):
            refresh = int(m.group(1))
        if REFRESH_FAILED in line:
            failed.append(line)
        if m := RESET_REASON.search(line):
            reasons.append(m.group(2))
        if any(mark in line for mark in PANIC):
            panics.append(line)
        if ERROR_LINE.match(line) and not any(mark in line for mark in BENIGN_ERRORS):
            errors.append(line)

    def seen(value, fmt="%s"):
        return fmt % value if value is not None else "not seen"

    fault_resets = [r for r in reasons if FAULT_RESET.search(r)]
    absent = [part.split()[0] for part in (devices or "").split(", ") if part.endswith("absent")]

    print("boot_check: %s, %.0f s" % (port, args.seconds))
    print("  ROM boots:      %d" % boots)
    print("  reset reason:   %s" % (", ".join(r + (" (a fault)" if FAULT_RESET.search(r) else
                                                   " (a normal boot)" if r in NORMAL_RESETS else "")
                                              for r in reasons) or "not seen"))
    print("  PSRAM:          %s" % seen(psram, "%d MB"))
    print("  board up:       %s" % ("yes" if board_up else "not seen"))
    print("  panel:          %s" % seen(panel))
    print("  devices:        %s" % seen(devices) + (" — ABSENT: " + ", ".join(absent) if absent else ""))
    print("  touch:          %s" % seen(touch))
    print("  first refresh:  %s" % seen(refresh, "full, %d ms"))
    print("  firmware:       %s" % (", ".join(apps) or "not seen"))
    print("  console:        %s" % ("ready" if console else "not seen"))
    print("  panics:         %d" % len(panics))
    for line in panics:
        print("    " + line)
    print("  error lines:    %d" % len(errors))
    for line in errors + failed:
        print("    " + line)

    if not apps and boots == 0:
        return 2
    no_app = boots > 0 and not apps
    fault = (len(apps) > 1 or boots > 1 or panics or errors or failed or psram != 8 or fault_resets or no_app
             or panel is None or refresh is None)
    return 1 if fault else 0


if __name__ == "__main__":
    sys.exit(main())
