---
id: 076
title: Walls, corners and roofs in 3D: one rule instead of patches
milestone: M1
status: done
owner: claude
suggested: claude
claude-only: no
user-check: yes
depends-on: [071]
reviewed: yes
---

## Goal

From any fly camera angle buildings read as whole: walls meet cleanly at
every corner, no wall shows through another, nothing sticks out of a roof.
The fix is in the rule that builds walls and roofs from the map, not in
patches for single spots.

## Context

- The owner's second live review after 071 round 4 (2026-10-25,
  `runs/inbox/2026-10-25-review2-01..10.png`, the list in
  `tasks/inbox.md`); their verdict: fix the 3D-fication algorithm itself,
  no hand-made cases.
  - 01: the STOP corner still meets wrong.
  - 02, 05, 06, 07: dark buildings' corners show a striped strip of light
    or other material down the corner.
  - 03: a wall shows through another wall.
  - 08: walls and triangular pieces stick out of the roofs.
  - 10: a house corner broken next to a door.
- Where 071 left it: corners are closed by matching wall ends in the
  exporter (`tools/scene_export/wall_sidecar.py`, `_corner_fits`), fly
  camera only; the corner picture is borrowed from the neighbouring wall.
  The stripes likely come from that borrowed picture (the donor's corner
  sprite belongs to one wall's line). Roof squares lie on the map grid and
  need not match the wall lines, which may be what sticks out.
- Directions to weigh: build each building's outline once (walls as a
  closed polyline per elevation) and derive corners, caps and roof
  edges from it; take corner faces from the corner sprite itself; clip
  roofs to the outline and walls to the roof.
- The game's own view stays pixel for pixel (`dev-snapshot.ps1 -Map
  kladwtwn|klamall` against the pre-071 shots: 0 px).
- The camera ships limited to 45 degrees either way (077), but the owner
  wants corners and walls to meet right by the algorithm, not by spot
  fixes; views within 45 degrees come first.
- The owner may add more from the same review.
- This task takes over 060 (Vic's shack from the back: roof gap, open
  corner, hanging sign) (closed into it).

## Acceptance

- The owner's spots read clean from turned views, before | after in
  `runs/076-walls-roofs-algorithm/`.
- The game's own view unchanged (regression 16/16).

## Out of scope

Things and furniture (075).

## Log
### 2026-10-25 claude
Done: the corners of plain buildings close for the free camera, by rule.
- Stripes: a block that fills a corner's shortfall, and an end face, took
  their columns from the row nearest the corner, often a 2-column end
  strip (the light edge), stretched 8 to 15 times: the striped strips of
  the owner's shots 02, 05, 06, 07. They now take a row at least half as
  wide as the columns they need (`_corner_fits` width, px): 116 of 276
  blocks and 57 of 234 end faces on kladwtwn and klamall changed picture.
- Holes: a corner piece's picture is often clear along its edge, where
  the game shows the other wall behind it; the free camera looked through
  it into the hollow wall. A one-colour core (the picture's median opaque
  texel) now stands inside the corner square and 0.5 into each solid wall,
  within what the free camera keeps of the wall; mesh fences (front
  pictures under 90% opaque) and thin rails get none, so they stay clear.
- Check: `tools/scene_export/corner_rays.py <scene dir>` casts rays through
  every corner where the walls stand. kladwtwn: corners with rays through
  10 -> 4 (2 grazing rays of ~660 each); klamall's left are where a mesh
  fence or a 0.06 rail meets the corner; klagraz, klatrap, klacanyn 0.
- The game's own view is untouched: kladwtwn and klamall snapshots match
  the pre-071 shots, 0 px. Exporter tests 52/52.
- Before | after in `runs/076-walls-roofs-algorithm/corners-before-after.jpg`
  (rows: 12666, 13050 and 17104 lose the light stripe, the STOP corner
  turned, 24268 from behind, the klamall fence corner stays a mesh).
- Screen sampling (texels from what the game drew at a face's screen
  point) was tried and dropped: at 24268 the corner piece shows the other
  wall's face there, and bricks came out sheared.
Left: roofs and walls that stick out of them, a wall seen through
another, the corner by a door: task 080.
Check: `py -3 tools/scene_export/corner_rays.py build/scene-cache/kladwtwn/0`.
