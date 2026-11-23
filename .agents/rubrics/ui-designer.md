# Rubric — ui-designer

How the [ui-designer](../agents/ui-designer.md) scores a UI change in review mode. The verdict, the evidence rules, the
items R1–R20 and the report's shape travel with the skill, in
[`ui-designer/references/rubric.md`](../skills/ui-designer/references/rubric.md) (UI_REVIEW_RUBRIC), so the agent and
the skill work in any repo. This file holds what this project adds: the items learned from what the person found on
the target ([verify-change](../skills/verify-change/SKILL.md#recording-what-was-learned) → Recording what was learned,
route `rubric`). A review scores the skill's items and every item here. A new learned item goes here, not in the skill
or the agent.

## Learned

| Item | Since | From the person | Looks for |
| --- | --- | --- | --- |
| L1 | 2026-11-20 | The ui-designer, before the glass: the type scale page stepped every size by body's line space while printing each size's own | A specimen or swatch page: every property it labels — size, line height, weight, colour — set on the sample itself, never inherited from the screen's style |
| L2 | 2026-11-20 | The ui-designer judge, before the glass: a disabled row's value stayed grey on the dither | A state style on a composite (disabled, chosen, focused): every child that sets its own colour at selector 0 also given the state's colour, or the parent's state trickled to it — a child's own value beats the parent's state style |
| L3 | 2026-11-23 | The person: "Don't change my checkmarks, radio, and switch components. I've asked you to fix typography and spacing, not make changes to UI components." | A mock or build that changes a component outside the layer asked for: a per-control pixel diff against today shows every other layer unchanged |
| L4 | 2026-11-23 | The person, promoted from taste rows T2 and T3: spacing with no visual harmony, and too little margin below a large heading | Every gap on the spacing ladder (LAYOUT §7.1): pairs glued and groups spaced, the gap below a heading growing with the heading's size, measured between the texts' own boxes and checked by ink |

## Quality

The ui-designer scores its dimensions as [HARNESS §12](../HARNESS.md#12-quality-taste-and-weights) says; its anchors are in `.agents/evals/anchors/ui-designer/`. The ui-designer's review scores what the change touches, and `verify.py audit` scores the whole product.

## Taste

The ui-designer reads every row below before it scores; [HARNESS §12.3](../HARNESS.md#123-the-persons-taste) says how a row is recorded and promoted.

| Item | Since | From the person | Dimension | Anchor |
| --- | --- | --- | --- | --- |
| T1 | 2026-11-23 | "Type with no visual balance: a label and its description, headings and headers paired by a 1.5 size jump with nearly the same weight" (poor; a miss — every UI review passed it under R20, before scores; judgements.jsonl 2026-11-23) | `hierarchy` | [.agents/evals/anchors/ui-designer/hierarchy.md](../evals/anchors/ui-designer/hierarchy.md) |
| T2 | 2026-11-23 | "Spacing with no visual harmony across components and layouts: the same pairing 8 to 41 px apart, one-off tokens" (poor; a miss — every UI review passed it under R19, before scores; judgements.jsonl 2026-11-23; promoted to L4) | `spacing` | [.agents/evals/anchors/ui-designer/spacing.md](../evals/anchors/ui-designer/spacing.md) |
| T3 | 2026-11-23 | "There needs to be more margin between large display headings and body text as well as modal titles and description. Usually larger text need more margin below their description or body text than smaller text" (poor; a miss — the ui-designer's mock passed it, before scores; judgements.jsonl 2026-11-23; promoted to L4) | `spacing` | [.agents/evals/anchors/ui-designer/spacing.md](../evals/anchors/ui-designer/spacing.md) |
| T4 | 2026-11-23 | "Don't change my checkmarks, radio, and switch components, I've asked you to fix typography and spacing not make changes to ui components" (poor; already the floor item L3) | `consistency` | [.agents/evals/anchors/ui-designer/consistency.md](../evals/anchors/ui-designer/consistency.md) |
