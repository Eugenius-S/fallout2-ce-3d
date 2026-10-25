---
id: 072
title: Doors sit on the 3D wall planes
milestone: M1
status: done
owner: claude
suggested: claude
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

Every door is pinned to its 3D wall's plane, so from a turned view it stays
in the wall instead of floating as a card.

## Context

- The owner asked for it on 2026-10-24 and showed it on 2026-10-25 in
  their live game: an open door stands out of its wall at an angle
  (`runs/inbox/2026-10-25-open-door-buckner.webp`, the Buckner house; also
  `2026-10-25-door-ajar-on-wall.png`), and closed doors sit crooked on their
  walls (`2026-10-25-bathhouse-interior.png`). The PLANE mechanism from 037 pins
  signs to walls already; doors need the same, with their open and closed
  frames.

## Acceptance

- Turned shots of a few doors, open and closed, under
  `runs/072-doors-on-walls/`.
- The regression stays where it is.

## Out of scope

Wall geometry (071).

## Log

### 2026-10-25 claude
Looked, nothing changed. Doors already lie on their wall's plane under the
free camera since 037 (`onFlyPlane`, live_objects.cc), and in today's fly
shots of kladwtwn from four sides every door sits in its wall
(`runs/inbox/2026-10-25-door-on-wall-fly.png`). The owner's review shots
16 and 17 show wall ends at corners, not doors: 071 fixed those.
Next: the owner's screenshot of the door that floats (map, where, which
view: fly camera, turned view, an open door), then fix that case.

### 2026-10-25 claude
Done: an open door under a free camera is a real leaf on a hinge. The game
draws an open door as a picture of the swung leaf; laid on the wall's plane
(037) it read as a crooked panel sticking out of the wall. Now
`scene_objects.cc` hands the renderer the door's closed frame and how far it
is open (frame / last frame), and `swingDoor` (live_objects.cc) turns that
closed leaf round its hinge, out to the camera's side of the wall; the hinge
is the leaf's edge nearer to where the game draws the open door. The game's
own view keeps the frame it shows. A new dev switch opens every door of the
map for snapshots: `dev-snapshot.ps1 -OpenDoors` (`--dev-open-doors`).
Check: ctest 15/15; the game's view 0 pixels changed (kladwtwn, klamall);
the Buckner door open from three sides in `runs/072-doors-on-walls/`.
Not settled: the closed doors the owner saw sitting crooked in the bathhouse;
in today's shots closed doors lie flat in their doorways. If they still read
wrong in the owner's game, that is a new task with a screenshot.

### 2026-10-25 claude (the owner's call)
The owner: no doors opening in 3D. Under a free camera a door standing wide
open (its last frame) is not drawn at all, the doorway stays empty; a door
caught between frames keeps its picture on the wall's plane, as since 037.
The hinged leaf (`swingDoor`) is gone. `-OpenDoors` stays for snapshots.
Check: ctest 15/15, the game's view 0 pixels changed; swung against hidden:
`runs/072-doors-on-walls/buckner-door-swing-vs-hidden.png`.
