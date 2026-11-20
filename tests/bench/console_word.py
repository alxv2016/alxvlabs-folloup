#!/usr/bin/env python3
"""Sends one console command to the Sticky and prints what it answers.

    console_word.py [--port PORT] [--listen 2] help
    console_word.py --listen 70 bench

The command goes out at the console's 115200 baud; then, for --listen seconds, every line the board prints is shown —
the answer, an error, a log line — except a `shot`'s data (tests/bench/screenshot.py takes those). The commands are
hw_test's (`help` lists them). The port is the one Sticky attached, found by its CH343P, or --port; the agent bridge
lends it for the command and takes it back after (tests/bench/boardport.py). Opening the port does not reset the
board. Adapted from claude-agent-deck-3. Needs pyserial (ESP-IDF's Python has it).
"""

import argparse
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from boardport import add_arguments, bridge_released, find_port, open_port  # noqa: E402


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    add_arguments(ap)
    ap.add_argument("--listen", type=float, default=2.0, help="seconds to print the board's lines after the command")
    ap.add_argument("word", nargs="+", help="the command and its arguments, as typed on the console")
    args = ap.parse_args()
    port = find_port(args.port)
    with bridge_released(args.listen + 10):
        s = open_port(port, wait_s=3.0, timeout=0.2)
        try:
            time.sleep(0.3)
            s.reset_input_buffer()
            s.write((" ".join(args.word) + "\n").encode())
            s.flush()
            end = time.time() + args.listen
            while time.time() < end:
                line = s.readline().decode(errors="replace").rstrip()
                if line and not line.startswith("@shot "):
                    print(line[:220])
        finally:
            s.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
