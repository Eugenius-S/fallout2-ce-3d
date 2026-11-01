---
id: 129
title: Klamath graphics sweep, find and fix every visible defect
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

Every map of Klamath (kladwtwn, klamall, klagraz, klacanyn, klaratcv,
klatoxcv, klatrap, all elevations) looks clean in the game camera and in the
fly camera at morning, noon and evening: no holes, no flat things that
should stand, no seams, nothing poking through walls or roofs, no things
floating or sunk, no wrong shapes. This is the owner's first prerequisite
before any release: "a fully working game without most graphic bugs".

## Context

- The owner's finds so far live in `tasks/inbox.md` and in tasks 100-125.
  Read them first so the sweep does not refind what a task already covers.
- The fly tour (`tools/fly-tour*`), the regression spots and the snapshot
  tools give the views. Add tour spots where the tour does not reach.
- Fix by general rules in the exporter or the renderer. A per-object
  override in `shapes/overrides.json` is allowed only where the task of that
  kind already allows it, and the Log says why a rule cannot do it.

- Folded in: 101 (roof edges clean, the roof fringe stands up, no wall tops
  through roofs) and 110 (no thin seam where two walls meet at a corner).
  Neither reproduced on main in earlier rounds; check both on every sweep.

## How to work

A loop, without waiting for anyone between rounds:
1. Sweep: walk every map and camera, write each defect as one numbered line
   in this task's Log (map, tile or spot, camera, hour, a crop in
   `runs/129-klamath-graphics-sweep/finds/`, the likely cause).
2. Group the finds by cause. Take the group with the most visible effect.
3. Fix it on its own branch `task/129-<short-cause>` from current origin/main,
   with before | after crops of each find it closes, ctest, the original
   view regression against origin/main and the fly tour. One PR per cause
   group, titled `129: <cause>`; mark the finds it closes in the Log.
4. Go back to 2. When the list is empty, sweep again (1). A defect that
   needs the owner's eye or a design decision goes into the Log as
   "owner:" and the loop moves on.

## Acceptance

- Each PR: before | after crops, ctest passes, the original view
  regression is pixel for pixel or every changed pixel is a closed find,
  the fly tour changes are explained.
- The Log holds the full find list with each line marked fixed (PR number),
  owner, or open.

## Out of scope

The interface, menus, wallpapers and text (gpt's tasks 126-128), the sky
(113), new 3D models of landmark objects from scratch (a later task).
