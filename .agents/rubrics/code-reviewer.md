# Rubric — code-reviewer

How the [code-reviewer](../agents/code-reviewer.md) scores a diff to the code: the verdict, the evidence each result
needs, and the items. R-items are its Procedure steps, each resting on the project's own rules — AGENTS.md, the docs'
Do and Never lists, the skills the touched files call for. L-items were learned from what the person found on the
target, recorded with `verify.py finding --route rubric`
([verify-change](../skills/verify-change/SKILL.md#recording-what-was-learned) → Recording what was learned). A new item
goes here, not in the agent.

## Verdict

- **blocking** — an item fails on an added or changed line against a doc § or a skill rule, or the change does not do
  what it claims.
- **pass** — every item passes, is n/a or unsure; should-fix findings and notes do not block.

A decision in `verify.py status` is the person's word for this task: the judge notes it and never blocks on it.

## Evidence

- **fail** — the file and line, the line quoted, and the rule (doc § or skill rule).
- **pass** — what was read or run: the files and lines checked, or the command and its result.
- **unsure** — what could not be settled, and what would settle it: a test, the target, the person.
- **n/a** — why the item does not touch this diff.

A result without evidence counts as unsure.

## Items

| Item | Looks for | Procedure step |
| --- | --- | --- |
| R1 | Correctness: the change does what it claims — edge cases (empty, missing, slow, failed), async order, state that outlives its screen | 1 |
| R2 | The project's rules: each Do and Never in the docs and skills the touched files call for; layering and imports in the allowed directions; AGENTS.md's conventions | 2 |
| R3 | Coupled changes: a setting, schema, token or API changed with its callers, types, migrations, tests and the other platforms | 3 |
| R4 | What must never ship: a `DEBUG_MARKERS` line, a switch off its shipping value (`INVARIANTS`), a secret in code or a log line (`.agents/project.py`; AGENTS.md → Safety) | 4 |
| R5 | Errors and the person's data: failures handled and shown, nothing the user owns lost or sent where they did not ask | 5 |
| R6 | Tests: new behaviour tested where the project tests that kind of thing; a fixed bug with a test that fails without the fix | 6 |
| R7 | Docs in the same change: no doc outside the diff still states the old behaviour or value (GUIDELINES §7) | 7 |
| R8 | Comments: local mechanics explained in place, briefly; a decision, a value's provenance or a rule shared across files cited as `DOC §n` — the comment may carry the argument too, never instead of the citation; no `§` without its doc name, no line number (GUIDELINES §4.8) | 8 |

## Learned

| Item | Since | From the person | Looks for |
| --- | --- | --- | --- |
| L1 | 2026-11-19 | The V1.0 froze 8 s at a time on a page | A driver step that changes a device's power state (standby, power off, sleep) sent without checking the power state the driver tracks |
| L2 | 2026-11-19 | The same freeze, repeated every 8 s | A wait on a busy line or a timeout, on the UI's task, retried on its own failure in a loop |
| L3 | 2026-11-20 | The code reviewer, before the glass: compiled fonts dropped the → and … the run-time font drew | A change to a compiled font's glyph range, or a new drawn string: every character of every string the UI draws, checked against the range the font was compiled with |
| L4 | 2026-11-20 | The code reviewer, before the battery test: a failed power-off fell back to deep sleep with the RTC countdown still running | A sequence that powers something down or leaves (sleep, power off, a fallback between them): each step's result checked, every wake source the next step does not arm cleared, and the glass promising only what was set |
| L5 | 2026-11-20 | The code reviewer, before the glass: a flick counted the whole drag, and a widget's scroll-on-focus animated on screens that had not replaced it | A gesture's time window or a widget's stock LVGL behaviour: the window taken from samples at the lift, not reset on a pause, with a host test; a stock flag that animates (scroll on focus, scroll to) replaced by the widget itself, not left to each screen; a scrollable widget that takes UP and DOWN in an edit with `LV_OBJ_FLAG_SCROLL_WITH_ARROW` removed and a host test that its scroll stays put (the roller, 2026-11-22, as `text_view` before it) |
