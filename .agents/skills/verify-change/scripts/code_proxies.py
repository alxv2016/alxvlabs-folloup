#!/usr/bin/env python3
"""The code-reviewer judge's measured proxies, on the change against HEAD: what a reviewer otherwise judges by eye, as
numbers with a range. A proxy outside its range caps its quality dimension at 2; it never fails a run.

    python3 .agents/skills/verify-change/scripts/code_proxies.py                 # the change against HEAD
    python3 .agents/skills/verify-change/scripts/code_proxies.py --all           # every source: the audit
    python3 .agents/skills/verify-change/scripts/code_proxies.py --save-baseline # this tree's warnings and size as the baseline
    python3 .agents/skills/verify-change/scripts/code_proxies.py --build-dir B   # a firmware build other than build/
    python3 .agents/skills/verify-change/scripts/code_proxies.py --log L         # a build log other than the newest kept
    python3 .agents/skills/verify-change/scripts/code_proxies.py --self-test

A review scores the change, an audit the product: bare, only the functions the diff against HEAD touches are measured
(a new file: all of its functions), the warnings new since the baseline, and the changed sources; with --all, every
function in the project's own sources, every warning in the build log, and every source. Measures, by dimension (the
keys of .agents/evals/weights.json):
  simplicity  the longest changed C/C++ function, in lines (at most 60), and its deepest nesting of control blocks (at
              most 4) — [J] ranges, the common review limits; a test file's functions count too
  fit         new compiler warnings in the project's own sources (none — build-run-test's check), from the newest build
              log quiet.py kept in .cache/harness/logs/, against the baseline's warnings, else those in changed files
  robustness  the app's size against its partition (at most 90 %), and its growth since the baseline — the image at most
              16 KiB, DIRAM at most 1 KiB, IRAM none (the ESP32-S3's IRAM is full) — [J] ranges; read with ESP-IDF's
              esp_idf_size from build/<project>.map
  tests       changed sources a host test does not touch — named on a tests/host test's `// host-sources:` line or
              #included by one; the parts only the bench can run (the board, its drivers, the display task) left out
What is not measured is said so, and no metric is written for it. Writes .cache/harness/proxies/code-reviewer.json —
{"judge", "at", "scope": "change" | "product", "metrics": [{"dimension", "name", "value", "range": [lo, hi], "ok",
"where", "base"?, "offenders"?, "base_offenders"?}]} — and prints a summary. In the change's scope a metric carries its
value at HEAD (`base`): the changed functions as HEAD holds them, with the functions over a limit there and here by
file and name; none for warnings and growth, which are new by construction; the untested sources HEAD already had. So
verify.py caps only what the change made worse (.agents/HARNESS.md §12.5).
The baseline is .cache/harness/proxies/baseline/ (warnings.json, size.json); the first run with a build saves one.
Python 3.9+, standard library only; ESP-IDF's Python for the size.
"""

from __future__ import annotations

import argparse
import glob
import json
import os
import re
import subprocess
import sys
import tempfile
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(subprocess.run(["git", "rev-parse", "--show-toplevel"], capture_output=True, text=True).stdout.strip()
            or ".")
PROXIES = ROOT / ".cache/harness/proxies"
BASELINE = PROXIES / "baseline"
C_EXT = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx"}
SKIP = ("managed_components/", "build/", ".cache/")
# Seeed's components, kept as Seeed ships them (AGENTS.md): their warnings are not the project's.
VENDORED = ("components/gt911/", "components/bq27220/", "components/pcf8563/", "components/debug_logging/")
# What only the bench can run: the board, its drivers, the display and input tasks, the app's start.
BENCH_ONLY = ("main/", "components/board/", "components/board_config/", "components/epaper/", "components/ssd1677/",
              "components/ssd2677/", "components/devices/", "components/lvgl_display/display.cpp",
              "components/lvgl_display/input.cpp", "components/lvgl_display/lvgl_display.cpp",
              "components/app_shell/app_shell.cpp") + VENDORED
