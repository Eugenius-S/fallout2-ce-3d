---
id: 051
title: Smoke over a burning barrel draws in front of it, as in the game
milestone: M1
status: done
owner: glm
suggested: gpt
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The smoke and flames of a burning barrel are drawn over the barrel as the
classic 2D game draws them, not behind it.

## Context

- The owner's find, 2026-10-21: the smoke goes behind the barrel
  (`runs/051-smoke-order/owner-smoke-behind-barrel.png`, Klamath).
- First check the classic 2D at the same spot (`dev-snapshot.ps1 -Classic`):
  if the game also draws the smoke behind, there is nothing to fix, say so in
  the Log and close.
- Cards take the depth of their hex row's plane (`spriteRowPlane`,
  `ce/src/scene_renderer/sprite_card.h`); a smoke object on the hex behind the
  barrel lands behind it. The barrel's rim and flames were handled in 028
  (`tasks/028-barrel-rim.md`).

## Acceptance

- The smoke over the barrel as in the 2D view, before | after in
  `runs/051-smoke-order/`.
- The regression passes or its changed views show only this fix.

## Log

### 2026-10-24 glm
Done: the smoke draws in front of the burning barrel, as in the classic 2D.
The classic first, per the task: at the two burning barrels of kladwtwn's SE
corner (smoke at tiles 14896/14900, its barrels at 15097/15101) the 2D paints
the smoke's low puffs over the barrel's rim and flames; our 3D cut the column
at the rim, floating it behind the drum. The cause: the smoke's art hangs
wholly above its anchor (FRM 0x200092d, 55x77, bottom 4 px above the anchor)
and its card took its own hex row's plane, one row behind the drum's. The
fix: a live standing card whose art's bottom is at least 2 px above its
anchor (an overhead sprite with no footprint on the ground: smoke, and
nothing else on Klamath) takes the row plane of the hex one row south, one
stack step nearer than that row's own things (live_objects.cc). The fly
camera is untouched: it ignores the baked row anchors and takes each vertex's
own depth.
Check: the triple classic | before | after in runs/051-smoke-order/
(smoke-order-triple.png): the after column issues from the barrel's mouth
over the fire, as the 2D. ctest 12/12; the regression with the pinned config
is exactly the pre-existing 054 baseline (the same 13 of 16, klacanyn-e0,
klaratcv-e0 and klatoxcv-e2 at 0): none of the 16 views moved, the smoke
barrels sit outside them. The shared build/scene-cache had been rebuilt as
the main-version (v5) export for this branch; the 046 branch's v6 cache is
kept aside as build/scene-cache-046v6.
Next: the owner judges the look (user-check: yes). If a fire pit's flames
ever read wrong in front of its ring, the overhead rule is one condition in
live_objects.cc to tighten.
