---
id: 039
title: All of Klamath looks decent from turned views
milestone: M1
status: todo
owner: none
suggested: claude
claude-only: no
user-check: yes
depends-on: [037, 038]
reviewed: no
---

## Goal

Every static thing on the seven Klamath maps is looked at once from turned
views; the frequent failures get exceptions; 70% or more pass, weighted by
how often they stand.

## Context

- The design: [docs/2026-10-20-shapes-and-rotation-design.md](../docs/2026-10-20-shapes-and-rotation-design.md). Read it first; its "How the agents work on
  this" holds for this task.
- Exceptions go to `tools/scene_export/shapes/overrides.json`, each with a
  reason, at most 10% of Klamath's distinct static FRMs.
- The owner's report (2026-10-20, `runs/039-shapes-klamath/owner-vic-shack.png`,
  ours `vic-shack-turn-270.jpg`: kladwtwn, `-HeroTile 17665 -ViewTile 17467
  -Turn 3`): at Vic's shack the roof does not meet the front, the corner left
  of the sign is a hole into the dark inside, and the VIC'S sign hangs in the
  air. Buildings matter more than furniture here: start with them.

## Acceptance

- The weighted Klamath score 70% or more, written in the `Log` with the
  list of failures left.
- Exceptions within the cap.
- Regression 16/16, or changed references with before | after.

## Out of scope

Maps outside Klamath.

## Log
