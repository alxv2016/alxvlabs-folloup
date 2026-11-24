#!/usr/bin/env python3
"""The screen audit: builds and runs tests/host/screen_audit_lvgl_test.cpp — every screen, the UI kit page a glass-full at a
time and its overlays, drawn by the firmware's own code on LVGL's renderer — and prints what it measured for the
ui-designer judge (hierarchy, spacing, consistency, balance, state, medium) and the ux-designer judge (flow).

    python3 .agents/skills/ui-designer/scripts/screen_audit.py              # the change: the screens it touched
    python3 .agents/skills/ui-designer/scripts/screen_audit.py --all        # every screen: the whole-product audit
    python3 .agents/skills/ui-designer/scripts/screen_audit.py --no-build   # summarise the last run's JSON
    python3 .agents/skills/ui-designer/scripts/screen_audit.py --every      # every metric, not only those outside
    python3 .agents/skills/ui-designer/scripts/screen_audit.py --self-test

A review scores the change, an audit the product. Bare, the audit keeps only the metrics of the screens whose sources the
change touched (against HEAD): each screen is mapped to the function in components/hw_test/hw_test.cpp that builds it
(SCREENS), and a function is touched when the change wrote in it or it calls one that is — across files by the names the
UI's headers declare, so a widget change touches every screen that uses that widget. A change to the shared foundation
(EVERY_SCREEN: ui, lvgl_display, router, the tokens) touches every screen; a change to no UI source touches none, and the
program is not even built — no UI cap applies. With --all, every metric is kept.

The program writes .cache/harness/proxies/ui-designer.json and ux-designer.json — `{"judge", "at", "scope": "change" |
"product", "screens": [...], "metrics": [{"dimension", "name", "value", "range": [lo, hi], "ok", "where", "screens"}]}` —
and a one-bit PNG of each view to build/host/audit/, and of each ringed stop to build/host/audit/rings/. A metric
outside its range caps its dimension's score; it never fails the run. Exit 0 when the audit ran, 1 when it could not
build or run, or a fixture — two labels at known gaps, or rings cut, let out and doubled — misread.
Bare, each metric also carries `base`: its value at the same place — dimension, name, view, quoted words — when HEAD
draws every screen (null: no such place there), so verify.py caps only what the change made worse
(.agents/HARNESS.md §12.5). HEAD's audit is run once per commit in a git worktree under .cache/harness/worktrees/, which
shares this tree's managed_components and compiled LVGL (about 15 s), then cached in .cache/harness/proxies/base/
<commit>/ and the worktree removed; --no-base leaves it out.
The first run compiles LVGL into build/host/lvgl (about half a minute); later runs take about 10 s. Python 3.9+, standard
library only.
"""

from __future__ import annotations

import argparse
import importlib.util
import json
import re
import shutil
import subprocess
import sys
import tempfile
import time
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(subprocess.run(["git", "rev-parse", "--show-toplevel"], capture_output=True, text=True).stdout.strip()
            or ".")
PROXIES = ROOT / ".cache/harness/proxies"
JUDGES = ("ui-designer", "ux-designer")
_spec = importlib.util.spec_from_file_location("code_proxies", ROOT / ".agents/skills/verify-change/scripts/code_proxies.py")
code_proxies = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(code_proxies)  # its C/C++ function reader, comments and strings blanked

# --- Which screens a change touched -------------------------------------------------------------------------------------