LONGEST, DEEPEST = 60, 4
BIN_GROWTH, DIRAM_GROWTH, IRAM_GROWTH, PARTITION_SHARE = 16 * 1024, 1024, 0, 0.9


def git(*args: str) -> str:
    return subprocess.run(["git", *args], cwd=ROOT, capture_output=True, text=True, errors="replace").stdout


# --- What changed -------------------------------------------------------------------------------------------------------

def changed_files() -> list:
    names = set(git("diff", "--name-only", "HEAD").split()) | set(git("ls-files", "--others", "--exclude-standard").split())
    return sorted(n for n in names if Path(n).suffix in C_EXT and not n.startswith(SKIP) and (ROOT / n).is_file())


def all_sources() -> list:
    """Every C/C++ source of the project's own — tracked or new — Seeed's vendored components left out."""
    names = set(git("ls-files").split()) | set(git("ls-files", "--others", "--exclude-standard").split())
    return sorted(n for n in names if Path(n).suffix in C_EXT and not n.startswith(SKIP + VENDORED)
                  and (ROOT / n).is_file())


def changed_lines(path: str) -> set | None:
    """The new file's changed lines; None when the file is new (every line)."""
    if git("ls-files", "--", path).strip() == "":
        return None
    lines: set = set()
    for m in re.finditer(r"^@@ -\d+(?:,\d+)? \+(\d+)(?:,(\d+))? @@", git("diff", "-U0", "HEAD", "--", path), re.M):
        start, count = int(m.group(1)), int(m.group(2) or 1)
        lines.update(range(start, start + max(count, 1)))
    return lines


# --- Functions in C and C++ ---------------------------------------------------------------------------------------------

def blank(text: str) -> str:
    """Comments, string and character literals and preprocessor lines blanked to spaces, newlines kept."""
    out, i, n = list(text), 0, len(text)

    def wipe(a: int, b: int):
        for k in range(a, min(b, n)):
            if out[k] != "\n":
                out[k] = " "

    line_start = True
    while i < n:
        c = text[i]
        if line_start and c == "#":
            j = i
            while j < n and not (text[j] == "\n" and text[j - 1] != "\\"):
                j += 1
            wipe(i, j)
            i = j
            continue
        if c in " \t":
            i += 1
            continue
        line_start = c == "\n"
        if text.startswith("//", i):
            j = text.find("\n", i)
            j = n if j < 0 else j
            wipe(i, j)
            i = j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            wipe(i, j)
            i = j
        elif c in "\"'":
            j = i + 1
            while j < n and text[j] != c and text[j] != "\n":
                j += 2 if text[j] == "\\" else 1
            wipe(i + 1, j)
            i = j + 1
        else:
            i += 1
    return "".join(out)


QUALIFIERS = re.compile(r"(?:\s*(?:const|noexcept(?:\([^()]*\))?|override|final|mutable|volatile|&|&&|->\s*[\w:<>,\s*&]+))*\s*$")
CONTROL = re.compile(r"(?:\)|\]|\belse|\bdo|\btry)\s*$")


def function_name(header: str) -> str | None:
    """The name of the function whose body follows `header`, or None when the brace opens no function body."""
    h = header.rstrip()
    if re.search(r"\b(namespace|class|struct|union|enum)\b", h.split("(")[0]) or re.search(r"=\s*$|\breturn\b", h):
        return None
    init = re.search(r"\)\s*:\s*[\w:]+\s*[({]", h)  # a constructor's initialiser list: cut at its colon
    if init:
        h = h[: init.start() + 1]
    h = QUALIFIERS.sub("", h)
    if not h.endswith(")"):
        return None
    depth, k = 0, len(h) - 1
    while k >= 0:
        depth += {")": 1, "(": -1}.get(h[k], 0)
        if depth == 0:
            break
        k -= 1
    m = re.search(r"(~?[A-Za-z_][\w:~]*|operator\s*\S+)\s*$", h[:k])
    if not m or m.group(1) in {"if", "for", "while", "switch", "catch", "sizeof", "return"}:
        return None
    return m.group(1)


