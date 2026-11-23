# ui-designer — Consistency across screens

What scores of 0, 2 and 4 look like on this dimension, for the `ui-designer` judge. The same thing looks and behaves the same on every screen: rows, insets, glyphs, focus, the words for one action. Its weight is in `.agents/evals/weights.json`; how it is scored is the judge's rubric, Quality (.agents/HARNESS.md §12).

**Measured proxies — built.** The screen audit, `screen_audit.py`: a type size and weight that is no DESIGN.md role, the same pair of roles spaced alike on every view, and Back in one place on every screen. `verify.py proxies --judge ui-designer` runs them; a value out of its range caps this dimension at 2.

**How to use it.** Find the level the work sits nearest, quote the evidence — a file and line, a screenshot, a measured value — and give 1 or 3 when it sits between two. The person's own examples, under From the person, outrank the ones written here.

## 4 — what good looks like

- One row component everywhere a row appears, with one inset and one gap; one glyph for one meaning; the same action named and placed the same on every screen. Atlassian and Carbon both ship a row or list item once and reuse it — a screen never restyles it.

## 2 — acceptable, with a clear gap

- A shared component reused, but one family drifts — FolloUp's toggle, checkbox and radio button rows start their words at x 24 where its other rows start at x 40 ([spacing-overlay](../img/spacing-overlay.png), panel 2 beside panel 1).

## 0 — what fails it

- Each screen its own: a row built by hand per screen, two looks for one state, two words for one action.

## From the person

- **2026-11-23, poor** (T4) — "Don't change my checkmarks, radio, and switch components, I've asked you to fix typography and spacing not make changes to ui components"