HW_TEST = "components/hw_test/hw_test.cpp"
# Each screen the audit names (its kScreens and its views) → the functions in hw_test.cpp it is built from — the route's
# factory, which names its create, enter and leave callbacks — and any file that is wholly its own.
SCREENS = {
    "home": {"roots": ("BuildHome",), "files": ("components/app_shell/screen_home.cpp",
                                                "components/app_shell/screen_home.h")},
    "display": {"roots": ("DisplayScreen",)}, "four greys": {"roots": ("GraysScreen",)},
    "type scale": {"roots": ("TypeScaleScreen",)}, "kit": {"roots": ("KitScreen",)},
    "benchmark": {"roots": ("BenchmarkScreen",)}, "touch": {"roots": ("TouchScreen",)},
    "buttons": {"roots": ("ButtonsScreen",)}, "sensors": {"roots": ("SensorsScreen",)},
    "clock": {"roots": ("ClockScreenRoute",)}, "welcome": {"roots": ("WelcomeScreen",)},
    "power": {"roots": ("PowerScreen",)}, "storage": {"roots": ("StorageScreen",)},
    "microphone": {"roots": ("MicrophoneScreen",)}, "reminders": {"roots": ("RemindersScreen",)},
    "about": {"roots": ("AboutScreen",)},
    "status line": {"roots": ("StorageScreen",)},  # Storage's news, said through router::Say
    "keyboard": {"roots": ("KitScreen",)}, "dialog": {"roots": ("KitScreen",)}, "select sheet": {"roots": ("KitScreen",)},
}
# The foundation every screen stands on: a change here touches them all.
EVERY_SCREEN = ("components/ui/", "components/lvgl_display/", "components/router/", "components/design_tokens/",
                "components/app_hooks/", "components/widgets/marks/")
# The sources whose functions are read for who calls whom; the headers' names are what one file may call in another.
UI_SOURCES = ("components/hw_test/", "components/app_shell/screen_", "components/widgets/")
C_EXT = (".c", ".cc", ".cpp", ".h", ".hpp")
DECLARED = re.compile(r"([A-Za-z_]\w*)\s*(?:\[[^\]]*\])?\s*(?:[=\[({;:]|$)")


def is_ui_source(path: str) -> bool:
    return path.endswith(C_EXT) and path.startswith(UI_SOURCES + EVERY_SCREEN)


def declared_on(line: str) -> str | None:
    """The name a declaration line declares: the last identifier before its first `=`, `[`, `(`, `{`, `;` or `:`."""
    head = re.split(r"[=\[({;]", line, maxsplit=1)[0]
    names = [n for n in re.findall(r"[A-Za-z_]\w*", head) if n not in ("const", "constexpr", "static", "inline", "struct",
                                                                      "class", "enum", "namespace", "typedef", "using")]
    return names[-1] if names else None


def names_on_lines(code: str, lines: set, inside: list) -> tuple[set, bool]:
    """The names the changed lines outside every function declare — a line inside an initializer gives the name of the
    array or struct it fills — and whether a changed line could not be placed (then the whole file is taken)."""
    rows = code.split("\n")
    names, lost = set(), False
    for n in sorted(lines):
        if not 1 <= n <= len(rows) or any(a <= n <= b for a, b in inside):
            continue
        text = rows[n - 1].strip()
        if not text or text in ("{", "}", "};", "});"):
            continue  # blank, a comment (blanked) or a brace: nothing drawn changes
        name = declared_on(text) if re.match(r"[A-Za-z_]", text) else None
        k = n
        while name is None and k > 1 and n - k < 400:  # inside an initializer: the declaration that opens it
            k -= 1
            up = rows[k - 1].rstrip()
            if up.endswith("{") and re.match(r"\s*[A-Za-z_]", up):
                name = declared_on(up.strip())
        if name is None:
            lost = True
        else:
            names.add(name)
    return names, lost


def touched_screens(sources: dict, changes: dict) -> tuple[set, str]:
    """The screens a change touched, and why. `sources` {path: text} holds the UI sources; `changes` {path: changed
    lines, or None for a new file} holds every file the change wrote."""
    ui = sorted(f for f in changes if is_ui_source(f))
    if not ui:
        return set(), "no UI source changed — no screen is in the change's scope"
    base = [f for f in ui if f.startswith(EVERY_SCREEN)]
    if base:
        return set(SCREENS), f"{base[0]} is the foundation every screen stands on"
    fns, names, public = [], set(), set()  # fns: (path, short name, body's identifiers)
    for path, text in sources.items():
        code = code_proxies.blank(text)
        if path.endswith((".h", ".hpp")):
            public.update(re.findall(r"[A-Za-z_]\w*", code))
        found = code_proxies.functions(text)
        rows = code.split("\n")
        for name, first, last, _ in found:
            short = name.split("::")[-1]
            fns.append((path, short, set(re.findall(r"[A-Za-z_]\w*", "\n".join(rows[first - 1:last])))))
            lines = changes.get(path, set()) if path in changes else set()
            if path in changes and (lines is None or any(first <= n <= last for n in lines)):
                names.add((path, short))
        if path in changes:
            spans = [(a, b) for _, a, b, _ in found]
            lines = changes[path]
            if lines is None:
                names.update((path, n.split("::")[-1]) for n, *_ in found)
                continue
            declared, lost = names_on_lines(code, lines, spans)
            names.update((path, n) for n in declared)
            if lost:
                names.update((path, n.split("::")[-1]) for n, *_ in found)
    whole = {s for s, spec in SCREENS.items() if any(f in changes for f in spec.get("files", ()))}
    touched = called_by(fns, names, public)
    hit = {s for s, spec in SCREENS.items() if any((HW_TEST, r) in touched for r in spec["roots"])} | whole
    return hit, f"{len(touched)} function(s) and name(s) touched in {', '.join(ui[:3])}"


