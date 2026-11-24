#!/usr/bin/env python3
"""The ux-content-writer judge's measured proxies: the UI strings ux_lint.py reads, measured against the room their
element has, and the share of them its rules flag. A proxy outside its range caps its quality dimension at 2; it never
fails a run.

    python3 .agents/skills/ux-content/scripts/ux_proxies.py          # the change: the strings on its changed lines
    python3 .agents/skills/ux-content/scripts/ux_proxies.py --all    # every UI file whole: the whole-product audit
    python3 .agents/skills/ux-content/scripts/ux_proxies.py FILE …   # these files, whole
    python3 .agents/skills/ux-content/scripts/ux_proxies.py --self-test

A review scores the change, an audit the product: bare, only the strings on the lines the diff against HEAD wrote in
the changed UI files are read (a new file: all of them); a string the change never touched never caps its review.
Measures, by dimension (the keys of .agents/evals/weights.json):
  short      strings longer than their element's room, in characters of the element's type — none. The room is the
             project's own arithmetic (STICKY_DESIGN_TOKENS §4.2, §9, §11.4; DESIGN → Components): a half or standard
             button 10 characters of `label`, 7 beside a glyph; a full-width or compact button 22; a home row's name 22 and
             its line 33; a Wi-Fi network's name 28; a text field 19; a status bar's title 36 of `caption`; a dialog's and
             the keyboard's buttons 10 and 6. A string whose element is not in ROOMS wraps or is not measured.
             Also the share of strings with a U8 or U10 hit (three full stops, "successfully")
  clear      the share of strings with a hit of U1, U5, U9, U11 or U13 (a generic button, "invalid", "e.g.", "click here",
             "enable") — none
  human      the share with U2, U3, U4, U7, U12 or U14 ("please", "!", "oops", title case, "the user", capitals) — none
  next-step  the share with U6 ("Error" as a title) — none
The rules are ux_lint.py's (UX_CONTENT §n there); the strings are those its reader finds with the project's calls, and for
the room also the hardware test's HalfButton and the config fields `title`, `action` and `cancel`. Writes
.cache/harness/proxies/ux-content-writer.json — {"judge", "at", "scope": "change" | "product" | "files", "metrics":
[{"dimension", "name", "value", "range": [lo, hi], "ok", "where", "base"?, "offenders"?, "base_offenders"?}]} — and
prints a summary. In the change's scope each metric names its `offenders`, "file: string", beside HEAD's in the same
files whole (`base_offenders`), so verify.py caps only a string the change made worse; a string moved within its file
is carried debt (.agents/HARNESS.md §12.5). Python 3.9+, standard library only.
"""

from __future__ import annotations

import argparse
import importlib.util
import json
import re
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("ux_lint", HERE / "ux_lint.py")
ux_lint = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ux_lint)
ROOT = ux_lint.ROOT
PROXIES = ROOT / ".cache/harness/proxies"
JUDGE = "ux-content-writer"

DIMENSION_OF = {"U1": "clear", "U5": "clear", "U9": "clear", "U11": "clear", "U13": "clear",
                "U2": "human", "U3": "human", "U4": "human", "U7": "human", "U12": "human", "U14": "human",
                "U8": "short", "U10": "short", "U6": "next-step"}
ROOM_CALLS = (r"\bHalfButton", r"\.(?:title|action|cancel)\s*=")


