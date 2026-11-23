# ui-designer — Fit to the medium: e-ink cost, ghosting

What scores of 0, 2 and 4 look like on this dimension, for the `ui-designer` judge. Fit to e-ink: the refresh it costs, ghosting, the one-bit panel, the 10-unit pixel font. Its weight is in `.agents/evals/weights.json`; how it is scored is the judge's rubric, Quality (.agents/HARNESS.md §12).

**Measured proxies — built.** The screen audit, `screen_audit.py`: the share of the glass a ring move changes (at most 15 %, STICKY_DESIGN_TOKENS §5); refresh times against STICKY_EPAPER_WAVEFORMS §4 are the bench's, not measured here. `verify.py proxies --judge ui-designer` runs them; a value out of its range caps this dimension at 2.

**How to use it.** Find the level the work sits nearest, quote the evidence — a file and line, a screenshot, a measured value — and give 1 or 3 when it sits between two. The person's own examples, under From the person, outrank the ones written here.

## 4 — what good looks like

- A change painted with the partial refresh the refresh policy names, a full refresh only where it says ([STICKY_DESIGN_TOKENS](../../../../docs/STICKY_DESIGN_TOKENS.md)); no motion the glass cannot show; text in solid ink, never on a dither; sizes on the font's 10 px cell and whole pixels.

## 2 — acceptable, with a clear gap

- Right on the glass, but it costs more than it needs: a full refresh for a small change, a grey that ghosts after a few partials, a dithered fill behind text.

## 0 — what fails it

- Built for an LCD: animation, many refreshes in a row past the back-to-back guard, text on a dither, sizes off the font's cell ([STICKY_EPAPER_WAVEFORMS](../../../../docs/STICKY_EPAPER_WAVEFORMS.md)).
