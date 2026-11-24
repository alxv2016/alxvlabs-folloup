#!/usr/bin/env python3
"""DESIGN.md's front matter and the token header hold the same tokens with the same values, both ways
(STICKY_DESIGN_TOKENS §6, §7).

    python3 tests/host/token_parity.py           # DESIGN.md against components/design_tokens/include/design_tokens.h
    python3 tests/host/token_parity.py --test    # the check's own cases, on mutated copies of the real files

Every token in the front matter's colors, typography, rounded, spacing, elevation, motion and sound has the symbol the
header's naming rule gives it, holding the same value once `{refs}` are resolved; every `inline constexpr` in the header
is one of those tokens or is in HEADER_ONLY below. Prints one line per mismatch, with the token's path and DESIGN.md
line, both values, and the header's symbol and line; any mismatch exits 1. Python 3.9+, the standard library only.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DESIGN = ROOT / "DESIGN.md"
HEADER = ROOT / "components/design_tokens/include/design_tokens.h"

# A colour in the header is its level: the four panel greys 0 to 3, and the two sparse dithers 4 and 5 — no panel level,
# a fill only (STICKY_DESIGN_TOKENS §2.1).
LEVELS = {"#000000": 0, "#555555": 1, "#AAAAAA": 2, "#FFFFFF": 3, "#F8F8F8": 4, "#F0F0F0": 5}

# The front matter's keys that are no token in the header: the brief's own metadata, and the components, which are
# compositions of token refs that widgets build — their refs land on tokens this check compares.
FRONT_ONLY = ("version", "name", "description", "components")

# The header's symbols that are no token in the front matter, each by design; a symbol in a namespace by its qualified
# name. An entry that the header no longer declares, or that a front-matter token now names, is stale and fails.
HEADER_ONLY = {
    "kNoShadow": "the elevation struct's value for a level with no offset; elevation.0 gives none",
    "kFocusRingReach": "derived from two spacing tokens, for the static_asserts",
    "detail::kScale": "the list of primitives the type-grid static_assert walks",
    "kTouchSwipeDistance": "touch distances are STICKY_DESIGN_TOKENS §10's, proposed and not yet DESIGN tokens",
    "kTouchFlickDistance": "as kTouchSwipeDistance",
    "kTouchFlickMovement": "as kTouchSwipeDistance",
    "kTouchFlickWindowMs": "as kTouchSwipeDistance",
    "kRefreshPartialLimit": "the refresh policy, STICKY_DESIGN_TOKENS §5: the firmware's, not a design value",
    "kFeedbackTickMs": "as kRefreshPartialLimit",
}
# The refresh policy's names for three motion delays (STICKY_DESIGN_TOKENS §5): no token of their own, so each must hold
# the value of the token symbol it names.
ALIASES = {
    "kWaitIndicatorDelayMs": "kMotionWaitIndicatorDelayMs",
    "kSelfUpdateMinMs": "kMotionSelfUpdateDelayMs",
    "kClockTickMs": "kMotionClockTickDelayMs",
}

# A token group's field and the header struct's field it is stored in. A type token's fontFamily has no field: the one
# family, Bitcount Single, is the fonts ui compiles (STICKY_DESIGN_TOKENS §3).
TYPE_FIELDS = {"fontSize": "size_px", "lineHeight": "line_px", "fontWeight": "weight"}
ELEVATION_FIELDS = {"outline": "outline_px", "shadow": "shadow", "offset": "offset_px", "keyline": "keyline_px"}
NOTE_FIELDS = {"wave": "wave", "hz": "hz", "hzEnd": "hz_end", "ms": "ms", "volume": "volume"}
# Nothing animates (DESIGN → Motion): a motion token is one symbol holding its delay, the time the code waits.
STILL = "steps(1, start)"

REF = re.compile(r"^\{([A-Za-z0-9.\-]+)\}$")
NUMBER = re.compile(r"^(-?\d+)([A-Za-z%]*)$")


class Leaf:
    """A front-matter scalar and the DESIGN.md line it is on."""

    def __init__(self, value: str, line: int):
        self.value, self.line = value, line

    def __repr__(self) -> str:
        return repr(self.value)


# --- DESIGN.md's front matter: the YAML subset it is written in ----------------------------------------------------

def strip_comment(line: str) -> str:
    """The line without its `# comment`; a `#` inside double quotes (a hex) is kept."""
    quoted = False
    for i, ch in enumerate(line):
        if ch == '"':
            quoted = not quoted
        elif ch == "#" and not quoted and (i == 0 or line[i - 1] in " \t"):
            return line[:i]
    return line


def unquote(value: str) -> str:
    value = value.strip()
    return value[1:-1] if len(value) >= 2 and value[0] == value[-1] and value[0] in "\"'" else value


def parse_flow(body: str, line: int) -> dict:
    """A flow map on one line: `{wave: pulse-50, hz: 4000}`."""
    inner = body.strip()
    if not (inner.startswith("{") and inner.endswith("}")):
        raise ValueError(f"DESIGN.md:{line}: a list item is not a flow map: {body!r}")
    out = {}
    for part in inner[1:-1].split(","):
        key, _, value = part.partition(":")
        out[key.strip()] = Leaf(unquote(value), line)
    return out


def parse_front(text: str) -> dict:
    """The front matter between the first two `---` lines, as nested dicts and lists of Leafs."""
    lines = text.splitlines()
    if not lines or lines[0].strip() != "---":
        raise ValueError("DESIGN.md: no front matter — the file does not start with ---")
    root: dict = {}
    stack: list = [(-1, root)]  # (the indent of the key that owns it, the container)
    pending = None  # (indent, parent, key) of a key with no value yet: a map or a list follows
    for no, raw in enumerate(lines[1:], start=2):
        if raw.strip() == "---":
            return root
        body = strip_comment(raw).rstrip()
        if not body.strip():
            continue
        indent = len(body) - len(body.lstrip())
        body = body.strip()
        if pending is not None:
            p_indent, parent, key = pending
            pending = None
            if indent > p_indent:
                parent[key] = [] if body.startswith("- ") else {}
                stack.append((p_indent, parent[key]))
        while stack[-1][0] >= indent:
            stack.pop()
        container = stack[-1][1]
        if body.startswith("- "):
            if not isinstance(container, list):
                raise ValueError(f"DESIGN.md:{no}: a list item outside a list")
            container.append(parse_flow(body[2:], no))
            continue
        key, sep, value = body.partition(":")
        if not sep or not isinstance(container, dict):
            raise ValueError(f"DESIGN.md:{no}: not a `key: value` line: {raw!r}")
        key = unquote(key)
        if value.strip():
            container[key] = Leaf(unquote(value), no)
        else:
            container[key] = {}
            pending = (indent, container, key)
    raise ValueError("DESIGN.md: the front matter has no closing ---")


def lookup(tree: dict, path: str):
    node = tree
    for part in path.split("."):
        if not isinstance(node, dict) or part not in node:
            return None
        node = node[part]
    return node


def resolve(tree: dict, node, seen: tuple = ()):
    """A node with every `{ref}` followed to what it names — a Leaf's value, or a group with its own refs resolved."""
    if isinstance(node, Leaf):
        m = REF.match(node.value)
        if not m:
            return node.value
        path = m.group(1)
        if path in seen:
            raise ValueError(f"DESIGN.md:{node.line}: a ref loops through {path}")
        target = lookup(tree, path)
        if target is None:
            raise ValueError(f"DESIGN.md:{node.line}: {{{path}}} names no token")
        return resolve(tree, target, seen + (path,))
    if isinstance(node, dict):
        return {k: resolve(tree, v, seen) for k, v in node.items()}
    if isinstance(node, list):
        return [resolve(tree, v, seen) for v in node]
    return node