def called_by(fns: list, names: set, public: set) -> set:
    """Every (path, name) the touched names reach upward: a function is touched when it names a touched one — in its
    own file, or in another by a name a header declares."""
    touched = set(names)
    grew = True
    while grew:
        grew = False
        by_name: dict = defaultdict(set)
        for path, n in touched:
            by_name[n].add(path)
        for path, short, idents in fns:
            if (path, short) in touched:
                continue
            for n in idents & set(by_name):
                if path in by_name[n] or n in public:
                    touched.add((path, short))
                    grew = True
                    break
    return touched


def git(*args: str) -> str:
    return subprocess.run(["git", *args], cwd=ROOT, capture_output=True, text=True, errors="replace").stdout


def change_against_head() -> dict:
    """{path: the new file's changed lines, or None when the file is new} for every file the change wrote."""
    out = {}
    for f in set(git("diff", "--name-only", "HEAD").split()):
        out[f] = code_proxies.changed_lines(f) if (ROOT / f).is_file() else set()
    for f in git("ls-files", "--others", "--exclude-standard").split():
        out[f] = None
    return out


def ui_sources() -> dict:
    names = set(git("ls-files").split()) | set(git("ls-files", "--others", "--exclude-standard").split())
    return {f: (ROOT / f).read_text(errors="replace") for f in sorted(names)
            if f.startswith(UI_SOURCES) and f.endswith(C_EXT) and (ROOT / f).is_file()}


def in_scope(metric: dict, screens: set) -> bool:
    """A metric belongs to the change when a screen it measured was touched; one naming a screen the audit does not map
    belongs to any change that touched a screen."""
    named = set(metric.get("screens") or ())
    return bool(screens) and (not named or bool(named & screens) or bool(named - set(SCREENS)))


def scope_to_change(data: dict, screens: set, why: str) -> dict:
    kept = [m for m in data.get("metrics", []) if in_scope(m, screens)]
    return {**data, "scope": "change", "screens": sorted(screens), "in_scope": sorted(screens), "why": why,
            "scope_what": "the screens whose sources (screens, widgets, tokens, routes) the change touched",
            "metrics": kept}


def write(judge: str, data: dict) -> None:
    PROXIES.mkdir(parents=True, exist_ok=True)
    (PROXIES / f"{judge}.json").write_text(json.dumps(data, indent=1, ensure_ascii=False) + "\n")


# --- The base: every screen as HEAD draws it, so verify.py caps only what the change made worse --------------------

BASE_DIR = PROXIES / "base"
WORKTREES = ROOT / ".cache/harness/worktrees"


def metric_key(m: dict) -> tuple:
    """A metric's place, kept across an edit elsewhere: its dimension, its name, its view — a kit page's number
    dropped, since a page added before it renumbers the rest — and the words it quotes."""
    where = str(m.get("where", ""))
    view = re.sub(r"\s+p\d+$", "", where.split(":", 1)[0].strip())
    return m.get("dimension"), m.get("name"), view, tuple(re.findall(r"'([^']*)'", where))


def outside(m: dict) -> float:
    lo, hi = m["range"]
    return max(lo - m["value"], m["value"] - hi, 0)