def functions(text: str) -> list:
    """[(name, first line, last line, nesting)] for every function body in a C or C++ file."""
    code = blank(text)
    found, stack, last_stop = [], [], 0  # stack: ("ns" | "fn" | "init" | "block", start, name, depth)
    fn = None  # [name, line, depth, deepest]
    for i, c in enumerate(code):
        if c in ";":
            last_stop = i + 1
        elif c == "{":
            header = code[last_stop:i]
            if fn is None and re.search(r"\)\s*:[^;{}]*\w\s*$", header):
                stack.append("member")  # a constructor's member initialised with braces: the body is still to come
                continue
            if fn is None:
                name = function_name(header)
                if name:
                    line = code.count("\n", 0, last_stop + len(header) - len(header.lstrip())) + 1
                    fn = [name, line, 1, 1]
                    stack.append("fn")
                else:
                    stack.append("ns")
            elif CONTROL.search(header):
                fn[2] += 1
                fn[3] = max(fn[3], fn[2])
                stack.append("block")
            else:
                stack.append("init")
            last_stop = i + 1
        elif c == "}":
            kind = stack.pop() if stack else "ns"
            if kind == "member":
                continue
            if kind == "block" and fn:
                fn[2] -= 1
            elif kind == "fn" and fn:
                found.append((fn[0], fn[1], code.count("\n", 0, i) + 1, fn[3] - 1))
                fn = None
            last_stop = i + 1
    return found


# --- Warnings -----------------------------------------------------------------------------------------------------------

WARNING = re.compile(r"^(?P<path>[^\s:][^:]*):(?P<line>\d+):(?:\d+:)? warning: (?P<msg>.*?)(?: \[(?P<flag>-W[^\]]+)\])?$", re.M)


def project_path(path: str) -> str | None:
    """A warning's file as the repo names it, or None when it is not the project's own source."""
    m = re.search(r"(?:^|/)((?:components|main)/.*)$", path)
    if not m or "managed_components/" in path or m.group(1).startswith(VENDORED):
        return None
    return m.group(1)


def warnings_in(log: str) -> list:
    out = []
    for m in WARNING.finditer(log):
        p = project_path(m.group("path"))
        if p:
            out.append({"file": p, "line": int(m.group("line")), "key": f"{p}: {m.group('msg')} [{m.group('flag') or ''}]"})
    return out


def newest_build_log() -> Path | None:
    logs = sorted(glob.glob(str(ROOT / ".cache/harness/logs/*.log")), key=os.path.getmtime, reverse=True)
    for path in logs[:40]:
        text = Path(path).read_text(errors="replace")
        if "ninja" in text or "Project build complete" in text or re.search(r"^\[\d+/\d+\]", text, re.M):
            return Path(path)
    return None


# --- Size ---------------------------------------------------------------------------------------------------------------

def idf_python() -> str | None:
    candidates = [sys.executable]
    if os.environ.get("IDF_PYTHON_ENV_PATH"):
        candidates.append(os.path.join(os.environ["IDF_PYTHON_ENV_PATH"], "bin", "python"))
    candidates += sorted(glob.glob(os.path.expanduser("~/.espressif/tools/python/*/venv/bin/python")), reverse=True)
    for py in candidates:
        if py and os.path.exists(py) and subprocess.run([py, "-c", "import esp_idf_size"], capture_output=True).returncode == 0:
            return py
    return None


def size_of(build: Path) -> dict | None:
    """{bin, diram, iram, partition} of the build's app, or None when there is no build to read."""
    desc = build / "project_description.json"
    try:
        project = json.loads(desc.read_text())
        name = project.get("project_name") or project.get("app_elf", "").replace(".elf", "")
    except (OSError, ValueError):
        name = None
    if not name:
        maps = sorted(build.glob("*.map"))
        name = maps[0].stem if maps else None
    if not name or not (build / f"{name}.bin").is_file():
        return None
    out = {"bin": (build / f"{name}.bin").stat().st_size, "partition": app_partition()}
    py = idf_python()
    if py and (build / f"{name}.map").is_file():
        proc = subprocess.run([py, "-m", "esp_idf_size", "--format", "json2", str(build / f"{name}.map")],
                              capture_output=True, text=True)
        try:
            for part in json.loads(proc.stdout).get("layout", []):
                if part.get("name") == "DIRAM":
                    out["diram"] = part.get("used")
                elif part.get("name") == "IRAM":
                    out["iram"] = part.get("used")
        except ValueError:
            pass
    return out