def room(call: str, arg: int, context: str) -> tuple | None:
    """(characters, the element) for the string at argument `arg` of `call`; `context` is the call's text, or for an
    assignment the statements before it. None when the element wraps or is not measured."""
    glyph = "Glyph::" in context
    if call.endswith("HalfButton") and arg == 1:
        return 10, "a half button"
    if call.endswith("MakeButton") and arg == 1:
        if glyph:
            return 7, "a button beside its glyph"
        if "kSpacingButtonWidth" in context or "kSpacingContent - dt::kSpacingMd) / 2" in context:
            return 10, "a standard button"
        return 22, "a compact button (at most the content's width)"
    if call.endswith("NewStatusBar") and arg == 1:
        return 36, "the status bar's title"
    if call.endswith("MakeListRow"):
        return {1: (22, "a home row's name"), 2: (33, "a home row's line")}.get(arg)
    if call.endswith("MakeNetworkRow") and arg == 1:
        return 28, "a Wi-Fi network's name"
    if call.endswith(("MakeTextInput", "MakePasswordInput")) and arg == 1:
        return 19, "a text field"
    if call.endswith("MakeDialog") and arg in (3, 4):
        return 10, "a dialog's button"
    if call.startswith("."):  # a config's field
        field = call[1:]
        keyboard = context.rfind("KeyboardConfig") > context.rfind("DialogConfig")
        if field in ("action", "cancel"):
            return (6, "the keyboard's footer button") if keyboard else (10, "a dialog's button")
        if field == "title" and keyboard:
            return 36, "the keyboard's status bar title"
    return None


def enclosing(code: str, tokens: list, pos: int) -> tuple:
    """(the call's name, the argument's index, the call's text) for the literal at `pos`; for a literal assigned to a
    field, (".field", 0, the text before it)."""
    i = next((k for k, t in enumerate(tokens) if t[0] == pos), None)
    depth, commas = 0, 0
    for k in range(i - 1 if i is not None else -1, -1, -1):
        start, end, typ, content = tokens[k]
        if typ == "b" and content in ")]}":
            depth += 1
        elif typ == "b" and content in "([{":
            if depth == 0:
                if content != "(":
                    break
                name = re.search(r"([\w:]+)\s*$", code[:start])
                close = next((t[0] for t in tokens[i:] if t[2] == "b" and t[3] == ")"), pos)
                return (name.group(1) if name else "", commas, code[start:close + 1])
            depth -= 1
        elif typ == "p" and depth == 0:
            commas += 1
    stmt = code[max(code.rfind(";", 0, pos), code.rfind("{", 0, pos)) + 1:pos]
    field = re.search(r"\.(\w+)\s*=", stmt)
    if field:
        return "." + field.group(1), 0, code[max(0, pos - 1200):pos]
    return "", 0, ""


def git(*args: str) -> str:
    return subprocess.run(["git", *args], cwd=ROOT, capture_output=True, text=True, errors="replace").stdout


def changed_lines(path: str) -> set | None:
    """The file's lines the change wrote, numbered in the new file; None when the file is new (all of it)."""
    if git("ls-files", "--", path).strip() == "":
        return None
    lines: set = set()
    for m in re.finditer(r"^@@ -\d+(?:,\d+)? \+(\d+)(?:,(\d+))? @@", git("diff", "-U0", "HEAD", "--", path), re.M):
        start, count = int(m.group(1)), int(m.group(2) if m.group(2) is not None else 1)
        lines.update(range(start, start + count))
    return lines


