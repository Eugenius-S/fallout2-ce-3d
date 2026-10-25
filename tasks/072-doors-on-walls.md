---
id: 072
title: Doors sit on the 3D wall planes
milestone: M1
status: todo
owner: none
suggested: claude
claude-only: no
user-check: yes
depends-on: []
reviewed: no
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