def app_partition() -> int | None:
    """The size of the first app partition in partitions.csv."""
    try:
        for line in (ROOT / "partitions.csv").read_text().splitlines():
            cols = [c.strip() for c in line.split("#")[0].split(",")]
            if len(cols) >= 5 and cols[1] == "app":
                return int(cols[4], 0) if not cols[4][-1:].upper() in "KM" else int(cols[4][:-1]) * (1024 if cols[4][-1].upper() == "K" else 1024 * 1024)
    except (OSError, ValueError):
        return None
    return None


# --- Host tests ---------------------------------------------------------------------------------------------------------

def host_tested() -> set:
    """Every repo path a host test compiles or #includes."""
    touched: set = set()
    for test in glob.glob(str(ROOT / "tests/host/*_test.cpp")):
        text = Path(test).read_text(errors="replace")
        for m in re.finditer(r"^// host-sources:(.*)$", text, re.M):
            for pattern in m.group(1).split():
                touched.update(str(Path(p).relative_to(ROOT)) for p in glob.glob(str(ROOT / pattern)))
        for m in re.finditer(r'^\s*#include\s+"([^"]+)"', text, re.M):
            inc = m.group(1)
            if inc.startswith("../"):
                touched.add(os.path.normpath(os.path.join("tests/host", inc)))
            else:
                touched.update(str(Path(p).relative_to(ROOT)) for p in glob.glob(str(ROOT / "components/*/include/**" / inc), recursive=True))
                touched.update(str(Path(p).relative_to(ROOT)) for p in glob.glob(str(ROOT / "components/*/src" / inc)))
    return touched


# --- The proxies --------------------------------------------------------------------------------------------------------

def metric(dimension: str, name: str, value: float, lo: float, hi: float, where: str, **base) -> dict:
    """A metric; `base`, in the change's scope, is its value at HEAD (`base`) and what was out of range there
    (`base_offenders`, beside this tree's `offenders`), so verify.py caps only what the change made worse."""
    return {"dimension": dimension, "name": name, "value": value, "range": [lo, hi], "ok": lo <= value <= hi,
            "where": where, **base}


def at_head(path: str) -> str | None:
    """The file as HEAD holds it; None when HEAD does not have it."""
    proc = subprocess.run(["git", "show", f"HEAD:{path}"], cwd=ROOT, capture_output=True, text=True, errors="replace")
    return proc.stdout if proc.returncode == 0 else None


def over_limits(path: str, text: str | None) -> tuple:
    """({"path name": lines} over LONGEST, {"path name": nesting} over DEEPEST, {"path name": (lines, nesting)}) for
    every function in a file — the places the simplicity metrics name."""
    longer, deeper, every = {}, {}, {}
    for name, first, last, nest in functions(text or ""):
        key = f"{path} {name}"
        size = last - first + 1
        every[key] = (max(size, every.get(key, (0, 0))[0]), max(nest, every.get(key, (0, 0))[1]))
        if size > LONGEST:
            longer[key] = max(size, longer.get(key, 0))
        if nest > DEEPEST:
            deeper[key] = max(nest, deeper.get(key, 0))
    return longer, deeper, every


