"""The Sticky's serial port: found by its USB bridge, lent by the agent bridge, opened without resetting the chip
(shared by the bench scripts). Adapted from claude-agent-deck-3's tests/bench/boardport.py.

The reTerminal Sticky's USB-C reaches the ESP32-S3 only through a WCH CH343P, as UART0 — USB 0x1A86:0x55D3, on macOS
/dev/cu.wchusbserial* (STICKY_HARDWARE_SPECS §1, §3.5). Its DTR and RTS drive EN and GPIO0 through the auto-download
transistors Q3 and Q4: the lines are active low, and RTS asserted (pyserial `rts = True`) with DTR released resets the
chip. pyserial sets DTR on open, so RTS is released before the open and DTR after it — both released, equal, which
touches neither.

The agent deck's bridge, when it runs on this Mac, holds any WCH port it finds — the Sticky's too. `bridge_released()`
asks it to lend the port for a script's run (`POST 127.0.0.1:8737/release {"seconds": n}`) and gives it back after
(`{"seconds": 0}`); no bridge running is no error. Never stop the bridge: it is the person's, and its plugin starts it
again.
"""

import contextlib
import fnmatch
import glob
import json
import socket
import sys
import time
import urllib.error
import urllib.request

CONSOLE_BAUD = 115200  # CONFIG_ESP_CONSOLE_UART_BAUDRATE
USB_IDS = ((0x1A86, 0x55D3),)  # the CH343P
GLOBS = ("/dev/cu.wchusbserial*", "/dev/cu.usbmodem*", "/dev/ttyACM*", "/dev/ttyUSB*")

BRIDGE_HOST, BRIDGE_PORT = "127.0.0.1", 8737
BRIDGE_RELEASE = "http://%s:%d/release" % (BRIDGE_HOST, BRIDGE_PORT)
MAX_RELEASE_S = 600  # the bridge's own cap on one release


def attached():
    """Every Sticky attached, by its USB bridge where pyserial can list ports, else by the port's name."""
    try:
        from serial.tools import list_ports
    except ImportError:
        return sorted(glob.glob(GLOBS[0]))
    return sorted(p.device for p in list_ports.comports()
                  if (p.vid, p.pid) in USB_IDS and any(fnmatch.fnmatch(p.device, g) for g in GLOBS))


def find_port(port=None):
    """The port given, or the one Sticky attached; none or several stop with what was found — never a guess."""
    if port:
        return port
    found = attached()
    if len(found) == 1:
        return found[0]
    if not found:
        raise SystemExit("no reTerminal Sticky attached (CH343P, 1a86:55d3) — plug it in with a data cable, or pass "
                         "--port")
    raise SystemExit("several Stickies attached (%s) — pass --port" % ", ".join(found))


def bridge_listening(timeout=0.5):
    try:
        with socket.create_connection((BRIDGE_HOST, BRIDGE_PORT), timeout=timeout):
            return True
    except OSError:
        return False


def release_bridge(seconds):
    """Asks the bridge to lend the port for `seconds` (0 gives it back). Returns the seconds lent, 0 when it holds no
    port, or None when no bridge answered."""
    body = json.dumps({"seconds": max(0, min(int(seconds), MAX_RELEASE_S))}).encode()
    req = urllib.request.Request(BRIDGE_RELEASE, data=body, method="POST", headers={"Content-Type": "application/json"})
    opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))  # 127.0.0.1 never goes through a proxy
    try:
        with opener.open(req, timeout=3) as r:
            return float(json.loads(r.read() or b"{}").get("released_s", 0))
    except (urllib.error.URLError, OSError, ValueError):
        return None


@contextlib.contextmanager
def bridge_released(seconds, log=sys.stderr):
    """The port lent by the bridge for at most `seconds` while the block runs, given back after."""
    lent = release_bridge(seconds)
    if lent:
        print("bridge: lent the port for up to %.0f s" % lent, file=log)
        time.sleep(0.3)
    try:
        yield lent
    finally:
        if lent:
            release_bridge(0)
            print("bridge: port given back", file=log)


def open_port(port, wait_s=0.0, timeout=0.1, baud=CONSOLE_BAUD):
    import serial

    end = time.time() + wait_s
    while True:
        try:
            s = serial.Serial()
            s.port, s.baudrate, s.timeout = port, baud, timeout
            s.rts = False
            s.open()
            s.dtr = False
            return s
        except (serial.SerialException, OSError):
            if time.time() >= end:
                raise
            time.sleep(0.05)


def reset(s):
    """ESP32-S3 TRM "reset into booting from flash": DTR 0, RTS 0, RTS 1 (EN low), RTS 0. On battery alone the reset
    also releases the power latch, and the board turns off (STICKY_HARDWARE_SPECS §5.2): reset it on USB."""
    s.dtr = False
    s.rts = False
    s.rts = True
    time.sleep(0.1)
    s.rts = False


def add_arguments(parser):
    parser.add_argument("--port", help="the Sticky's port (default: the one attached, found by its CH343P)")