def first_line(node) -> int:
    if isinstance(node, Leaf):
        return node.line
    if isinstance(node, dict):
        return min((first_line(v) for v in node.values()), default=0)
    if isinstance(node, list):
        return min((first_line(v) for v in node), default=0)
    return 0


# --- The token header: its `inline constexpr` values ---------------------------------------------------------------

TOKEN = re.compile(r"\s*(0[xX][0-9a-fA-F]+|\d+|[A-Za-z_][\w]*(?:::[A-Za-z_]\w*)*(?:\.[A-Za-z_]\w*)?|[{}(),+\-*/])")
DECL = re.compile(r"inline constexpr\s+([\w:]+)\s+(\w+)\s*(\[\])?\s*=\s*(.*);\s*$", re.S)


def enum_word(name: str) -> str:
    """`SoundWave::kPulse50` → `pulse-50`, the front matter's spelling."""
    word = name.split("::")[-1]
    word = word[1:] if word.startswith("k") else word
    return re.sub(r"(?<=[A-Za-z])(?=\d)", "-", word).lower()


class Header:
    def __init__(self, text: str):
        self.structs: dict = {}  # name → its fields in order
        self.exprs: dict = {}  # symbol, qualified inside a namespace → (type, is_array, expression, line, comment)
        self.values: dict = {}
        for m in re.finditer(r"struct\s+(\w+)\s*\{(.*?)\};", text, re.S):
            body = re.sub(r"//[^\n]*", "", m.group(2))
            self.structs[m.group(1)] = re.findall(r"\b\w+\s+(\w+)\s*;", body)
        detail = False
        statement, start, comment = "", 0, ""
        for no, raw in enumerate(text.splitlines(), start=1):
            code, _, note = raw.partition("//")
            if re.match(r"\s*namespace detail\s*\{", raw):
                detail = True
            elif detail and re.match(r"\s*\}\s*//\s*namespace detail", raw):
                detail = False
            if not statement:
                if not code.strip().startswith("inline constexpr"):
                    continue
                start, comment = no, note.strip()
            statement += " " + code
            if code.rstrip().endswith(";"):
                m = DECL.match(statement.strip())
                if not m:
                    raise ValueError(f"design_tokens.h:{start}: cannot read {statement.strip()!r}")
                ctype, name, array, expr = m.groups()
                self.exprs[("detail::" if detail else "") + name] = (ctype, bool(array), expr, start, comment)
                statement = ""

    def line(self, name: str) -> int:
        return self.exprs[name][3]

    def comment(self, name: str) -> str:
        return self.exprs[name][4]

    def value(self, name: str):
        if name not in self.exprs:
            raise ValueError(f"design_tokens.h: {name} is not declared")
        if name not in self.values:
            ctype, array, expr, line, _ = self.exprs[name]
            tokens = TOKEN.findall(expr)
            if "".join(tokens) != re.sub(r"\s+", "", expr):
                raise ValueError(f"design_tokens.h:{line}: cannot read the expression {expr!r}")
            self.values[name] = self.typed(Parser(self, tokens, line).whole(), ctype, array)
        return self.values[name]

    def typed(self, value, ctype: str, array: bool):
        """A brace list as the struct it initialises: its fields by name."""
        if array:
            return [self.typed(v, ctype, False) for v in value]
        fields = self.structs.get(ctype)
        if fields and isinstance(value, list):
            return dict(zip(fields, value)) if len(value) == len(fields) else value
        return value