def measure(build: Path, save_baseline: bool, log_file: Path | None = None, product: bool = False) -> tuple:
    metrics, notes = [], []
    files = all_sources() if product else changed_files()
    # Simplicity: the changed functions (the audit: every function).
    changed_fns = []
    for f in files:
        text = (ROOT / f).read_text(errors="replace")
        lines = None if product else changed_lines(f)
        for name, first, last, nest in functions(text):
            if lines is None or any(first <= n <= last for n in lines):
                changed_fns.append((f, name, first, last, nest))
    if changed_fns:
        by_len = sorted(changed_fns, key=lambda t: t[3] - t[2], reverse=True)
        by_nest = sorted(changed_fns, key=lambda t: t[4], reverse=True)
        long_base, deep_base = {}, {}
        if not product:
            # The base: each changed function as HEAD holds it, by file and name — a function already over a limit
            # there and no worse is carried debt; a new one, or one the change made longer or deeper, is not.
            heads = {f: over_limits(f, at_head(f)) for f in {t[0] for t in changed_fns}}
            keys = {f"{t[0]} {t[1]}" for t in changed_fns}
            was = [heads[k.split(" ", 1)[0]][2][k] for k in keys if k in heads[k.split(" ", 1)[0]][2]]
            long_base = {"base": max((w[0] for w in was), default=None),
                         "offenders": {f"{t[0]} {t[1]}": t[3] - t[2] + 1 for t in changed_fns
                                       if t[3] - t[2] + 1 > LONGEST},
                         "base_offenders": {k: v for h in heads.values() for k, v in h[0].items()}}
            deep_base = {"base": max((w[1] for w in was), default=None),
                         "offenders": {f"{t[0]} {t[1]}": t[4] for t in changed_fns if t[4] > DEEPEST},
                         "base_offenders": {k: v for h in heads.values() for k, v in h[1].items()}}
        f, name, first, last, _ = by_len[0]
        over = [t for t in by_len if t[3] - t[2] + 1 > LONGEST]
        metrics.append(metric("simplicity", "longest changed function (lines)", last - first + 1, 0, LONGEST,
                              f"{f}:{first} {name}; {len(over)} of {len(changed_fns)} changed functions over {LONGEST}: "
                              + ", ".join(f"{t[1]} {t[3] - t[2] + 1}" for t in over[:4]), **long_base))
        f, name, first, _, nest = by_nest[0]
        deep = [t for t in by_nest if t[4] > DEEPEST]
        metrics.append(metric("simplicity", "deepest nesting in a changed function", nest, 0, DEEPEST,
                              f"{f}:{first} {name}; {len(deep)} over {DEEPEST}", **deep_base))
    else:
        notes.append("simplicity: no changed C or C++ function")
    # Fit: new warnings.
    log = log_file or newest_build_log()
    current = warnings_in(log.read_text(errors="replace")) if log else []
    base_file = BASELINE / "warnings.json"
    if log is None:
        notes.append("fit: no build log in .cache/harness/logs/ — build through quiet.py to measure warnings")
    else:
        if product:
            new, against = current, "every warning in the log (the product)"
        elif base_file.is_file():
            known = set(json.loads(base_file.read_text()))
            new = [w for w in current if w["key"] not in known]
            against = "the baseline's"
        else:
            new = [w for w in current if w["file"] in files]
            against = "none in changed files"
        # Against the baseline the count is already what the change added: none at the base.
        metrics.append(metric("fit", "new compiler warnings in the project's sources", len(new), 0, 0,
                              f"{log.name} against {against}: "
                              + ("; ".join(f"{w['file']}:{w['line']} {w['key'].split(': ', 1)[1][:80]}" for w in new[:3])
                                 or "none"), **({"base": 0} if against == "the baseline's" else {})))
    # Robustness: size.
    size = size_of(build)
    size_file = BASELINE / "size.json"
    if size is None:
        notes.append(f"robustness: no firmware build in {build.relative_to(ROOT) if build.is_relative_to(ROOT) else build}"
                     " — the size is not measured")
    else:
        base = json.loads(size_file.read_text()) if size_file.is_file() else {}
        if size.get("partition"):
            share = size["bin"] / size["partition"]
            was = {"base": round(base["bin"] / size["partition"], 4)} if base.get("bin") and not product else {}
            metrics.append(metric("robustness", "app image's share of its partition", round(share, 4), 0, PARTITION_SHARE,
                                  f"{size['bin']} of {size['partition']} bytes", **was))
        if base:
            for key, label, cap in (("bin", "app image growth (bytes)", BIN_GROWTH),
                                    ("diram", "DIRAM growth (bytes)", DIRAM_GROWTH),
                                    ("iram", "IRAM growth (bytes)", IRAM_GROWTH)):
                if size.get(key) is not None and base.get(key) is not None:
                    metrics.append(metric("robustness", label, size[key] - base[key], -10**9, cap,
                                          f"{base[key]} → {size[key]} bytes", **({} if product else {"base": 0})))
        else:
            notes.append("robustness: no size baseline yet — this build's size is saved as it")
            save_baseline = True
        if size.get("diram") is None:
            notes.append("robustness: esp_idf_size not found — DIRAM and IRAM not measured")
    # Tests: a host test touches each changed source.
    sources = [f for f in files if f.startswith(("components/", "main/")) and not f.startswith(BENCH_ONLY)]
    if sources:
        touched = host_tested()
        bare = [f for f in sources if f not in touched]
        # The base: a source HEAD already had, untested there — known only while no host test changed.
        was = {} if product or any(f.startswith("tests/host/") for f in files) else {
            "base": None if not bare else sum(1 for f in bare if at_head(f) is not None),
            "offenders": {f: 1 for f in bare}, "base_offenders": {f: 1 for f in bare if at_head(f) is not None}}
        metrics.append(metric("tests", "changed sources no host test touches", len(bare), 0, 0,
                              (", ".join(bare[:5]) or f"each of {len(sources)} is compiled or included by a host test"),
                              **was))
    else:
        notes.append("tests: no changed source outside the bench-only parts")
    if save_baseline:
        BASELINE.mkdir(parents=True, exist_ok=True)
        if log:
            base_file.write_text(json.dumps(sorted({w["key"] for w in current}), indent=1))
        if size:
            size_file.write_text(json.dumps(size, indent=1))
        notes.append(f"baseline saved: {BASELINE.relative_to(ROOT)}/")
    return metrics, notes, len(files), [f"{t[0]}:{t[2]} {t[1]}" for t in changed_fns]


