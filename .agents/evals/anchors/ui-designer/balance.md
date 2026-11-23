# ui-designer — Balance, alignment, white space

What scores of 0, 2 and 4 look like on this dimension, for the `ui-designer` judge. Alignment, white space and visual weight across the screen: shared edges, nothing crowded, nothing floating. Its weight is in `.agents/evals/weights.json`; how it is scored is the judge's rubric, Quality (.agents/HARNESS.md §12).

**Measured proxies — built.** The screen audit, `screen_audit.py`: the distinct left edges of start-aligned words on each view (at most 3), and words cut off by their box (none). `verify.py proxies --judge ui-designer` runs them; a value out of its range caps this dimension at 2.

**How to use it.** Find the level the work sits nearest, quote the evidence — a file and line, a screenshot, a measured value — and give 1 or 3 when it sits between two. The person's own examples, under From the person, outrank the ones written here.

## 4 — what good looks like

- [ref-4-payments](../img/ref-4-payments.jpg) — titles share one left edge; each icon sits on its title's first line, not centred on the block; the chevrons share one right column.
- [ref-2-balance-card](../img/ref-2-balance-card.jpg) — labels left, values right on one edge, a rule setting the total apart; room around the hero.

## 2 — acceptable, with a clear gap

- One stray left edge or a block crowding the margin; the screen still reads in one pass.
- [current-pairings](../img/current-pairings.png) panel 14 — a disabled dropdown's name at x 40 and its reason at x 24: two left edges for one setting.

## 0 — what fails it

- Centred and left-aligned text mixed in one column, content against the bezel, a screen heavy on one side with nothing to balance it.