class Parser:
    """An expression of the header: integers, symbols, `sym.field`, `Enum::kValue`, `{…}` lists, + - * / ( )."""

    def __init__(self, header: Header, tokens: list, line: int):
        self.h, self.t, self.i, self.line = header, tokens, 0, line

    def peek(self):
        return self.t[self.i] if self.i < len(self.t) else None

    def take(self, want=None):
        tok = self.peek()
        if tok is None or (want is not None and tok != want):
            raise ValueError(f"design_tokens.h:{self.line}: expected {want or 'more'}, found {tok!r}")
        self.i += 1
        return tok

    def whole(self):
        v = self.expr()
        if self.peek() is not None:
            raise ValueError(f"design_tokens.h:{self.line}: left over: {self.t[self.i:]}")
        return v

    def expr(self):
        v = self.term()
        while self.peek() in ("+", "-"):
            op, rhs = self.take(), self.term()
            v = v + rhs if op == "+" else v - rhs
        return v

    def term(self):
        v = self.factor()
        while self.peek() in ("*", "/"):
            op, rhs = self.take(), self.factor()
            v = v * rhs if op == "*" else int(v / rhs)
        return v

    def factor(self):
        tok = self.take()
        if tok == "-":
            return -self.factor()
        if tok == "(":
            v = self.expr()
            self.take(")")
            return v
        if tok == "{":
            items = []
            while self.peek() != "}":
                items.append(self.expr())
                if self.peek() == ",":
                    self.take(",")
            self.take("}")
            return items
        if re.match(r"\d", tok):
            return int(tok, 0)
        if "::" in tok:
            return enum_word(tok)
        name, _, field = tok.partition(".")
        if name not in self.h.exprs:
            raise ValueError(f"design_tokens.h:{self.line}: {name} is not declared")
        v = self.h.value(name)
        if field:
            if not isinstance(v, dict) or field not in v:
                raise ValueError(f"design_tokens.h:{self.line}: {name} has no field {field}")
            return v[field]
        return v


# --- The comparison --------------------------------------------------------------------------------------------------

def pascal(path: str) -> str:
    """`text-primary` → `TextPrimary`, `stroke.focus` → `StrokeFocus`, `2xl` → `2xl`."""
    return "".join(p[:1].upper() + p[1:].lower() for p in re.split(r"[.\-]", path) if p)