def with_base(data: dict, base: dict) -> dict:
    """Each metric with `base`, its value at the same place at the base — the one with the same `where`, else the
    farthest out of those left at its key — or null when the base has no such place left. Each base metric is matched
    once: a second violation at a key the base held once is new, even on a renumbered kit page."""
    index = defaultdict(list)
    for b in base.get("metrics", []):
        index[metric_key(b)].append(b)
    picks: dict = {}
    for k, m in enumerate(data["metrics"]):  # the same `where` first, so a place that stayed keeps its own base
        found = index.get(metric_key(m), [])
        same = next((b for b in found if b.get("where") == m.get("where")), None)
        if same is not None:
            picks[k] = same
            found.remove(same)
    for k, m in enumerate(data["metrics"]):
        if k not in picks:
            found = index.get(metric_key(m), [])
            pick = max(found, key=outside, default=None)
            if pick is not None:
                found.remove(pick)
            picks[k] = pick
    for k, m in enumerate(data["metrics"]):
        m["base"] = picks[k]["value"] if picks[k] else None
    return {**data, "base_commit": base.get("commit")}


def drop_worktree(wt: Path) -> None:
    """The base's worktree gone: its links into this tree removed first, so nothing is deleted through them."""
    for link in (wt / "managed_components", wt / "build/host/lvgl"):
        if link.is_symlink():
            link.unlink()
    # Only this worktree: a repo-wide `git worktree prune` could drop another worktree's records.
    subprocess.run(["git", "worktree", "remove", "--force", str(wt)], cwd=ROOT, capture_output=True)
    shutil.rmtree(wt, ignore_errors=True)


def base_results(commit: str) -> tuple:
    """({judge: every screen's results at `commit`}, how they were had). Cached by the commit's hash in
    .cache/harness/proxies/base/; else audited once in a git worktree of that commit under .cache/harness/worktrees/,
    which shares this tree's managed_components and compiled LVGL, so only the audit and its fonts compile. The worktree
    is removed after. ({}, why) when the base cannot be had — then each metric caps as before."""
    cache = BASE_DIR / commit
    got = {j: load(cache / f"{j}.json") for j in JUDGES}
    if all(got.values()):
        return got, f"the base {commit[:9]}, from the cache"
    if not (ROOT / "managed_components/lvgl__lvgl/src").is_dir():
        return {}, "no managed_components/lvgl__lvgl to build the base with — run a firmware build once"
    t0, wt = time.time(), WORKTREES / f"base-{commit[:12]}"
    drop_worktree(wt)
    WORKTREES.mkdir(parents=True, exist_ok=True)
    # -f: a record of this path left by a run that died is taken over, not refused
    if subprocess.run(["git", "worktree", "add", "-f", "--detach", "--quiet", str(wt), commit], cwd=ROOT,
                      capture_output=True).returncode != 0:
        return {}, f"git worktree add at {commit[:9]} failed"
    try:
        (wt / "managed_components").symlink_to(ROOT / "managed_components", target_is_directory=True)
        (wt / "build/host").mkdir(parents=True, exist_ok=True)
        if (ROOT / "build/host/lvgl").is_dir():
            (wt / "build/host/lvgl").symlink_to(ROOT / "build/host/lvgl", target_is_directory=True)
        (wt / ".cache/harness/proxies").mkdir(parents=True, exist_ok=True)
        proc = subprocess.run(["bash", "tests/host/run.sh", "screen_audit"], cwd=wt, capture_output=True, text=True,
                              errors="replace")
        got = {j: load(wt / ".cache/harness/proxies" / f"{j}.json") for j in JUDGES}
        if proc.returncode != 0 or not all(got.values()):
            return {}, f"the audit at the base {commit[:9]} failed (exit {proc.returncode})"
        shutil.rmtree(BASE_DIR, ignore_errors=True)  # one base kept: this one
        cache.mkdir(parents=True)
        for j, d in got.items():
            (cache / f"{j}.json").write_text(json.dumps({**d, "commit": commit}, indent=1, ensure_ascii=False) + "\n")
        return got, f"the base {commit[:9]}, audited once in a worktree in {time.time() - t0:.0f} s"
    finally:
        drop_worktree(wt)


