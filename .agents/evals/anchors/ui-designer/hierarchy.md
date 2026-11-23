# ui-designer — Hierarchy, type pairing

What scores of 0, 2 and 4 look like on this dimension, for the `ui-designer` judge. Which text leads and which follows, and how a title pairs with its description, a label with its value. Its weight is in `.agents/evals/weights.json`; how it is scored is the judge's rubric, Quality (.agents/HARNESS.md §12).

**The rules it is held to: [TYPOGRAPHY §17](../../../skills/typography/references/typography.md#17-pairing-type-in-a-component)** — a pair's size step, its weights, the secondary never heavier, and hierarchy without colour or fine sizes (§17.3).

**Measured proxies — built.** The screen audit, `screen_audit.py`: the type sizes on each view (at most 3), each text pair's size ratio (1.0–1.5) and weight order (the secondary never heavier), and that a pair differs by size or weight at all (TYPOGRAPHY §17.1, §17.3). `verify.py proxies --judge ui-designer` runs them; a value out of its range caps this dimension at 2.

**How to use it.** Find the level the work sits nearest, quote the evidence — a file and line, a screenshot, a measured value — and give 1 or 3 when it sits between two. The person's own examples, under From the person, outrank the ones written here.

## 4 — what good looks like

- [ref-2-balance-card](../img/ref-2-balance-card.jpg) — the hero is big by size alone (~44 px **regular**); a key and its value share one size, told apart by weight (value semibold) and side (value right, tabular).
- [ref-3-transactions](../img/ref-3-transactions.jpg) — a title ~18 medium over its description ~15 regular, 1.2 : 1; each row's name medium over its category regular; the amount the one heavy thing in the row.
- [ref-6-editorial-empty](../img/ref-6-editorial-empty.jpg) — the eyebrow the smallest, quietest text; the heading 1.75x body at medium weight on a tight line.
- What they share: two or three sizes per component; hierarchy from weight first and size last; the secondary text never heavier than the primary. On the glass, where the smallest size step is 1.5 : 1 and colour cannot carry it, a 4 is one top level per screen and every pair told apart by one property the panel can draw.

## 2 — acceptable, with a clear gap

- Readable, with one top level, but some pairs jump a full size step with nearly the same weight, or a description is as heavy as its title; four sizes on one screen.

## 1 — what the person found poor

- [current-pairings](../img/current-pairings.png), FolloUp before the type study — panel 1: a row's label 30/700 over its caption 20/600, a 1.5 jump at nearly one weight; panel 4: a name and its value at the same size and weight, told apart only by a grey that a partial refresh paints black; panel 6: a choice's name 20/700 over its detail 20/600, the same size with nothing else between them.

## 0 — what fails it

- No order: every text one size and weight, a description larger than its title, or five or more sizes on one screen.

## From the person

- **2026-11-23, poor** (T1) — "Type with no visual balance: a label and its description, headings and headers paired by a 1.5 size jump with nearly the same weight"
