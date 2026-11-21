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