def build_and_run() -> tuple[int, str]:
    """Runs the host test runner on the audit alone; (exit, the lines that are not compiler chatter)."""
    proc = subprocess.run(["bash", "tests/host/run.sh", "screen_audit"], cwd=ROOT, capture_output=True, text=True,
                          errors="replace")
    keep = [line for line in (proc.stdout + proc.stderr).splitlines()
            if line.strip() and "Include public headers" not in line and "#warning" not in line
            and not line.lstrip().startswith(("^", "|")) and not line.lstrip()[:1].isdigit()]
    return proc.returncode, "\n".join(keep)


def load(path: Path) -> dict | None:
    try:
        data = json.loads(path.read_text())
    except (OSError, ValueError):
        return None
    return data if isinstance(data, dict) and isinstance(data.get("metrics"), list) else None


def problems(data: dict) -> list:
    """The ways a proxy file breaks the contract, empty when it keeps it."""
    out = []
    for key in ("judge", "at", "metrics"):
        if key not in data:
            out.append(f"no {key}")
    for i, m in enumerate(data.get("metrics", [])):
        for key, kind in (("dimension", str), ("name", str), ("value", (int, float)), ("range", list), ("ok", bool),
                          ("where", str)):
            if not isinstance(m.get(key), kind):
                out.append(f"metric {i}: {key} missing or not {getattr(kind, '__name__', 'a number')}")
        if isinstance(m.get("range"), list) and len(m["range"]) != 2:
            out.append(f"metric {i}: range is not [lo, hi]")
    return out


def summarise(data: dict, show_all: bool, limit: int = 12) -> list:
    """The summary's lines: per dimension, how many metrics and how many outside, then the outside ones."""
    by_dim = defaultdict(lambda: [0, 0])
    for m in data["metrics"]:
        by_dim[m["dimension"]][0] += 1
        by_dim[m["dimension"]][1] += 0 if m["ok"] else 1
    outside = [m for m in data["metrics"] if not m["ok"]]
    lines = [f"{data['judge']}: {len(data['metrics'])} metrics, {len(outside)} outside their range (at {data['at']})"]
    for dim, (n, bad) in sorted(by_dim.items()):
        lines.append(f"  {dim:<12} {n:>4} measured  {bad:>3} outside" + ("  -> caps the dimension at 2" if bad else ""))
    shown = data["metrics"] if show_all else outside
    for m in shown[: None if show_all else limit]:
        lo, hi = m["range"]
        lines.append(f"  {'ok ' if m['ok'] else 'OUT'} {m['dimension']}: {m['name']} = {m['value']:g} "
                     f"(range {lo:g}–{hi:g}) — {m['where'][:160]}")
    if not show_all and len(outside) > limit:
        lines.append(f"  … {len(outside) - limit} more outside: --every, or {PROXIES.relative_to(ROOT)}/{data['judge']}.json")
    return lines


def self_test() -> int:
    failures = []
    checks[0] = 8

    def check(ok: bool, what: str):
        if not ok:
            failures.append(what)

    good = {"judge": "ui-designer", "at": "2026-11-23T00:00:00Z", "metrics": [
        {"dimension": "hierarchy", "name": "type sizes on the view", "value": 2, "range": [1, 3], "ok": True,
         "where": "home"},
        {"dimension": "spacing", "name": "spacing values off the token scale", "value": 1, "range": [0, 0], "ok": False,
         "where": "kit p9: 28 px"},
    ]}
    check(problems(good) == [], "a well-formed file keeps the contract")
    check(problems({"judge": "x", "metrics": [{"dimension": "d"}]}) != [], "a file missing keys breaks it")
    lines = summarise(good, show_all=False)
    check(lines[0].startswith("ui-designer: 2 metrics, 1 outside"), "the head line counts metrics and outside ones")
    check(any("spacing" in line and "caps" in line for line in lines), "a dimension with one outside is capped")
    check(not any("type sizes" in line for line in lines), "a metric in its range is not listed without --all")
    check(any("type sizes" in line for line in summarise(good, show_all=True)), "--every lists every metric")
    with tempfile.TemporaryDirectory() as tmp:
        p = Path(tmp) / "x.json"
        p.write_text(json.dumps(good))
        check(load(p) == good, "a proxy file loads")
        p.write_text("not json")
        check(load(p) is None, "a broken file loads as nothing")
    scope_checks(check)
    for f in failures:
        print(f"FAIL  {f}")
    print(f"screen_audit self-test: {checks[0] - len(failures)} of {checks[0]} checks pass")
    return 1 if failures else 0