def number(value, unit: str = "px"):
    """`24px` → 24, `600` → 600; None when it is not a number in that unit."""
    m = NUMBER.match(str(value))
    if not m or m.group(2) not in (unit, ""):
        return None
    return int(m.group(1))


def colour_level(value):
    return LEVELS.get(str(value).upper())


def leaves(node, prefix: str):
    """Every leaf under a group, by its dotted path."""
    if isinstance(node, dict):
        for k, v in node.items():
            yield from leaves(v, f"{prefix}.{k}")
    else:
        yield prefix, node


def expected(tree: dict, header: Header) -> tuple[list, list]:
    """(symbol, token path, DESIGN line, DESIGN value as written, the value the header must hold) for every token,
    and the problems found before any comparison."""
    rows, problems = [], []

    def want(symbol, path, node, shown, value):
        rows.append((symbol, path, first_line(node), shown, value))

    for group in tree:
        if group in FRONT_ONLY:
            continue
        node = tree[group]
        if group == "colors":
            for path, leaf in leaves(node, "colors"):
                hexv = resolve(tree, leaf)
                level = colour_level(hexv)
                if level is None:
                    problems.append(f"DESIGN.md:{leaf.line} {path} = {hexv}: not one of the panel's levels {LEVELS}")
                    continue
                want("kColor" + pascal(path[len("colors."):]), path, leaf, hexv, level)
        elif group == "typography":
            for name, role in node.items():
                path, value = f"typography.{name}", resolve(tree, role)
                fields = {TYPE_FIELDS[k]: number(v) for k, v in value.items() if k in TYPE_FIELDS}
                unknown = set(value) - set(TYPE_FIELDS) - {"fontFamily"}
                if unknown:
                    problems.append(f"DESIGN.md:{first_line(role)} {path}: no TypeRole field for {sorted(unknown)}")
                want("kTypography" + pascal(name), path, role, value, fields)
        elif group in ("rounded", "spacing"):
            for path, leaf in leaves(node, group):
                value = resolve(tree, leaf)
                want("k" + pascal(path), path, leaf, value, number(value))
        elif group == "elevation":
            for level, fields in node.items():
                path, value = f"elevation.{level}", resolve(tree, fields)
                out = {}
                for key, field in ELEVATION_FIELDS.items():
                    if key not in value:  # a level without an offset: no shadow and no distance
                        out[field] = header.value("kNoShadow") if key == "shadow" else 0
                    elif key == "shadow":
                        out[field] = colour_level(value[key])
                    else:
                        out[field] = number(value[key])
                unknown = set(value) - set(ELEVATION_FIELDS)
                if unknown:
                    problems.append(f"DESIGN.md:{first_line(fields)} {path}: no struct field for {sorted(unknown)}")
                want(f"kElevation{pascal(level)}", path, fields, value, out)
        elif group == "motion":
            for name, fields in node.items():
                path, value = f"motion.{name}", resolve(tree, fields)
                line = first_line(fields)
                if number(value.get("duration"), "ms") != 0 or value.get("timingFunction") != STILL:
                    problems.append(f"DESIGN.md:{line} {path}: duration {value.get('duration')}, timing "
                                    f"{value.get('timingFunction')} — nothing animates, so the header holds only "
                                    f"its delay; a duration needs a symbol decided first")
                names = [s for s in (f"kMotion{pascal(name)}DelayMs", f"kMotion{pascal(name)}Ms") if s in header.exprs]
                symbol = names[0] if names else f"kMotion{pascal(name)}DelayMs"
                want(symbol, f"{path}.delay", fields, value.get("delay"), number(value.get("delay"), "ms"))
        elif group == "sound":
            for name, value in node.items():
                path = f"sound.{name}"
                if isinstance(value, list):
                    notes = [{NOTE_FIELDS.get(k, k): (v.value if k == "wave" else number(v.value, ""))
                              for k, v in note.items()} for note in value]
                    want("kSound" + pascal(name), path, value, f"{len(value)} notes", notes)
                else:
                    m = NUMBER.match(value.value)
                    unit = m.group(2).capitalize() if m else ""
                    want("kSound" + pascal(name) + unit, path, value, value.value, int(m.group(1)) if m else None)
        else:
            problems.append(f"DESIGN.md:{first_line(node)} {group}: a token group this check does not know — teach "
                            f"tests/host/token_parity.py its naming rule, or add it to FRONT_ONLY with the reason")
    return rows, problems


