---
id: 129
title: Klamath graphics sweep, find and fix every visible defect
milestone: M1
status: doing
owner: glm
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

### 2026-11-01 glm: sweep round 1, the tour over all Klamath maps

Done: the fresh export of all seven Klamath maps into build/scene-cache
(origin/main's exporter), the fly tour over all 43 spots at -45, 0, +45
(the sheets in build/tour/sheets/, the raw views in build/tour/after/),
the sheet-scale review of every map. Find list:

1. klatoxcv e1, tile 19091, fly +45: the cave wall boxes' TOP faces are
   flat untextured grey caps (two boxes right of the drums). Crop:
   runs/129-klamath-graphics-sweep/finds/klatoxcv-e1-19091-p45-grey-caps.jpg.
   Likely cause: the solid boxes extruded from the wall art carry a
   colourless top cap; the game camera never sees it, the fly camera does.
2. klatoxcv e1, tile 22924, fly +45: the same - the big wall box top is
   flat grey, a second box top right is flat pink-brown. Crop:
   runs/129-klamath-graphics-sweep/finds/klatoxcv-e1-22924-p45-grey-pink-caps.jpg.
   Same cause as 1.
3. Checked, not a defect: the slime pools read flat bright green from
   above and in the game view - the vanilla 2D lights the slime by its own
   light source the same way; the pools keep their texture shape. The
   map-edge voids (kladwtwn 22958, klacanyn 23869, klatoxcv e0 23879) are
   the designed map ends, the 2D shows the same.
4. No defects at the sheet scale: kladwtwn e0 (19 spots), klamall e0 (7),
   klatrap e0 (3), klacanyn e0 (2), klagraz e0 (2), klaratcv e0-e2,
   klatoxcv e0 and e2. The round 2 needs the closer inspection the tour
   cannot give: the building interiors, the shapes-eval solo shots.

Marked: 1 and 2 = open, one cause group (the solid caps' top faces).

Next: the fix thread for the group "the solid boxes' top caps take the
art's own top texels instead of the flat colour", branch
task/129-solid-cap-tops from current origin/main; then sweep round 2.
Check: the sheets and the finds crops above.