checks = [0]


def scope_checks(check) -> None:
    """Which screens a change touches, on sources of the test's own, and the real hw_test.cpp holds every root."""
    hw = """namespace hw_test {
namespace {
lv_obj_t *s_label = nullptr;
constexpr Entry kRows[] = {
    {"One", "first"},
};
void Shared(lv_obj_t *s) { lv_obj_clean(s); }
// a comment
void CreateDisplay(lv_obj_t *s) { Shared(s); widgets::MakeButton(s, "Go"); }
void CreateKit(lv_obj_t *s) { Shared(s); widgets::MakeChip(s, "x"); s_label = nullptr; }
std::unique_ptr<router::Screen> DisplayScreen() { return TestScreen(CreateDisplay); }
std::unique_ptr<router::Screen> KitScreen() { return TestScreen(CreateKit); }
void BuildHome(lv_obj_t *home) { for (const Entry &e : kRows) {} }
}  // namespace
}  // namespace hw_test
"""
    button = "namespace widgets {\nlv_obj_t *MakeButton(lv_obj_t *p, const char *t)\n{\n    return Face(p, t);\n}\n}\n"
    chip = "namespace widgets {\nlv_obj_t *MakeChip(lv_obj_t *p, const char *t)\n{\n    return nullptr;\n}\n}\n"
    header = "namespace widgets {\nlv_obj_t *MakeButton(lv_obj_t *, const char *);\nlv_obj_t *MakeChip(lv_obj_t *, const char *);\n}\n"
    src = {HW_TEST: hw, "components/widgets/src/button.cpp": button, "components/widgets/src/chip.cpp": chip,
           "components/widgets/include/widgets/button.h": header}
    line = lambda text, needle: {next(k for k, ln in enumerate(text.split("\n"), 1) if needle in ln)}  # noqa: E731
    cases = [
        ("a change to one screen's create touches that screen", {HW_TEST: line(hw, "void CreateDisplay")}, {"display"}),
        ("a widget change touches every screen that uses it", {"components/widgets/src/button.cpp": {4}}, {"display"}),
        ("another widget, the screens that use it", {"components/widgets/src/chip.cpp": {4}},
         {"kit", "keyboard", "dialog", "select sheet"}),
        ("a shared helper touches each screen that calls it", {HW_TEST: line(hw, "void Shared")},
         {"display", "kit", "keyboard", "dialog", "select sheet"}),
        ("a static a screen reads touches it", {HW_TEST: line(hw, "s_label = nullptr;")},
         {"kit", "keyboard", "dialog", "select sheet"}),
        ("a row in an initializer touches what reads the array", {HW_TEST: line(hw, '"One"')}, {"home"}),
        ("a comment touches no screen", {HW_TEST: line(hw, "// a comment")}, set()),
        ("the foundation touches every screen", {"components/router/router.cpp": {1}}, set(SCREENS)),
        ("home's own file touches home", {"components/app_shell/screen_home.cpp": {3}}, {"home"}),
        ("no UI source, no screen", {".agents/skills/x/SKILL.md": {1}, "docs/A.md": None}, set()),
    ]
    for name, changes, want in cases:
        checks[0] += 1
        got, _ = touched_screens(src, changes)
        check(got == want, f"{name}: {sorted(got)}, want {sorted(want)}")
    m = lambda *screens: {"screens": list(screens)}  # noqa: E731
    checks[0] += 3
    check(in_scope(m("kit"), {"kit"}) and not in_scope(m("about"), {"kit"}), "a metric is kept for its screens only")
    check(in_scope(m("hw.unmapped"), {"kit"}) and not in_scope(m("kit"), set()), "an unmapped screen goes with any "
          "UI change; nothing touched keeps nothing")
    # The base: a place is matched by its key — a kit page renumbered is the same place; new words are a new one.
    off = {"dimension": "spacing", "name": "spacing values off the token scale", "value": 1, "range": [0, 0],
           "ok": False, "where": "kit p10: 28 px between blocks, above at 'Brightness'"}
    edges = {"dimension": "balance", "name": "left edges", "value": 5, "range": [1, 3], "ok": False,
             "where": "kit p18: 24 px 'UI kit'; 39 px 'Status'"}
    base = {"commit": "abc",
            "metrics": [off, edges, dict(edges, value=4, where="kit p18: 2 px 'UI kit'; 3 px 'Status'")]}
    data = {"metrics": [dict(off, where="kit p11: 28 px between blocks, above at 'Brightness'"), dict(edges),
                        dict(edges, where="kit p19: 9 px 'UI kit'; 8 px 'Status'"),
                        dict(off, where="kit p12: 20 px between blocks, above at 'Contrast'")]}
    got = [m["base"] for m in with_base(data, base)["metrics"]]
    checks[0] += 3
    check(got == [1, 5, 4, None], f"each metric's base is the value at its place, else the farthest out left at its "
          f"key, none at a new place: {got}")
    twice = {"metrics": [dict(off, where="kit p11: 28 px between blocks, above at 'Brightness'"),
                         dict(off, where="kit p14: 28 px between blocks, above at 'Brightness'")]}
    got = [m["base"] for m in with_base(twice, {"metrics": [off]})["metrics"]]
    check(got == [1, None], f"a second violation at a place the base held once is new, not carried: {got}")
    check(with_base({"metrics": []}, base)["base_commit"] == "abc", "the results name the base they were held against")
    real = (ROOT / HW_TEST).read_text(errors="replace") if (ROOT / HW_TEST).is_file() else ""
    defined = {n.split("::")[-1] for n, *_ in code_proxies.functions(real)}
    missing = sorted({r for spec in SCREENS.values() for r in spec["roots"]} - defined)
    check(not real or not missing, f"every screen's root is a function in {HW_TEST}: missing {missing}")