def check(design_text: str, header_text: str) -> list[str]:
    tree = parse_front(design_text)
    header = Header(header_text)
    rows, problems = expected(tree, header)
    claimed = set()
    for symbol, path, line, shown, value in rows:
        claimed.add(symbol)
        if symbol not in header.exprs:
            problems.append(f"DESIGN.md:{line} {path} = {shown}: no {symbol} in design_tokens.h")
            continue
        got = header.value(symbol)
        hline = header.line(symbol)
        if value is None or got != value:
            problems.append(f"DESIGN.md:{line} {path} = {shown} (header value {value}) but design_tokens.h:{hline} "
                            f"{symbol} = {got}")
        if path.startswith("colors."):
            written = re.match(r"#[0-9A-Fa-f]{6}\b", header.comment(symbol))
            if written and written.group(0).upper() != str(shown).upper():
                problems.append(f"DESIGN.md:{line} {path} = {shown} but design_tokens.h:{hline} {symbol}'s comment "
                                f"says {written.group(0)}")
    for symbol in header.exprs:
        if symbol not in claimed and symbol not in HEADER_ONLY and symbol not in ALIASES:
            problems.append(f"design_tokens.h:{header.line(symbol)} {symbol} = {header.value(symbol)}: no token in "
                            f"DESIGN.md's front matter names it — add the token there first, or add it to HEADER_ONLY "
                            f"with the reason")
    for alias, target in ALIASES.items():
        if alias in header.exprs and target in header.exprs and header.value(alias) != header.value(target):
            problems.append(f"design_tokens.h:{header.line(alias)} {alias} = {header.value(alias)} but the token it "
                            f"names, {target}, is {header.value(target)} (design_tokens.h:{header.line(target)})")
        if target not in claimed:
            problems.append(f"token_parity.py: ALIASES names {alias} → {target}, which no front-matter token names")
    for symbol in list(HEADER_ONLY) + list(ALIASES):
        if symbol not in header.exprs:
            problems.append(f"token_parity.py: {symbol} is listed, but design_tokens.h no longer declares it — "
                            f"drop it from the list")
        elif symbol in claimed:
            problems.append(f"token_parity.py: {symbol} is listed as header-only, but a front-matter token now names "
                            f"it — drop it from the list")
    return problems


# --- The check's own cases -----------------------------------------------------------------------------------------

