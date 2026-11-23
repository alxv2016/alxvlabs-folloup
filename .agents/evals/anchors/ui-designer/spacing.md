# ui-designer — Spacing rhythm, grouping

What scores of 0, 2 and 4 look like on this dimension, for the `ui-designer` judge. The rhythm of space: what is glued together, what is set apart, and whether every value comes from the scale. Its weight is in `.agents/evals/weights.json`; how it is scored is the judge's rubric, Quality (.agents/HARNESS.md §12).

**The rules it is held to: [LAYOUT §7.1](../../../skills/layout/references/layout.md#71-the-ladder)** — pair gap < inset ≤ group gap < section gap — with the scale (§7.3) and no one-off values (§7.5).

**Measured proxies — built.** The screen audit, `screen_audit.py`: a pair's gap against the gap round its group (at most 1 : 2, LAYOUT §7.1), a lone label nearer one of its neighbours (LAYOUT §7.3), and padding, margins and gaps off DESIGN.md's spacing tokens (LAYOUT §7.5). `verify.py proxies --judge ui-designer` runs them; a value out of its range caps this dimension at 2.

**How to use it.** Find the level the work sits nearest, quote the evidence — a file and line, a screenshot, a measured value — and give 1 or 3 when it sits between two. The person's own examples, under From the person, outrank the ones written here.

## 4 — what good looks like

- Carbon — 16 px padding beside a 32 px gutter (1 : 2), every value a multiple of 2, 4 or 8 on spacing-01 to -13 (carbondesignsystem.com, spacing overview and 2x grid).
- Atlassian — an 8 px base; 0–8 inside small components and between repeated items, 12–24 in cards, 32–80 between sections; spacing named as gap and inset (atlassian.design/foundations/spacing).
- [ref-1-nav-groups](../img/ref-1-nav-groups.jpg) — a group's header nearer its first item than the gap between groups; one row rhythm. [ref-3-transactions](../img/ref-3-transactions.jpg) — each name 2–4 px over its category, rows 12–24 apart: proximity does the grouping.
- The ladder: pair gap < inset ≤ group gap < section gap, each a clear step (2–3x) above the one inside it, every value a token on the scale.

## 2 — acceptable, with a clear gap

- Every value on the scale, but two levels too close — a pair's gap equal to the gap between pairs — or one or two one-off values (a 12 or a 20 off a 4, 8, 16, 24 scale).

## 1 — what the person found poor

- [spacing-overlay](../img/spacing-overlay.png), FolloUp's kit measured from LVGL's layout — padding (pink), gaps between siblings (blue) and screen margins (orange); with [current-pairings](../img/current-pairings.png): a dialog's title 32 px of ink from its message where a row's label sits 8 from its caption; a section heading 35–38 px over its own rows but only 48 under the last section's text, so it barely groups with what it heads; `card-inset` 20 and `step-gap` 12 off the scale.

## 0 — what fails it

- No rhythm: values picked per screen, a group's inside looser than the space around it, nothing tells which text belongs to which control.

## From the person

- **2026-11-23, poor** (T2) — "Spacing with no visual harmony across components and layouts: the same pairing 8 to 41 px apart, one-off tokens"
- **2026-11-23, poor** (T3) — "There needs to be more margin between large display headings and body text as well as modal titles and description. Usually larger text need more margin below their description or body text than smaller text"