def write(judge: str, metrics: list, scope: str = "change", in_scope: list | None = None) -> Path:
    """The results; `in_scope` names the functions measured — empty, the change is out of this judge's scope."""
    PROXIES.mkdir(parents=True, exist_ok=True)
    path = PROXIES / f"{judge}.json"
    at = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    path.write_text(json.dumps({"judge": judge, "at": at, "scope": scope, "scope_what": "the C/C++ functions the change "
                                "wrote in", "in_scope": in_scope or [], "metrics": metrics}, indent=1,
                               ensure_ascii=False) + "\n")
    return path


def self_test() -> int:
    failures = []

    def check(ok: bool, what: str):
        if not ok:
            failures.append(what)

    src = '''#include "x.h"  // { not a brace
namespace a {
namespace {
struct S { int x; };
const char *kWords = "if (x) { y }";
int Flat(int x) { return x + 1; }
void Deep(int x)
{
    Box b{1, 2};
    if (x) {
        for (int i = 0; i < x; ++i) {
            while (true) {
                if (i) { break; }
            }
        }
    } else {
        auto f = [](int y) { return y; };
    }
}
}  // namespace
Thing::Thing(int v) : value_(v), other_{v}
{
    if (v) {
        value_ = 0;
    }
}
bool Thing::Ok() const noexcept { return true; }
}  // namespace a
'''
    fns = {name: (first, last, nest) for name, first, last, nest in functions(src)}
    check(set(fns) == {"Flat", "Deep", "Thing::Thing", "Thing::Ok"}, f"every function body, no struct or namespace: {sorted(fns)}")
    check(fns.get("Flat") == (6, 6, 0), f"a one-line function: 1 line, nesting 0 — {fns.get('Flat')}")
    check(fns.get("Deep", (0, 0, 0))[2] == 4, f"Deep nests if > for > while > if: 4, an initialiser and a lambda's body "
                                              f"aside — {fns.get('Deep')}")
    check(fns.get("Deep", (0, 0, 0))[:2] == (7, 19), f"Deep runs from its name to its brace — {fns.get('Deep')}")
    check(fns.get("Thing::Thing", (0, 0, 0))[2] == 1, "a constructor with an initialiser list is one function")
    at = ":12:5: "  # a compiler's file, line and column, kept apart so the source holds no file:line reference
    log = (f"/Users/x/repo/components/ui/ui.cpp{at}warning: unused variable 'y' [-Wunused-variable]\n"
           f"/Users/x/repo/managed_components/lvgl__lvgl/src/a.c{at}warning: deprecated [-Wdeprecated]\n"
           f"/Users/x/repo/components/gt911/gt911.cpp{at}warning: shadow [-Wshadow]\n"
           "[12/40] Building CXX object\n")
    ws = warnings_in(log)
    check([w["file"] for w in ws] == ["components/ui/ui.cpp"], f"only the project's own warnings count: {ws}")
    check(ws and ws[0]["key"] == "components/ui/ui.cpp: unused variable 'y' [-Wunused-variable]", "a warning's key has no line")
    m = metric("tests", "x", 1, 0, 0, "w")
    check(m["ok"] is False and m["range"] == [0, 0], "a value past its range is not ok")
    check(project_path("/a/b/main/main.cpp") == "main/main.cpp", "main/ is the project's")
    with tempfile.TemporaryDirectory() as tmp:
        b = Path(tmp)
        check(size_of(b) is None, "no build: no size")
        (b / "project_description.json").write_text(json.dumps({"project_name": "app"}))
        (b / "app.bin").write_bytes(b"\0" * 1000)
        s = size_of(b)
        check(s is not None and s["bin"] == 1000, f"the image's size from the build: {s}")
    # The base: the places over a limit, by file and function, as HEAD holds them.
    big = "void Big()\n{\n" + "    x();\n" * 70 + "}\nvoid Small() { y(); }\n"
    longer, deeper, every = over_limits("a.cpp", big)
    check(longer == {"a.cpp Big": 73} and not deeper and set(every) == {"a.cpp Big", "a.cpp Small"},
          f"a function over the limit is named by file and name: {longer}, {sorted(every)}")
    check(over_limits("a.cpp", None) == ({}, {}, {}), "a file HEAD does not have has nothing at the base")
    m = metric("simplicity", "x", 73, 0, 60, "w", base=73, offenders={"a.cpp Big": 73}, base_offenders=longer)
    check(m["base"] == 73 and m["offenders"] == m["base_offenders"], "a metric carries its base and its places")
    for f in failures:
        print(f"FAIL  {f}")
    total = 14
    print(f"code_proxies self-test: {total - len(failures)} of {total} checks pass")
    return 1 if failures else 0


