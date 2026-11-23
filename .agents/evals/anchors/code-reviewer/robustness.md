# code-reviewer — Robustness, error paths, resources on the chip

What scores of 0, 2 and 4 look like on this dimension, for the `code-reviewer` judge. Error paths and the chip's resources: stack, heap, IRAM, the bus, power states, timeouts. Its weight is in `.agents/evals/weights.json`; how it is scored is the judge's rubric, Quality (.agents/HARNESS.md §12).

**Measured proxies — built.** `code_proxies.py` measures the app's size against its partition (at most 90 %) and its growth since the baseline (image 16 KiB, DIRAM 1 KiB, IRAM none); stack high-water is not measured. `verify.py proxies --judge code-reviewer` runs them; a value out of its range caps this dimension at 2.

**How to use it.** Find the level the work sits nearest, quote the evidence — a file and line, a screenshot, a measured value — and give 1 or 3 when it sits between two. The person's own examples, under From the person, outrank the ones written here.

## 4 — what good looks like

- Every call that can fail is checked and its failure shown; power-down steps check each result and clear every wake source the next step does not arm; no wait on the UI's task; memory bounded.

## 2 — acceptable, with a clear gap

- Failures handled on the main calls, one ignored; a resource cost noticed but not measured.

## 0 — what fails it

- A wait on a busy line retried on its own failure in a loop (the V1.0 froze 8 s at a time, code-reviewer L2); a power step sent without checking the state the driver tracks (L1).
