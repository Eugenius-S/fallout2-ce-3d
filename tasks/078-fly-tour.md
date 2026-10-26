---
id: 078
title: A fixed fly tour shoots every map before and after each scene change
milestone: M1
status: doing
owner: claude
suggested: glm
priority: high
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

One command shoots a fixed set of turned camera views per Klamath map
(building corners, walls, roofs, furniture, at 0, -45 and +45 degrees) and
puts them on a before | after sheet, so an agent checks its own scene
change on one sheet before the owner looks.

## Context

- The owner, 2026-10-25: an automatic tour instead of their manual walk
  round the game; 071's corners took four rounds of the owner's eye.
- Building blocks: `tools/dev-snapshot.ps1` (`-FlyTurn`, `-FlyPitch`,
  `-FlyDistance`, `-ViewTile`, `-Map`), the shapes eval's one-run shooting
  and sheets (`tools/shapes/eval/eval.py`, 041 and 043), `tools/sheets`
  if it fits. One game run per map, as 041 does.
- Spots to start from: the kladwtwn long building corners (tiles 24268,
  24292, 20268, 20292, 20690), the bathhouse front (19265), Vic's shack
  (17467), the owner's spots in `runs/inbox/2026-10-25-review2-*.png`; add
  a few per other Klamath map. The spot list lives in one file.
- Turns stay within 45 degrees (077).

## Acceptance

- `tools/tour.ps1` (or the eval's command) shoots the tour for all seven
  maps and writes `before | after` sheets between two builds or caches;
  the sheets of one run are in `runs/078-fly-tour/`.
- AGENTS.md says a scene change attaches the tour sheet to its PR.

## Out of scope

Judging the views automatically.

## Log
