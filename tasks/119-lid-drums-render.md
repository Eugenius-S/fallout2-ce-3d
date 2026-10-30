---
id: 119
title: Lid drums render right: the raise lands on the lid, the drum matches its card, the window covers the stoves
milestone: M1
status: todo
owner: none
suggested: glm
priority: high
claude-only: no
user-check: yes
depends-on: [116]
reviewed: no
---

## Goal

The stoves stand as drums with their lid on top and the lockers as boxes in
the fly view, rendered at their card's size and place; the game's own views
keep their pixels.

## Context

Task 116 built the fit and the rules (props._lid_drum, the loose and container
gates, its tests) and then stood them down: the fly solid of a seen=1 row
rendered at the wrong scale (a stump against its card, the scene row said
z1=1.12 where the fit probe said the raise landed at 58 px - the raise's EMPTY
cost stops in the gap under the lid), and after that the card is already
hidden (fly_hidden), so the thing vanishes. Also:

- The kladwtwn stove hexes (20937, 22528, 26106 - ground anchors 49..73) sit
  outside the region the wall sidecar exports (its vertices span about
  -35..32): their solids never reach the engine no matter what the fit says.
  Find what draws that window (the authored map clip the devshot JSON logs as
  map_clip) and whether the bathhouse stove (klamall 24138) is inside its own
  map's window.
- The lockers are OBJ_TYPE_ITEM containers with animating arts (3 frames):
  live_solids keeps them live cards on purpose. The fly-only closed box
  (live_solids exempts seen==1 rows now) needs either engine-side live solids
  or the owner's OK with fly showing them closed.
- Wall scenery (bladsign) must keep its wall-sheet sorting: a fit-able card at
  its hex got occluded by its own wall and lost a third of its pixels in the
  game view. The stand bypass, if ever re-tried, is obj_type-gated.

Useful, proven: -HeroTile next to (or on, for NO_BLOCK things) the target
makes the engine hide the roof (frametest2); -Solo strips the whole sidecar
(useless for solids); -FlyPitch takes positive degrees only; -FlyTurn 45
-FlyPitch 25 -FlyDistance 6-12 orbits fine. The interior lit pair
(runs/116-stove-and-lockers-solid/stove3-lit.jpg) shows the blocker.

## Acceptance

- before | after of the bathhouse stove, the other Klamath stoves and the
  lockers, game angle and flight, the drums and boxes at their card's size
  and place, in runs/119-lid-drums-render/.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

New shapes for other things; engine live-solid work unless the owner asks.

## Log