def measure(files: list, lines_of: dict | None = None, keys: dict | None = None, texts: dict | None = None) -> tuple:
    """(strings read, hits by dimension, strings over their room, strings in sized elements); with `lines_of`
    {file: lines or None}, only the strings on those lines of each file (None: every line). `keys`, when given, is
    filled with the places out of range — {"room" or a dimension: {"file: string": how bad}} — and `texts` {file: text}
    reads a file's text from there (HEAD's) rather than from the tree."""
    settings = ux_lint.load_settings()
    ux = settings[0] if settings else {}
    names = ux_lint.as_tuple(ux.get("names"))
    calls = ux_lint.as_tuple(ux.get("calls")) or ux_lint.DEFAULT_CALLS
    cfg = {"calls": calls, "names": names, "case": ux.get("case", "sentence")}
    strings, by_dim, over, measured = 0, {d: [] for d in set(DIMENSION_OF.values())}, [], 0
    keys = keys if keys is not None else {}
    for f in files:
        try:
            text = texts[f] if texts is not None else ux_lint.decode((ROOT / f).read_bytes())
        except (OSError, KeyError):
            continue
        newlines = [m.start() for m in re.finditer("\n", text)]
        wanted = (lines_of or {}).get(f)
        keep = (lambda pos: True) if wanted is None else \
            (lambda pos: ux_lint.bisect.bisect_left(newlines, pos) + 1 in wanted)
        excepted = {ux_lint.bisect.bisect_left(newlines, m.start()) for m in re.finditer("ux-exception", text)}
        # The rules, as ux_lint runs them.
        seen: dict = {}  # each string's occurrence in the whole file, untouched lines too, as HEAD's whole file counts

        def nth(kind: str, piece: str) -> str:
            seen[(kind, piece)] = seen.get((kind, piece), 0) + 1
            return f"{f}: {piece}" + (f" #{seen[(kind, piece)]}" if seen[(kind, piece)] > 1 else "")

        for pos, raw in ux_lint.literals(f, text, calls):
            n = ux_lint.bisect.bisect_left(newlines, pos)
            if n in excepted:
                continue
            for piece in ux_lint.pieces(raw):
                place = nth("rule", piece)
                if not keep(pos):
                    continue
                strings += 1
                dims = {DIMENSION_OF[r] for r in ux_lint.check(piece, cfg)}
                for d in dims:
                    by_dim[d].append(f"{f}:{n + 1} \"{piece[:50]}\"")
                    keys.setdefault(d, {})[place] = 1
        # The room, with the calls that place words in a sized element.
        lx = ux_lint.Lexer(f)
        code, tokens = lx.lex(text)
        found = dict(ux_lint.literals(f, text, tuple(calls) + ROOM_CALLS))
        for m in re.finditer(r"\.(?:title|action|cancel)\s*=", code):  # each arm of a ternary assigned to a field
            end = code.find(";", m.end())
            found.update((t[0], t[3]) for t in tokens if t[2] == "s" and m.end() <= t[0] < end)
        for pos, raw in sorted(found.items()):
            call, arg, context = enclosing(code, tokens, pos)
            fit = room(call, arg, context)
            if fit is None:
                continue
            for piece in ux_lint.pieces(raw):
                place = nth("room", piece)  # the same string twice in a file is two places
                if not keep(pos):
                    continue
                measured += 1
                if len(piece) > fit[0]:
                    over.append(f"{f}:{ux_lint.bisect.bisect_left(newlines, pos) + 1} \"{piece}\" {len(piece)} for "
                                f"{fit[1]}'s {fit[0]}")
                    keys.setdefault("room", {})[place] = len(piece) - fit[0]
    return strings, by_dim, over, measured


def with_base(metrics: list, keys: dict, head_keys: dict, head_metrics: list) -> list:
    """Each metric with its `offenders` here and `base_offenders` in HEAD's whole files — a string moved within its file
    is carried — and its `base`, the same metric over HEAD's files (null: none there)."""
    head = {m["name"]: m["value"] for m in head_metrics}
    for m in metrics:
        group = "room" if m["name"].startswith("strings longer") else m["dimension"]
        m["base"] = head.get(m["name"])
        m["offenders"] = keys.get(group, {})
        m["base_offenders"] = head_keys.get(group, {})
    return metrics


def at_head(path: str) -> str | None:
    """The file as HEAD holds it; None when HEAD does not have it."""
    proc = subprocess.run(["git", "show", f"HEAD:{path}"], cwd=ROOT, capture_output=True)
    return ux_lint.decode(proc.stdout) if proc.returncode == 0 else None


def metric(dimension: str, name: str, value: float, lo: float, hi: float, where: str) -> dict:
    return {"dimension": dimension, "name": name, "value": value, "range": [lo, hi], "ok": lo <= value <= hi,
            "where": where}


def proxies(strings: int, by_dim: dict, over: list, measured: int) -> list:
    out = []
    if measured:
        out.append(metric("short", "strings longer than their element's room", len(over), 0, 0,
                          "; ".join(over[:4]) or f"{measured} strings in sized elements, each fits"))
    if strings:
        for dim in ("clear", "short", "human", "next-step"):
            hits = by_dim.get(dim, [])
            out.append(metric(dim, f"share of strings with a {dim} hit", round(len(hits) / strings, 4), 0, 0,
                              "; ".join(hits[:3]) or f"none of {strings} strings"))
    return out


