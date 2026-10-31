---
id: 122
title: Things turn in flight: objects with full rotation sets face the fly camera
milestone: M1
status: doing
owner: glm
suggested: owner
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The owner, 2026-10-31: 087 made critters turn with the fly camera; now every
OBJECT whose art carries all six rotations and more than one of them turns
the same way. An object the fly camera looks at from another quarter shows
the rotation frame for that quarter instead of staying at its made rotation.

## Context

- 087's shownArt turns critters, items and misc; scenery was excluded
  because some scenery arts spend the six direction slots on variants (six
  different cacti) or damage states (a shelf breaking up), not on sides.
- tools/shapes/art_sides.py lists 57 item and scenery files with more than
  one stored direction; the six slots alone say nothing (the unused-art
  placeholders ship as real frames).
- Exclusions (the owner's list): the vault dweller head in Arroyo and the
  fallen vertibird - single-direction arts, they never qualified.

## Acceptance

- Fly pairs of multi-rotation things from two opposite quarters: the art's
  facing follows the camera.
- Unique objects and single-rotation arts do not turn.
- The game's views stay pixel for pixel; ctest passes.

## Out of scope

Critters (087 did them), solids re-projecting per quarter, unique objects.

## Log

### 2026-10-31 glm: the gate really rejects placeholders now, and the debug prints left the build

Two cleanups on the gate after the 120 session's measurements:

- The five fprintf(stderr, "sat: ...") traces from the hang hunt rode the
  committed branch and would have spammed the game's stderr; removed.
- The gate now also fails an art whose any direction is the arts' UNUSED ART
  placeholder card (a frame opaque edge to edge, the same 97% rule the
  exporter's props.real_directions uses) or empty. The overlap loop alone did
  not promise that: the tire piles' TIRE004 holds four placeholder slots and
  its adjacent-pair IoU is 0.48 - over the 0.4 bar - because the tire's own
  side views are wide low ovals that overlap the 46x22 card.

The measured truth about the bug's reach (so nobody hunts a ghost): in the
shipped game the gate never runs for TIRE004 at all. Every one of its 36
placements is covered by the baked-card pass (walls rows / SKIP in
baked.bin), so its card is pick-only and shownArt passes flat=true before the
gate. The placeholder-holding arts that DO stand as live cards (truck2 on
newrcs, truck3 on coast4/navarro, car1 on denres1/newrcs) all fail the overlap
bar (IoU 0.01-0.02, the engine's own trace agrees with the replication:
CAR4b 0.7530 turns, the tire never reached). So no shipped map shows a turned
placeholder today; the check makes the gate's documented contract true and
guards mapper-made maps, where a tire pile can stand as a live card. The
before/after fly shots of the tire (runs/_122fix_shots) are byte-identical,
which is the point: the fix's surface in the shipped game is empty.

Checks: ctest 18/18; the 16 regression spots after this commit.

### 2026-10-31 glm: the owner's correction: the cars are not for turning - they convert to 3D models

The owner, on the turning pairs: «по машинам - их надо в 3д модели
конвертировать». The multi-side big things (the cars CAR2b/CAR4b, the
trucks, the bath) are not for sprite turning at all: they belong to task
120's 3D-ification as real models - a fitted solid whose faces can take
their textures from the matching rotation views of the art (the six true
sides are exactly the material a proper model needs). The turning rule of
this task stays for the smaller multi-side scenery (the cola machine, the
wall lamp, the windows) pending the owner's word; the Den car pairs here
remain as the rule's evidence.

### 2026-10-31 glm: the rule is a silhouette-overlap test, and the Den's car turns

The turn rule, by measurement: a scenery art turns when every ADJACENT pair
of its six direction silhouettes, anchored at their bottom centres, overlaps
by at least 40% (the IoU of the opaque masks). The survey of all 57
multi-direction files separates them cleanly: the cola machine (0.94), the
Den cars CAR2b/CAR4b (0.46/0.73), the wall lamp and the Vault City windows
(0.48-0.62) are true sides; the cacti (0.10-0.16), the shelves' damage
states (0.02-0.04), the cars' junk-mixed files car1-5 (0.01-0.06) and the
wdbeam placeholder frames (0.00-0.11) stay at their made rotation. No name
lists. The engine (scene_objects.cc, sceneryArtTurns) computes the test once
per art under a mutex and shownArt lets qualifying scenery through 087's
existing turn path; pick-only cards (the baked solids) pass flat=true and
never turn, so a stood solid keeps the projection it was made from.

Three bugs bit on the way, all mine, all in the C++ translation of the
survey's Python alignment: two index errors (an extra half-span shift and a
flipped vertical) wrote outside the comparison grid and the heap corruption
hung the render loop; a frame-size guard (the card pass's own 2048 limit)
now rejects broken placeholder directions outright. The hangs cost three
build-and-trace rounds; the instrumented trace pointed at the IoU block
both times.

Checks: the exporter unittests 75 and ctest 18/18 on the branch; captures
in runs/122-things-turn-in-flight/ (the Den's CAR4b at fly turns 0 and 90,
the sprite-sides setting off | on: at turn 90 the car shows another side;
the cacti and shelves around it stay put). The regression run against
cache-121 is the branch's last check.

Branch note: branched from task/121-holes-filled-everywhere (that PR carries
the fill; both wait on the earlier reviews).
