---
id: 032
title: Long dashes left in four run READMEs
milestone: M1
status: done
owner: glm
suggested: glm
claude-only: no
user-check: no
depends-on: [030]
reviewed: yes
---

## Goal

No long dash (U+2014) is left in text we wrote.

## Context

- Found in the review of 030: the title line of `README.md` in
  `runs/p1-floor-review-20261018`, `runs/p1-hero-review-20261018`,
  `runs/p1-walk-review-20261018` and `runs/p1-wall-review-20261018`.
- The hits in `ce/src` came with the engine import and stay.

## Acceptance

- `git grep -I -n` for the long dash character finds only `ce/README.md`
  and `ce/src` files from the engine import.

## Out of scope

Other text changes.

## Log

### 2026-10-20 glm
Done: the title line of each of the four run READMEs rephrased with a comma
(for example "# P1 static-floor review, 2026-10-18"); meaning and dates kept.
Check: git grep -I -n for the long dash over runs/ finds nothing; the only
remaining hits are ce/README.md and ce/src from the engine import.
