#!/usr/bin/env python3
"""The reTerminal Sticky's hardware rules that code can show (STICKY_HARDWARE_SPECS §1, §3; STICKY_EPAPER_WAVEFORMS §2).

    python3 .agents/skills/sticky-hardware/scripts/sticky_check.py          # pins, code and otp
    python3 .agents/skills/sticky-hardware/scripts/sticky_check.py pins     # the spec's pins against board_config.h
    python3 .agents/skills/sticky-hardware/scripts/sticky_check.py code     # the shared-bus and pin rules in C/C++
    python3 .agents/skills/sticky-hardware/scripts/sticky_check.py otp      # the display drivers' commands
    python3 .agents/skills/sticky-hardware/scripts/sticky_check.py --test   # the script's own cases

Run from the repo root. Prints each check's name and hit count, then one line per hit; exits 1 on any hit and 2 on an
unknown check name. With no firmware yet (no C/C++ under components/ or main/), each check says it was skipped and
passes. Comments are blanked before matching, string literals kept; a macro that builds a pin or a bus config out of
sight of the source text is not seen.
"""
from __future__ import annotations

import contextlib
import io
import re
import sys
import tempfile
from pathlib import Path

SPEC = Path("docs/STICKY_HARDWARE_SPECS.md")
BOARD_CONFIG = Path("components/board_config/include/board_config.h")
CODE_DIRS = (Path("components"), Path("main"))
SKIP_DIRS = {"managed_components", "build", ".git"}
CODE_EXT = {".c", ".cc", ".cpp", ".h", ".hh", ".hpp"}
CHECK_NAMES = ("pins", "code", "otp")

# The display drivers' commands (STICKY_EPAPER_WAVEFORMS §2), read where they are sent: `Command(0xNN` and
# `Write(0xNN`. The SSD2677 sends only Seeed's commands and its partial window (0x83, PTLW) — its undocumented opcodes
# include OTP programming (0x90 enters it, 0x91 burns the OTP); the SSD1677 never sends its OTP-programming commands
# nor the voltages; the shared bus code sends only the ID probe.
SENT_COMMAND = re.compile(r"\b(?:Command|Write)\s*\(\s*(0[xX][0-9A-Fa-f]{1,2})\b")
SSD2677_ALLOWED = {0x00, 0x02, 0x04, 0x06, 0x07, 0x10, 0x12, 0x30, 0x40, 0x50, 0x61, 0x62, 0x65, 0x70, 0x83, 0xA5, 0xE0,
                   0xE6, 0xE7, 0xE9}
SSD1677_DENIED = {0x30: "programs the waveform OTP", 0x36: "programs the OTP selection", 0x2A: "programs VCOM into OTP",
                  0x08: "programs the initial code into OTP", 0x03: "writes the gate voltage",
                  0x04: "writes the source voltages", 0x2C: "writes VCOM"}
EPAPER_ALLOWED = {0x70}