def summary(judge: str, metrics: list, notes: list, extra: str) -> None:
    outside = [m for m in metrics if not m["ok"]]
    print(f"{judge}: {len(metrics)} metrics, {len(outside)} outside their range — {extra}")
    for m in metrics:
        lo, hi = m["range"]
        print(f"  {'ok ' if m['ok'] else 'OUT'} {m['dimension']}: {m['name']} = {m['value']:g} "
              f"(range {lo:g}–{hi:g}) — {m['where'][:170]}")
    for n in notes:
        print(f"  - {n}")


def main(argv: list) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--build-dir", default=str(ROOT / "build"), help="the firmware build to size (build/)")
    parser.add_argument("--save-baseline", action="store_true", help="save this tree's warnings and size as the baseline")
    parser.add_argument("--log", help="the build log to read warnings from (the newest in .cache/harness/logs/)")
    parser.add_argument("--all", action="store_true", help="every source and function: the whole-product audit")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    if args.self_test:
        return self_test()
    metrics, notes, n_files, fns = measure(Path(args.build_dir).resolve(), args.save_baseline,
                                             Path(args.log).resolve() if args.log else None, args.all)
    scope = "product" if args.all else "change"
    path = write("code-reviewer", metrics, scope, fns)
    n_fns = len(fns)
    what = "" if args.all else "changed "
    summary(f"code-reviewer ({scope})", metrics, notes, f"{n_files} {what}C/C++ files, {n_fns} {what}functions; "
                                                       f"{path.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