def main(argv: list) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--all", action="store_true", help="every screen: the whole-product audit, not the change")
    parser.add_argument("--no-build", action="store_true", help="summarise the last run's JSON only")
    parser.add_argument("--every", action="store_true", help="list every metric, not only those outside their range")
    parser.add_argument("--no-base", action="store_true", help="leave out the base's values: every metric out of "
                                                               "range then caps")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    if args.self_test:
        return self_test()
    screens, why = set(SCREENS), "every screen (--all)"
    if not args.all:
        screens, why = touched_screens(ui_sources(), change_against_head())
        print(f"scope: the change — {why}; screens: {', '.join(sorted(screens)) or 'none'}")
        if not screens:
            at = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
            for judge in JUDGES:
                write(judge, {"judge": judge, "at": at, "scope": "change", "screens": [], "in_scope": [], "why": why,
                              "scope_what": "the screens whose sources (screens, widgets, tokens, routes) the change "
                                            "touched", "metrics": []})
            print("no UI cap applies: nothing measured, nothing built")
            return 0
    if not args.no_build:
        t0 = time.time()
        code, out = build_and_run()
        if code != 0:
            print(f"screen audit: the host build, its fixture or its run failed (exit {code}):\n{out[-3000:]}")
            return 1
        print(f"screen audit ran in {time.time() - t0:.0f} s; PNGs of each view in build/host/audit/, of each ringed stop in rings/")
    base: dict = {}
    if not args.all and not args.no_base:
        base, how = base_results(git("rev-parse", "HEAD").strip())
        print(f"base: {how}" + ("" if base else " — no base values: each metric out of range caps as before"))
    status = 0
    for judge in JUDGES:
        data = load(PROXIES / f"{judge}.json")
        if data is None:
            print(f"{judge}: no {PROXIES.relative_to(ROOT)}/{judge}.json — run without --no-build")
            status = 1
            continue
        bad = problems(data)
        if bad:
            print(f"{judge}: the file breaks the proxy contract: {'; '.join(bad[:5])}")
            status = 1
            continue
        if not args.all:
            data = scope_to_change(data, screens, why)
            data = with_base(data, base[judge]) if base.get(judge) else data
            write(judge, data)
        print("\n".join(summarise(data, args.every)))
    return status


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
