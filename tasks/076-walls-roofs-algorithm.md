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
- The owner's look at the STOP corner (2026-10-25): the end face stuck out
  as a skewed jamb. It took the corner piece's picture, which draws the
  corner seen at an angle; the next try took a grey concrete wall's,
  which is another building's. An end face now takes the most typical
  columns (brightness nearest the median column) of its own wall's rows
  near the corner, past a gap in its line too, off the corner square; the
  other wall's rows join only when their material is alike (median
  brightness within FACE_OTHER). Its height stays that of the wall whose
  plane it lies in. The STOP corner reads as brick to the edge, the black
  building's corner as its dark wall.
- Screen sampling (texels from what the game drew at a face's screen
  point) was tried and dropped: at 24268 the corner piece shows the other
  wall's face there, and bricks came out sheared.
Left: roofs and walls that stick out of them, a wall seen through
another, the corner by a door: task 081.
Check: `py -3 tools/scene_export/corner_rays.py build/scene-cache/kladwtwn/0`.

### 2026-10-25 claude (round 2, the owner's yes/no)
The owner turned down the STOP corner (a skewed jamb, then a grey post) and
asked for corners that meet with no gap and nothing sticking out
(`runs/inbox/2026-10-25-corners-yes-no.jpg`). The rule is now face by face:
- At an L of two thick walls each wall's outer face (away from the other's
  body) runs to the other's outer face and its inner face to the other's
  inner face: the outer faces meet in one edge, the inner ones in another,
  and no end face stands between them. At STOP the outer corner is one
  wall's back and the other's front; 071 had stopped both faces of a wall
  at one place and closed the rest with an end face, the jamb. Tops stop at
  the inner faces; one cap covers the square where the walls cross.
- The corner piece (a row running into that square) is left out for the
  free camera: its picture draws both walls of the corner at an angle, and
  on one wall's plane the other's bricks came out skewed. Each face is
  filled from the last plain row to its edge in that face's own material
  (front or back pictures), the columns nearest the median brightness and
  fully opaque, so plinth and cornice bands run round the corner. Walls up
  to 1.5 units keep their pieces (both ends may be corners).
- A thick wall does not reach for a rail or a trim strip (some are wholly
  clear: it made a stub); a trim lying wholly within reach of a thick wall
  is left out (a ledge stood up out of a roof); where a wall's end is moved
  next to a rail, or next to a wall that does not turn the corner back, it
  gets an end face of its own; a wall reaching past the other end of a short
  wall is that end's corner, not this one's.
- Each arm of the core stays below its own wall's top (Codex review).
- Check: `corner_rays.py` over all Klamath scenes: kladwtwn 3 corners with
  2 grazing rays each; klagraz, klatrap, klacanyn, klatoxcv, klaratcv e0
  and e1 none (one of 10 rays); klamall's left are its mesh fences (clear on
  purpose) and 4 single rays; klaratcv e2 keeps two cave corners (66 and 20
  rays of ~700), cave walls rather than buildings: left for 081. Game view
  0 px on kladwtwn and klamall. Exporter tests 52/52.
- Before | after refreshed in `runs/076-walls-roofs-algorithm/corners-before-after.jpg`.