def self_test() -> int:
    failures = []

    def check(ok: bool, what: str):
        if not ok:
            failures.append(what)

    src = '''void Build(lv_obj_t *s) {
    HalfButton(row, "Calibrate the touch panel", true, OnGo);
    HalfButton(row, "Full", true, OnFull);
    widgets::MakeButton(stack, "Record", widgets::ButtonVariant::kAction, dt::kSpacingButtonWidth, widgets::Glyph::kMic);
    widgets::MakeButton(stack, "Recording now", widgets::ButtonVariant::kAction, dt::kSpacingButtonWidth, widgets::Glyph::kMic);
    widgets::MakeListRow(list, "Display", "Refresh modes, greys, ghosting", OnEntry, nullptr);
    widgets::KeyboardConfig config{};
    config.action = s_password ? "Join" : "Save changes";
    ui::NewLabel(s, Type::kBody, "Please wait!");
}
'''
    path = "components/hw_test/selftest.cpp"
    lx = ux_lint.Lexer(path)
    code, tokens = lx.lex(src)
    found = {raw: enclosing(code, tokens, pos)[:2] for pos, raw in ux_lint.literals(path, src, ux_lint.DEFAULT_CALLS + (
        r"\b(?:ui::|widgets::)?(?:Make|New)[A-Z]\w*",) + ROOM_CALLS)}
    check(found.get("Full") == ("HalfButton", 1), f"a half button's words are its second argument: {found.get('Full')}")
    check(found.get("Refresh modes, greys, ghosting") == ("widgets::MakeListRow", 2), "a row's line is its third argument")
    arm = next((t[0] for t in tokens if t[2] == "s" and t[3] == "Save changes"), -1)
    check(enclosing(code, tokens, arm)[:2] == (".action", 0), f"a ternary's arm assigned to a field: "
                                                              f"{enclosing(code, tokens, arm)[:2]}")
    import tempfile
    with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
        f = Path(tmp) / "selftest.cpp"
        f.write_text(src)
        rel = str(f.relative_to(ROOT))
        strings, by_dim, over, measured = measure([rel])
    text = " ".join(over)
    check('"Calibrate the touch panel" 25' in text, f"25 characters overflow a half button's 10: {over}")
    check('"Recording now" 13' in text and '"Record"' not in text, "beside a glyph 7 fit, 13 do not")
    check('"Save changes" 12' in text and '"Join"' not in text, "the keyboard's footer holds 6")
    check(measured == 8, f"eight strings in sized elements: {measured}")
    ms = proxies(strings, by_dim, over, measured)
    human = next((m for m in ms if m["name"] == "share of strings with a human hit"), None)
    check(human is not None and human["value"] > 0 and "Please wait" in human["where"], f"'Please wait!' is human's: {human}")
    # The change's scope: only the strings on the changed lines — here the line with "Record" alone.
    with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
        f = Path(tmp) / "selftest.cpp"
        f.write_text(src)
        rel = str(f.relative_to(ROOT))
        line = next(k for k, ln in enumerate(src.split("\n"), 1) if '"Record"' in ln)
        strings, by_dim, over, measured = measure([rel], {rel: {line}})
    check(strings == 1 and measured == 1 and not over, f"a change to one line reads its one string: {strings}, "
          f"{measured}, {over}")
    check(not any("Please wait" in h for hits in by_dim.values() for h in hits), "the untouched 'Please wait!' caps "
          "nothing in the change's scope")
    # The base: a string over its room at HEAD too is the same place; one the change wrote is new.
    keys, head_keys = {}, {}
    now = proxies(*measure([path], None, keys, {path: src}))
    old = src.replace('"Recording now"', '"Rec"')
    based = with_base(now, keys, head_keys, proxies(*measure([path], None, head_keys, {path: old})))
    room = next((m for m in based if m["name"].startswith("strings longer")), {})
    fresh = set(room.get("offenders", {})) - set(room.get("base_offenders", {}))
    check(fresh == {f"{path}: Recording now"}, f"only the string the change lengthened is new: {sorted(fresh)}")
    check(room.get("base") == 2, f"the base counts HEAD's strings over their room: {room.get('base')}")
    keys = {}
    twice = src.replace('HalfButton(row, "Full"', 'HalfButton(row, "Calibrate the touch panel"')
    measure([path], None, keys, {path: twice})
    check(sorted(k for k in keys.get("room", {}) if "Calibrate" in k) == [f"{path}: Calibrate the touch panel",
                                                                         f"{path}: Calibrate the touch panel #2"],
          f"the same string twice is two places: {sorted(keys.get('room', {}))}")
    # HEAD holds the string once; the change adds a copy on another line, and only that line is measured: still new.
    copy_line = next(k for k, ln in enumerate(twice.split("\n"), 1) if "Calibrate" in ln and "OnFull" in ln)
    keys, head_keys = {}, {}
    measure([path], {path: {copy_line}}, keys, {path: twice})
    measure([path], None, head_keys, {path: src})
    fresh = set(keys.get("room", {})) - set(head_keys.get("room", {}))
    check(fresh == {f"{path}: Calibrate the touch panel #2"}, f"a second copy of an untouched string is new: "
          f"{sorted(fresh)}")
    for f in failures:
        print(f"FAIL  {f}")
    total = 14
    print(f"ux_proxies self-test: {total - len(failures)} of {total} checks pass")
    return 1 if failures else 0


