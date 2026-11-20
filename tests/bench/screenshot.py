#!/usr/bin/env python3
"""What the Sticky's glass shows, as a PNG, over the console's `shot`.

    screenshot.py [--port PORT] [-o shot.png]

The board sends the one-bit frame on the glass — the frame the last refresh drew, 800 x 480 native — as base64 lines
marked `@shot` (hw_test's console). The PNG is the glass as you hold it: portrait, 480 x 800, black and white, the
frame turned back from the controller's landscape (lvgl_display's ToNative: the screen's top edge is the frame's column
799). A grey frame is not sent: the board answers `@shot none` until a one-bit frame is drawn again. The shot takes
about six seconds at 115200 baud. The port is the one Sticky attached, or --port; the agent bridge lends it for the
shot and takes it back after (tests/bench/boardport.py). Opening the port does not reset the board. Needs pyserial
(ESP-IDF's Python has it).
"""

import argparse
import base64
import os
import sys
import time
import zlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
from boardport import add_arguments, bridge_released, find_port, open_port  # noqa: E402
from pngio import write_png  # noqa: E402

TIMEOUT_S = 30
W, H = 800, 480  # the native frame
STRIDE = W // 8


def read_shot(s):
    s.reset_input_buffer()
    s.write(b"shot\n")
    s.flush()
    data, begun, end = bytearray(), False, time.time() + TIMEOUT_S
    while time.time() < end:
        line = s.readline().decode(errors="replace").strip()
        if not line.startswith("@shot "):
            continue
        body = line[len("@shot "):]
        if body == "none":
            raise SystemExit("the glass shows no one-bit frame now (a grey frame, or nothing drawn yet)")
        if body.startswith("begin "):
            data, begun = bytearray(), True
        elif body.startswith("end "):
            if not begun:
                continue
            size, crc = body.split()[1:3]
            if len(data) != int(size) or zlib.crc32(bytes(data)) != int(crc, 16):
                raise SystemExit("the shot arrived damaged (%d of %s bytes, or its CRC) — take it again"
                                 % (len(data), size))
            return bytes(data)
        elif begun:
            data += base64.b64decode(body)
    raise SystemExit("no complete shot in %d s — is the firmware's console running? (`console_word.py help`)"
                     % TIMEOUT_S)


def to_portrait(frame):
    """Portrait (x, y) is the frame's pixel (799 - y, x): lvgl_display's ToNative, turned back."""
    rgb = bytearray(480 * 800 * 3)
    for y in range(800):
        nx = W - 1 - y
        byte, bit = nx >> 3, 0x80 >> (nx & 7)
        for x in range(480):
            if not frame[x * STRIDE + byte] & bit:  # 0 = black
                continue
            o = (y * 480 + x) * 3
            rgb[o] = rgb[o + 1] = rgb[o + 2] = 255
    return bytes(rgb)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    add_arguments(ap)
    ap.add_argument("-o", "--output", default="shot.png")
    args = ap.parse_args()
    port = find_port(args.port)
    with bridge_released(TIMEOUT_S + 10):
        s = open_port(port, wait_s=3.0, timeout=0.5)
        try:
            time.sleep(0.3)
            frame = read_shot(s)
        finally:
            s.close()
    write_png(args.output, 480, 800, to_portrait(frame))
    print("shot: %s (480 x 800, as the glass shows it)" % args.output)
    return 0


if __name__ == "__main__":
    sys.exit(main())