PIN_ROW = re.compile(r"^\|\s*`?([A-Z][A-Z0-9_]*)`?\s*\|\s*([^|]*?)\s*\|")
ONE_GPIO = re.compile(r"^GPIO(\d{1,2})$")
CONST = re.compile(r"constexpr\s+gpio_num_t\s+(k[A-Za-z0-9]+)\s*=\s*GPIO_NUM_(\d{1,2})\b")
SPI_FREE = re.compile(r"\bspi_bus_free\s*\(")
SPI_INIT = re.compile(r"\bspi_bus_initialize\s*\(")
# A declared variable — `spi_bus_config_t bus = {…};`, `bus{…};`, `bus;` — not a pointer, a reference or a parameter.
SPI_CONFIG = re.compile(r"\bspi_bus_config_t\s+([A-Za-z_]\w*)\s*(=|\{|;)")
UNUSED_PINS = ("quadwp_io_num", "quadhd_io_num", "data4_io_num", "data5_io_num", "data6_io_num", "data7_io_num")
UNUSED_VALUE = r"(?:-\s*1\b|[^;,}]*\bGPIO_NUM_NC\b[^;,}]*)"  # -1, - 1, GPIO_NUM_NC and its casts
LITERAL_GPIO = re.compile(r"\bGPIO_NUM_(\d{1,2})\b")
# Strings first, so a `//` inside one is kept; comments blanked to spaces, their newlines kept for line numbers.
CODE_TOKENS = re.compile(r'"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'|//[^\n]*|/\*.*?\*/', re.S)


def const_name(signal: str) -> str:
    """The constant esp-idf-setup generates for a signal: `EP_SCK` → `kEpSck`."""
    return "k" + "".join(w.capitalize() for w in signal.lower().split("_"))


def strip_comments(text: str) -> str:
    def blank(m: re.Match) -> str:
        s = m.group(0)
        return s if s[0] in "\"'" else re.sub(r"[^\n]", " ", s)
    return CODE_TOKENS.sub(blank, text)


def line_of(text: str, pos: int) -> int:
    return text.count("\n", 0, pos) + 1


def spec_pins(text: str) -> tuple[dict[str, int], list[str]]:
    """`| SIGNAL | GPIOn |` rows under `## 3.` (STICKY_HARDWARE_SPECS §3), and the rows that break that shape."""
    start = re.search(r"^## 3\.", text, re.M)
    if not start:
        return {}, []
    end = re.search(r"^## 4\.", text[start.end():], re.M)
    section = text[start.end(): start.end() + end.start() if end else len(text)]
    pins: dict[str, int] = {}
    problems = []
    for line in section.splitlines():
        m = PIN_ROW.match(line.strip())
        if not m or not m.group(2).upper().startswith("GPIO"):
            continue  # a header, a separator or a row with no GPIO cell
        signal, cell = m.group(1), m.group(2).strip("` ")
        g = ONE_GPIO.match(cell)
        if not g:
            problems.append(f"{signal}: `{cell}` is not one GPIO — write it as `GPIO<n>` in {SPEC}")
        elif signal in pins and pins[signal] != int(g.group(1)):
            problems.append(f"{signal}: GPIO{pins[signal]} and GPIO{g.group(1)} in {SPEC} — one GPIO per signal")
        else:
            pins.setdefault(signal, int(g.group(1)))
    return pins, problems


def header_pins(text: str) -> dict[str, int]:
    return {m.group(1): int(m.group(2)) for m in CONST.finditer(strip_comments(text))}


def compare_pins(spec: dict[str, int], header: dict[str, int]) -> list[str]:
    hits = []
    wanted = {const_name(s): (s, g) for s, g in spec.items()}
    for const, (signal, gpio) in sorted(wanted.items()):
        if const not in header:
            hits.append(f"{signal}: no `{const}` in {BOARD_CONFIG} — STICKY_HARDWARE_SPECS §3 has GPIO{gpio}")
        elif header[const] != gpio:
            hits.append(f"{signal}: `{const}` is GPIO{header[const]} in {BOARD_CONFIG}, GPIO{gpio} in "
                        f"STICKY_HARDWARE_SPECS §3 — the spec changes first")
    for const, gpio in sorted(header.items()):
        if const not in wanted:
            hits.append(f"`{const}` = GPIO{gpio} in {BOARD_CONFIG} is not in STICKY_HARDWARE_SPECS §3 — add it to "
                        f"the spec first")
    return hits


def brace_block(text: str, pos: int) -> str:
    """The `{…}` starting at or after `pos` up to the next `;`, braces balanced; empty when there is none."""
    open_at = text.find("{", pos)
    semi = text.find(";", pos)
    if open_at < 0 or (0 <= semi < open_at):
        return ""
    depth = 0
    for i in range(open_at, len(text)):
        depth += {"{": 1, "}": -1}.get(text[i], 0)
        if depth == 0:
            return text[open_at: i + 1]
    return text[open_at:]


def unset_pins(text: str, name: str, decl_end: int, opener: str) -> list[str]:
    """The unused data pins a `spi_bus_config_t` named `name` leaves unset: neither `.field = -1` in its initialiser
    nor `name.field = -1` (or `{-1}`) anywhere in the file."""
    init = brace_block(text, decl_end - 1) if opener in "={" else ""
    missing = []
    for field in UNUSED_PINS:
        designated = re.search(rf"\.{field}\s*=\s*{UNUSED_VALUE}", init)
        assigned = re.search(rf"\b{re.escape(name)}\s*\.\s*{field}\s*(?:=\s*{UNUSED_VALUE}|\{{\s*{UNUSED_VALUE}\s*\}})",
                             text)
        if not (designated or assigned):
            missing.append(field)
    return missing


def code_hits(files: dict[str, str]) -> list[str]:
    """The shared-bus and pin rules (STICKY_HARDWARE_SPECS §1.2, §3) in C/C++ sources: path → text."""
    hits = []
    inits = []
    for path, raw in sorted(files.items()):
        text = strip_comments(raw)
        for m in SPI_FREE.finditer(text):
            hits.append(f"{path}:{line_of(text, m.start())}: `spi_bus_free()` — the shared SPI bus is never freed "
                        f"(STICKY_HARDWARE_SPECS §1.2)")
        inits += [f"{path}:{line_of(text, m.start())}" for m in SPI_INIT.finditer(text)]
        for m in SPI_CONFIG.finditer(text):
            missing = unset_pins(text, m.group(1), m.end(), m.group(2))
            if missing:
                hits.append(f"{path}:{line_of(text, m.start())}: `spi_bus_config_t {m.group(1)}` leaves "
                            f"{', '.join(f'`{f}`' for f in missing)} unset — set each to -1, as a 0 hands GPIO0 to "
                            f"the SPI bus (STICKY_HARDWARE_SPECS §1.2)")
        if path != str(BOARD_CONFIG):
            for m in LITERAL_GPIO.finditer(text):
                hits.append(f"{path}:{line_of(text, m.start())}: `GPIO_NUM_{m.group(1)}` — take every pin from "
                            f"board_config.h (STICKY_HARDWARE_SPECS §3)")
    if len(inits) > 1:
        hits.append(f"`spi_bus_initialize()` called {len(inits)} times ({', '.join(inits)}) — once, at board "
                    f"start-up (STICKY_HARDWARE_SPECS §1.2)")
    return hits


def otp_hits(files: dict[str, str]) -> list[str]:
    """Commands the display drivers must never send (STICKY_EPAPER_WAVEFORMS §2): path → text."""
    hits = []
    for path, raw in sorted(files.items()):
        parts = Path(path).parts
        if "ssd2677" in parts:
            def bad(op: int) -> str | None:
                return None if op in SSD2677_ALLOWED else "is not one of Seeed's SSD2677 commands or 0x83 — its " \
                    "undocumented opcodes include OTP programming"
        elif "ssd1677" in parts:
            def bad(op: int) -> str | None:
                return f"{SSD1677_DENIED[op]}" if op in SSD1677_DENIED else None
        elif "epaper" in parts:
            def bad(op: int) -> str | None:
                return None if op in EPAPER_ALLOWED else "the shared bus code sends only the ID probe (0x70)"
        else:
            continue
        text = strip_comments(raw)
        for m in SENT_COMMAND.finditer(text):
            why = bad(int(m.group(1), 16))
            if why:
                hits.append(f"{path}:{line_of(text, m.start())}: command `{m.group(1)}` {why} "
                            f"(STICKY_EPAPER_WAVEFORMS §2)")
    return hits


def read_code(root: Path) -> dict[str, str]:
    files = {}
    for base in CODE_DIRS:
        top = root / base
        if not top.is_dir():
            continue
        for f in top.rglob("*"):
            if f.suffix in CODE_EXT and f.is_file() and not SKIP_DIRS & set(f.relative_to(root).parts):
                files[f.relative_to(root).as_posix()] = f.read_text(errors="replace")
    return files


def report(name: str, hits: list[str], skipped: str | None = None) -> bool:
    if skipped:
        print(f"{name} — skipped: {skipped}")
        return True
    print(f"{name} — {len(hits)} hits (baseline 0)")
    for h in hits:
        print(f"    {h}")
    return not hits


def check_pins(root: Path, code: dict[str, str]) -> bool:
    spec = root / SPEC
    if not spec.is_file():
        return report("sticky pins", [f"{SPEC} is missing"])
    pins, problems = spec_pins(spec.read_text(errors="replace"))
    if not pins:
        return report("sticky pins", problems + [f"no `| SIGNAL | GPIOn |` rows under ## 3. in {SPEC}"])
    header = root / BOARD_CONFIG
    if not header.is_file():
        if not code:
            return report("sticky pins", problems, None if problems else
                          f"no firmware yet — {len(pins)} pins read from STICKY_HARDWARE_SPECS §3")
        return report("sticky pins", problems + [f"the firmware has no {BOARD_CONFIG} — every pin lives there "
                                                 f"(STICKY_HARDWARE_SPECS §3)"])
    return report("sticky pins", problems + compare_pins(pins, header_pins(header.read_text(errors="replace"))))


def check_code(code: dict[str, str]) -> bool:
    if not code:
        return report("sticky code", [], "no C/C++ under components/ or main/ yet")
    return report("sticky code", code_hits(code))


def check_otp(code: dict[str, str]) -> bool:
    if not code:
        return report("sticky otp", [], "no C/C++ under components/ or main/ yet")
    return report("sticky otp", otp_hits(code))


def self_test() -> int:
    failures = 0

    def expect(name, got, want):
        nonlocal failures
        if got != want:
            failures += 1
            print(f"FAIL {name}: want {want!r}, got {got!r}")

    spec = ("## 2. X\n| `EP_CS` | GPIO99 |\n## 3. Pins\n| Signal | GPIO | Notes |\n| --- | --- | --- |\n"
            "| `EP_SCK` | GPIO13 | a |\n| `SD_CLK` | GPIO13 | b |\n| `EP_SCK` | GPIO13 | same again |\n"
            "| `U0TXD` | GPIO43 | c |\n| `IMU_INT` | GPIO7 | d |\n| `X_PIN` | GPIO7 / GPIO8 | e |\n"
            "| `Y_PIN` | GPIO5 | f |\n| `Y_PIN` | GPIO6 | g |\n## 4. Buses\n| `TP_INT` | GPIO21 |\n")
    pins, problems = spec_pins(spec)
    expect("spec pins, own section", pins, {"EP_SCK": 13, "SD_CLK": 13, "U0TXD": 43, "IMU_INT": 7, "Y_PIN": 5})
    expect("spec problems: two GPIOs in a cell, one signal at two GPIOs", len(problems), 2)
    expect("const name", const_name("EP_PWR_EN"), "kEpPwrEn")
    header = ("// inline constexpr gpio_num_t kOld = GPIO_NUM_1;\n"
              "inline constexpr gpio_num_t kEpSck = GPIO_NUM_13;  // EP_SCK\n"
              "inline constexpr gpio_num_t kSdClk = GPIO_NUM_12;\n"
              "inline constexpr gpio_num_t kExtra = GPIO_NUM_5;\n")
    hp = header_pins(header)
    expect("header pins, comments ignored", hp, {"kEpSck": 13, "kSdClk": 12, "kExtra": 5})
    expect("pin hits", len(compare_pins({"EP_SCK": 13, "SD_CLK": 13, "U0TXD": 43}, hp)), 3)
    expect("pins match", compare_pins({"EP_SCK": 13}, {"kEpSck": 13}), [])

    good = ("spi_bus_config_t bus = {};\nbus.quadwp_io_num = -1; bus.quadhd_io_num = - 1;\n"
            "bus.data4_io_num = -1; bus.data5_io_num{-1}; bus.data6_io_num = GPIO_NUM_NC;\n"
            "bus.data7_io_num = static_cast<int>(GPIO_NUM_NC);\nspi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO);\n")
    expect("assigned fields, every spelling", code_hits({"components/board/board.cpp": good}), [])
    designated = ("spi_bus_config_t bus{\n  .mosi_io_num = 14, .quadwp_io_num = -1, .quadhd_io_num = -1,\n"
                  "  .data4_io_num = -1, .data5_io_num = -1, .data6_io_num = -1, .data7_io_num = -1,\n};\n")
    expect("designated brace-init", code_hits({"a.cpp": designated}), [])
    expect("brace-init, nothing set", len(code_hits({"a.cpp": "spi_bus_config_t bus{};\n"})), 1)
    expect("plain declaration, nothing set", len(code_hits({"a.cpp": "spi_bus_config_t bus;\n"})), 1)
    expect("pointer and parameter are not declarations",
           code_hits({"a.cpp": "void f(spi_bus_config_t cfg);\nspi_bus_config_t *p = nullptr;\n"}), [])
    two = ("spi_bus_config_t a = {};\na.quadwp_io_num = -1; a.quadhd_io_num = -1; a.data4_io_num = -1;\n"
           "a.data5_io_num = -1; a.data6_io_num = -1; a.data7_io_num = -1;\nspi_bus_config_t b = {};\n")
    expect("checked per declaration", len(code_hits({"a.cpp": two})), 1)
    expect("free and two inits", len(code_hits({"a.cpp": "spi_bus_free(SPI2_HOST);\n// spi_bus_free(x);\n",
                                                "b.cpp": "spi_bus_initialize(a);\n", "c.cpp": "spi_bus_initialize(b);\n"})), 2)
    expect("a // inside a string hides nothing",
           len(code_hits({"a.cpp": 'const char *u = "http://x"; spi_bus_free(SPI2_HOST);\n'})), 1)
    hits = code_hits({"a.cpp": "/* one\n two */\nspi_bus_free(SPI2_HOST);\n"})
    expect("line numbers kept after a block comment", hits[0].split(":")[1] if hits else None, "3")
    expect("literal gpio outside board_config.h only",
           len(code_hits({"main/x.cpp": "gpio_set_level(GPIO_NUM_4, 1);\n", str(BOARD_CONFIG):
                          "inline constexpr gpio_num_t kA = GPIO_NUM_4;\n", "components/foo_board_config.h":
                          "constexpr gpio_num_t kB = GPIO_NUM_5;\n"})), 2)
    expect("GPIO_NUM_NC is not a literal pin", code_hits({"main/x.cpp": "int p = GPIO_NUM_NC;\n"}), [])
    # otp: Seeed's SSD2677 commands pass and anything else is a hit; the SSD1677's OTP and voltage commands are hits;
    # comments never count; other components are not read
    expect("SSD2677 allow-list", otp_hits({"components/ssd2677/ssd2677.cpp":
                                           "bus_.Write(0x00, {0x2F});\nbus_.Command(0xA5);\nbus_.Write(0xE6, {s});\n"
                                           "bus_.Write(0x83, {0x00, 0x00});\n"}), [])
    expect("SSD2677 opcode off the list", len(otp_hits({"components/ssd2677/ssd2677.cpp": "bus_.Write(0x84, {1});\n"})), 1)
    expect("SSD2677 OTP program and burn", len(otp_hits({"components/ssd2677/ssd2677.cpp":
                                                         "bus_.Command(0x90);\nbus_.Write(0x91, {1});\n"})), 2)
    expect("SSD2677 opcode in a comment", otp_hits({"components/ssd2677/ssd2677.cpp":
                                                    "// never send 0x90, as in Command(0x90)\n"}), [])
    expect("SSD1677 documented commands", otp_hits({"components/ssd1677/ssd1677.cpp":
                                                    "bus_.Write(0x22, {0x0C});\nbus_.Command(0x32);\nbus_.Command(0x20);\n"}), [])
    expect("SSD1677 OTP and voltage commands", len(otp_hits({"components/ssd1677/ssd1677.cpp":
                                                             "bus_.Command(0x30);\nbus_.Write(0x04, {1});\n"
                                                             "bus_.Write(0x2C, {0x30});\n"})), 3)
    expect("shared bus: the ID probe only", len(otp_hits({"components/epaper/epaper_bus.cpp":
                                                          "Command(0x70);\nbus.Command(0x12);\n"})), 1)
    expect("other components unread", otp_hits({"components/devices/devices.cpp": "Write(0x30, {1});\n"}), [])
    with contextlib.redirect_stdout(io.StringIO()):
        expect("unknown check name", main(["pin"]), 2)
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / SPEC).parent.mkdir(parents=True)
            (root / SPEC).write_text("## 3. Pins\n| `EP_SCK` | GPIO13 | a |\n## 4. Buses\n")
            expect("no firmware yet: pins skip", check_pins(root, read_code(root)), True)
            (root / "components" / "board").mkdir(parents=True)
            (root / "components" / "board" / "board.cpp").write_text("int x;\n")
            expect("code without board_config.h: a hit", check_pins(root, read_code(root)), False)
            (root / BOARD_CONFIG).parent.mkdir(parents=True)
            (root / BOARD_CONFIG).write_text("inline constexpr gpio_num_t kEpSck = GPIO_NUM_13;\n")
            expect("board_config.h matches the spec", check_pins(root, read_code(root)), True)
            (root / BOARD_CONFIG).write_text("inline constexpr gpio_num_t kEpSck = GPIO_NUM_14;\n")
            expect("board_config.h differs from the spec", check_pins(root, read_code(root)), False)
    print("sticky_check self-test: " + ("PASS" if not failures else f"{failures} FAILED"))
    return 1 if failures else 0


def main(argv: list[str]) -> int:
    if "--test" in argv:
        return self_test()
    which = [a for a in argv if not a.startswith("-")] or list(CHECK_NAMES)
    unknown = [a for a in which if a not in CHECK_NAMES]
    if unknown:
        print(f"sticky_check: unknown check {', '.join(unknown)} — one of {', '.join(CHECK_NAMES)}")
        return 2
    root = Path.cwd()
    code = read_code(root)
    ok = True
    if "pins" in which:
        ok = check_pins(root, code) and ok
    if "code" in which:
        ok = check_code(code) and ok
    if "otp" in which:
        ok = check_otp(code) and ok
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