def main(argv: list) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("files", nargs="*", help="files to read (default: the UI files changed against HEAD)")
    parser.add_argument("--all", action="store_true", help="every UI file")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    if args.self_test:
        return self_test()
    ui_files, _ = ux_lint.select(ux_lint.tracked_files(), ux_lint.load_settings())
    scope, lines_of = ("files" if args.files else "product" if args.all else "change"), None
    if args.files:
        files = args.files
    elif args.all:
        files = ui_files
    else:
        changed = set(git("diff", "--name-only", "HEAD").split()) | set(git("ls-files", "--others",
                                                                            "--exclude-standard").split())
        files = [f for f in ui_files if f in changed]
        lines_of = {f: changed_lines(f) for f in files}
    keys: dict = {}
    strings, by_dim, over, measured = measure(files, lines_of, keys)
    metrics = proxies(strings, by_dim, over, measured)
    if scope == "change":  # the base: the same files as HEAD holds them, whole
        heads = {f: t for f in files if (t := at_head(f)) is not None}
        head_keys: dict = {}
        head = measure(sorted(heads), None, head_keys, heads)
        metrics = with_base(metrics, keys, head_keys, proxies(*head))
    # What was measured, file by file: empty, the change wrote no string a person reads.
    in_scope = [f for f in files if sum(measure([f], lines_of)[0::3])] if strings or measured else []
    PROXIES.mkdir(parents=True, exist_ok=True)
    path = PROXIES / f"{JUDGE}.json"
    at = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    path.write_text(json.dumps({"judge": JUDGE, "at": at, "scope": scope, "scope_what": "the UI strings on the lines "
                                "the change wrote", "in_scope": in_scope, "metrics": metrics}, indent=1,
                               ensure_ascii=False) + "\n")
    outside = [m for m in metrics if not m["ok"]]
    print(f"{JUDGE} ({scope}): {len(files)} UI files, {strings} strings, {measured} in sized elements; {len(metrics)} metrics, "
          f"{len(outside)} outside their range — {path.relative_to(ROOT)}"
          + ("" if files else " (no changed UI file; --all reads every one)"))
    for m in metrics:
        lo, hi = m["range"]
        print(f"  {'ok ' if m['ok'] else 'OUT'} {m['dimension']}: {m['name']} = {m['value']:g} (range {lo:g}–{hi:g})"
              f" — {m['where'][:170]}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
