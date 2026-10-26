---
id: 087
title: Characters turn with the camera: the sprite shows the side the camera sees
milestone: M1
status: done
owner: claude
suggested: glm
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

When the camera turns, people, creatures and robots (every critter) show
the side of themselves the camera now looks at, so a flat sprite reads as
a round figure. Three stages:

1. All critters.
2. Find the other objects that have all 6 sides and stay 2D sprites
   (scenery, items), and turn them the same way.
3. The 2D objects with fewer than 6 sides turn too: each shows the
   nearest side it has, switching sharply from one to the next.

## Context

- The owner, 2026-10-26: objects that have art for every side but stay
  sprites should turn as the camera turns, faking volume. Start with all
  characters; check the look on one, then roll it out to the rest without
  looking at each.
- Fallout draws a critter in 6 directions, 60 degrees apart (`rotation`
  0..5 in the frame). Today a card faces the fly camera
  (`faceFlyCamera` in `ce/src/scene_renderer/live_objects.cc`) but keeps
  the object's own `rotation`, so a turned camera still sees the side the
  game's camera would.
- The idea: the shown rotation is the object's rotation shifted by the
  camera's turn from the game's view, rounded to the nearest 60 degrees.
  This holds for the free fly camera and the turned game camera (038,
  limited to 45 degrees by 077). Near a 30-degree boundary keep a little
  hysteresis so the sprite does not flicker between two sides.
- The frame texture cache already keys on rotation (`objectArtKey`); a
  shown rotation that differs from the game's one is only another key.
- Game logic, facing and combat stay on the game's own rotation; only
  what is drawn changes.

## Acceptance

- One critter (the hero) shot at camera turns 0, 30, 45, 60 and 90
  degrees, before | after, in `runs/087-sprites-turn-with-camera/`, so the
  owner can judge the look.
- Every critter on the map turns the same way (one short sheet of a few
  critters is enough).
- Stage 2: the list of 6-sided 2D objects found (by art, not by hand) is
  in the task log, and they turn; one sheet of a few.
- Stage 3: the objects with fewer sides switch to their nearest side; one
  sheet of a few.
- The game's own view stays pixel for pixel (the regression).

## Out of scope

Objects already turned into 3D shapes (075) or walls; real 3D figures.

## Log

### 2026-10-26 claude
Done: under a fly or turned camera a standing critter, item or projectile
(misc) whose art has more than one side is drawn in its rotation moved by the
camera's yaw from the game's view, in 60-degree steps with 5 degrees of
hysteresis past each half-way (`spriteTurnSteps` in
`ce/src/scene_renderer/sprite_card.cc`, the per-frame count in
`live_objects.cc`, the choice of art in `shownArt`,
`ce/src/scene/scene_objects.cc`). Only the drawn art changes; the game keeps
its rotation. Flat sprites (they turn with the ground) and things `walls.bin`
draws keep theirs. `[3d] sprite_sides` (on) switches it; `dev-snapshot.ps1
-NoSpriteSides` shoots the old look. On the ground the hexes are squashed
(neighbouring facings 53 or 63 degrees apart), so the step is the art's own
60 degrees and only its sign comes from the ground.

Stage 1: `runs/087-sprites-turn-with-camera/hero-turns.jpg` (the hero at
turns 0/30/45/60/90, before | after: 30 still shows the game's side, inside
the hysteresis; 45 and 60 one side on, 90 two) and `critters.jpg` (brahmin and
townsfolk at 60 and 90).

Stage 2: `tools/shapes/art_sides.py` lists every item and scenery FRM with
more than one stored side: items spear, flare, rock; scenery cac1000,
cac2000, car1..car5, car1b..car5b, cola, truck2, truck3, vwin01..vwin04,
vwin06, vwin07, wlamp5, bath01, bath02, ss122, ss123, wdbeam01..wdbeam10,
wdbeam20..wdbeam25, crafter2, pipe003, tire004..tire006, brkglass,
shelf001..shelf006. `art/misc` adds 14 projectiles (rocket, knife, spear,
molotov, ...). The sheet `stage2-six-sided-art.jpg` shows why scenery stays:
its rotations are variants (other wrecks, other cacti, a shelf or window
breaking up, "UNUSED ART" placeholders), cola and wlamp5 are two wall
facings; turning them would swap the object. Items and projectiles turn.

Stage 3: no FRM stores 2 to 5 sides; fewer sides means one stored side (67
critter FRMs, most items and scenery) or six with repeats (9 critter
remains, all flat when shown). The art lookup in the moved rotation already
gives the nearest side; a one-sided object keeps its picture
(`stage3-fewer-sides.jpg`: the deathclaw egg and mabos2aa through a full
turn).

Next: nothing; the owner judges the look.
Check: `ctest --test-dir build --output-on-failure`;
`tools/dev-snapshot.ps1 -TurnDegrees 60 -Still` against the same with
`-NoSpriteSides`; `tools/regression.ps1` (0 changed pixels).
