#!/usr/bin/env python3
"""Flashes the built firmware to the Sticky — the bootloader, the partition table and the app, as `idf.py flash` does.

    flash.py [--port PORT] [--build-dir build] [--baud 460800]

Writes what the build's `flash_args` lists, with esptool — the bootloader at 0x0, the partition table at 0x8000 and the
app, the images `idf.py flash` writes, which the guard allows. It never erases; an erase, or a write below the app the
person names on its own, is theirs (the guard asks). The port is the one Sticky attached, found by its CH343P, or --port; the agent bridge
lends it for the flash and takes it back after (tests/bench/boardport.py). esptool resets the board into its bootloader
and back into the app. Run with ESP-IDF's Python, which has esptool and pyserial.

Exit status: esptool's — 0 when every write was verified.
"""

import argparse
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from boardport import add_arguments, bridge_released, find_port  # noqa: E402

FLASH_S = 120  # a flash takes about 25 s at 460800 baud; the bridge's lend covers it with room to spare


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    add_arguments(ap)
    ap.add_argument("--build-dir", default="build", help="the build folder that holds flash_args")
    ap.add_argument("--baud", type=int, default=460800)
    args = ap.parse_args()
    flash_args = os.path.join(args.build_dir, "flash_args")
    if not os.path.isfile(flash_args):
        raise SystemExit(f"no {flash_args} — build first")
    port = find_port(args.port)
    command = [sys.executable, "-m", "esptool", "--chip", "esp32s3", "-p", port, "-b", str(args.baud),
               "--before", "default-reset", "--after", "hard-reset", "write-flash", "@flash_args"]
    with bridge_released(FLASH_S):
        result = subprocess.run(command, cwd=args.build_dir, capture_output=True, text=True)
    verified = result.stdout.count("Hash of data verified")
    lines = [line for line in result.stdout.splitlines() if line.startswith(("Wrote", "Hash", "Chip", "A fatal",
                                                                              "A serial"))]
    print("\n".join(lines))
    print(f"flash: {port}, {verified} images written and verified, exit {result.returncode}")
    if result.returncode != 0:
        sys.stderr.write(result.stderr[-2000:])
    return result.returncode


if __name__ == "__main__":
    sys.exit(main())
