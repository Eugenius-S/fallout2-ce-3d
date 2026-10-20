---
id: 028
title: The rim of a burning barrel breaks off in 3D
milestone: M1
status: doing
owner: claude
suggested: claude
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

A burning barrel in 3D shows its whole rim, as in 2D.

## Context

- The owner's screenshot (`runs/028-barrel-rim/owner-report.png`, Klamath,
  3D, build of PR #20): the top edge of a burning barrel, circled in red,
  stands apart from the barrel's body, as if broken off.
- The barrel is scenery made solid by the exporter (`tools/scene_export/solids.py`,
  baked into `walls.bin` by `wall_sidecar.py`); its fire is a separate
  animated sprite. Find first whether the rim belongs to the baked solid's top
  or to the fire sprite's card, and compare with 2D at the same spot.

## Acceptance

- The barrel's rim is whole in 3D at the owner's spot and on the other
  burning barrels of Klamath; snapshots 2D | 3D in the PR.
- Regression snapshots unchanged elsewhere.

## Out of scope

Glow of the fire (022).

## Log

### 2026-10-20 claude
Done: the rim belonged to the solid, not to the fire. A drum (burning
barrel, metal barrel, hydrant, pot) on its card's depth drew only the quarter
of its body facing the camera, so its body was about 0.7 of the rim's width
and the full top stood out over it. Now it draws the near half of the body
(angles 0 to 180), each side showing the sprite pixel it lands on, as the game
sees it. A drum with its own depth (board -1) keeps the donor's mirrored sides.
Evidence: `runs/028-barrel-rim/burning-barrels-2d-before-after.png` (three
burning barrels of Klamath: 2D, 3D before, 3D after) and
`regression-changes-before-after.png` (every changed spot of the regression:
drums only). Regression references retaken for the 13 cases with drums; 16/16
on them. New exporter test: a drum's body spans its rim on the game's screen.
Next: merge, then `export.py` into `game/mod-cache` for the owner's game.
Check: `py -m unittest discover -s tools/scene_export`, `tools/regression.ps1`.
