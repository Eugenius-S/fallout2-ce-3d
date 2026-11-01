---
id: 135
title: The 3D view stays on in combat and whenever the map is on screen
milestone: M1
status: todo
owner: none
suggested: glm
priority: high
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The owner, 2026-11-01: "it must always work". Today the 3D view drops to
the original 2D picture in combat, while the interface is disabled (the
enemies' turn, scripted moments), while any game window is open over the
map, and in dialogue. The owner had never seen the toxic caves in 3D,
because the geckos start a fight at once. Wherever the map is on screen,
the player sees it in 3D; a fight in Klamath is played start to finish in
3D.

## Context

- `ce/src/scene/preview_policy.cc` returns the block reason
  (game-state, game-mode, ui-disabled, iso-disabled, combat, dialog, movie)
  and `scene_view.cc` stops presenting on any of them. The policy dates from
  the first preview; nobody decided that combat must stay 2D.
- Remove the blocks that are only caution. Keep a block only where the
  original game does not show the map either (a full screen window, a
  movie), and say in the Log why each kept one stays.
- Combat brings what the 3D view has not drawn before: the hex cursor and
  its to-hit number, the red and green outlines round critters, the attack
  and the projectiles, explosions, floating damage text, the end turn and
  end combat buttons, critters walking during the enemies' turn. Each must
  work and land on the right tile in 3D. What already works in the 3D view
  (picking, the no-path cross, the see-through circle) must keep working.
- Dialogue: 066 hides the street behind black on the dialogue screen. Check
  what the 3D view should show there and match the original's layout.

## Acceptance

- A whole fight in the toxic caves (klatoxcv) and one in Klamath downtown,
  in 3D from the first shot to the end of combat: a short video or a strip
  of shots in `runs/135-3d-always-on/`, with the to-hit cursor, an attack,
  an enemy turn and the end of combat.
- Clicks in combat hit the tile and the critter they hit in the original
  view (a check by script, as the ground pick fixture does).
- The Log lists every block reason: removed, or kept with the reason.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

New combat effects, the look of the outlines, the combat interface's layout.

## Log