def self_test() -> int:
    design, header = DESIGN.read_text(encoding="utf-8"), HEADER.read_text(encoding="utf-8")

    def fails(d: str, h: str, *words: str) -> bool:
        try:
            found = check(d, h)
        except ValueError as e:
            found = [str(e)]
        return any(all(w in p for w in words) for p in found)

    def swap(text: str, old: str, new: str) -> str:
        assert text.count(old) == 1, f"the self-test's fixture {old!r} is not in the file exactly once"
        return text.replace(old, new)

    def promoted() -> bool:
        """kSpacingKeyGap listed as header-only while spacing.key-gap names it, as when an entry becomes a token."""
        HEADER_ONLY["kSpacingKeyGap"] = "a stale entry"
        try:
            return fails(design, header, "kSpacingKeyGap is listed", "now names it")
        finally:
            del HEADER_ONLY["kSpacingKeyGap"]

    tree = parse_front(design)
    cases = [
        (check(design, header) == [], "the real files agree"),
        (resolve(tree, lookup(tree, "colors.text-secondary")) == "#555555", "a colour ref resolves through the scale"),
        (resolve(tree, Leaf("{typography.scale-60-bold}", 0))["fontSize"] == "60px", "a ref to a type group resolves"),
        (Header(header).value("kFocusRingReach") == 8, "header arithmetic between symbols is evaluated"),
        (Header(header).value("kTypographyBody") == {"size_px": 30, "line_px": 40, "weight": 400},
         "a struct of field references is evaluated"),
        (fails(swap(design, "list-row-height: 80px", "list-row-height: 88px"), header,
               "spacing.list-row-height", "kSpacingListRowHeight"), "DESIGN.md's list-row-height at 88 alone fails"),
        (fails(design, swap(header, "kSpacingListRowHeight = 80;", "kSpacingListRowHeight = 88;"),
               "spacing.list-row-height", "= 88"), "the header's list-row-height at 88 alone fails"),
        (fails(design, swap(header, "kTypographyScale30Bold = {30, 36, 700};", "kTypographyScale30Bold = {30, 40, 700};"),
               "typography.label", "kTypographyLabel"), "a header primitive changed alone fails the role built on it"),
        (fails(swap(design, 'divider: "{colors.scale.primary.1}"', 'divider: "{colors.scale.primary.2}"'), header,
               "colors.divider", "kColorDivider"), "a colour ref moved to another level fails"),
        (fails(design, swap(header, "kColorBand = kColorScalePrimary2;     // #AAAAAA",
                            "kColorBand = kColorScalePrimary2;     // #555555"), "colors.band", "comment"),
         "a colour's stale hex comment fails"),
        (fails(design, swap(header, "inline constexpr int16_t kSpacingKeyGap = 12;",
                            "inline constexpr int16_t kSpacingKeyGap = 12;\ninline constexpr int16_t kSpacingExtra = 5;"),
               "kSpacingExtra", "no token"), "a header symbol with no token fails"),
        (fails(swap(design, "  key-gap: 12px", "  key-gap: 12px\n  extra: 5px"), header, "spacing.extra", "no kSpacingExtra"),
         "a front-matter token with no symbol fails"),
        (fails(swap(design, "  edge: \"{spacing.lg}\"", "  edge: \"{spacing.huge}\""), header, "names no token"),
         "a ref to nothing fails"),
        (fails(design, swap(header, "{SoundWave::kPulse50, 4000, 0, 30, 255}", "{SoundWave::kPulse50, 4000, 0, 35, 255}"),
               "sound.tick", "kSoundTick"), "a sound's note changed alone fails"),
        (fails(design, swap(header, "kMotionKeyRepeatPeriodMs = 500;", "kMotionKeyRepeatPeriodMs = 400;"),
               "motion.key-repeat-period.delay"), "a motion delay changed alone fails"),
        (fails(swap(design, "  wait-indicator:\n    duration: 0ms", "  wait-indicator:\n    duration: 200ms"), header,
               "motion.wait-indicator", "nothing animates"), "a motion duration with no symbol fails"),
        (fails(design, swap(header, "kElevation2 = {kSpacingStrokeStrong, kColorShadow2,",
                            "kElevation2 = {kSpacingStrokeStrong, kColorShadow3,"), "elevation.2", "kElevation2"),
         "an elevation's shadow changed alone fails"),
        (fails(design, swap(header, "kWaitIndicatorDelayMs = kMotionWaitIndicatorDelayMs;",
                            "kWaitIndicatorDelayMs = 700;"), "kWaitIndicatorDelayMs = 700", "kMotionWaitIndicatorDelayMs"),
         "an alias with a value other than its token's fails"),
        (fails(design, swap(header, "inline constexpr int16_t kFocusRingReach = kSpacingFocusOffset + kSpacingStrokeFocus;",
                            "inline constexpr int16_t kFocusRingReachGone = kSpacingFocusOffset + kSpacingStrokeFocus;"
                            ).replace("kSpacingItem >= 2 * kFocusRingReach", "kSpacingItem >= 2 * kFocusRingReachGone"),
               "kFocusRingReach is listed", "no longer declares"), "a header-only entry the header dropped fails"),
        (promoted(), "a header-only entry a front-matter token now names fails"),
        (fails(design.replace("\nsound:", "\nrefresh:\n  partial-limit: 20\nsound:", 1), header, "refresh",
               "does not know"), "an unknown token group fails"),
        (fails(swap(design, "  scale-20-bold:\n    fontFamily: Bitcount Single",
                    "  scale-20-bold:\n    letterSpacing: 1px\n    fontFamily: Bitcount Single"), header,
               "typography.scale-20-bold", "letterSpacing"), "a type token with a key the struct lacks fails"),
        (fails(design, swap(header, "inline constexpr uint8_t kNoShadow = 0xFF;\n", ""), "kNoShadow", "not declared"),
         "a deleted kNoShadow fails as a mismatch, not a crash"),
    ]
    failed = [name for ok, name in cases if not ok]
    for name in failed:
        print(f"    FAIL — {name}")
    print(f"token parity self-test — {len(cases) - len(failed)} of {len(cases)} cases pass")
    return 1 if failed else 0


def main(argv: list[str]) -> int:
    if "--test" in argv:
        return self_test()
    try:
        problems = check(DESIGN.read_text(encoding="utf-8"), HEADER.read_text(encoding="utf-8"))
    except ValueError as e:
        problems = [str(e)]
    for p in problems:
        print(p)
    print(f"token parity — DESIGN.md's front matter against design_tokens.h: {len(problems)} mismatch(es)")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
