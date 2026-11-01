---
id: 135
title: The 3D view stays on in combat and whenever the map is on screen
milestone: M1
status: doing
owner: claude
suggested: claude
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

### 2026-11-01 claude, step 1
Block reasons (`preview_policy.h`):
- game-state: kept. Outside normal play (loading, the main menu) the game
  shows no map.
- game-mode: narrowed to the world map, the only mode that shows no map.
  Combat, the player's turn, inventory, loot, barter, options, the automap
  and the rest keep the 3D view: a window covers the scene as it covers the
  2D map (the overlay mask from `windowShownMask`).
- ui-disabled: removed. The enemies' turn and scripted moments show the map.
- iso-disabled: kept. The game does not draw the map (loads, fades).
- combat: removed.
- dialog: kept for now; 066 draws the dialogue backdrop in 2D. Step 2.
- movie: kept. A movie covers the screen.

Check: ctest; the toxic caves entrance in 3D in the background run.
Not checked yet: a whole fight (no combat fixture). Next: the owner plays a
fight in Klamath and the toxic caves; a combat fixture for the background
run; the to-hit cursor, outlines and floating text in 3D; dialogue.
- 2026-11-02 The owner played a fight in Klamath on `a4d47b26`: it stays in 3D start to finish; the only flaw seen is the unscaled outlines (149).
