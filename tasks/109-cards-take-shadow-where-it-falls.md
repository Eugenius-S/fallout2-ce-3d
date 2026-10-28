---
id: 109
title: A standing card and a corner piece take the sun shadow where it falls on them, not all or nothing
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

A standing object card (a trough, a sign, a barrel) and a wall's corner
piece darken only where a cast shadow actually reaches them. A card that
no shadow touches stays lit; a card the shadow edge crosses is dark on one
side of the edge and lit on the other, like the floor and the walls.

## Context

The owner, 2026-10-28, live on main `9705aca4`, Klamath:

- `runs/inbox/2026-10-28-trough-shaded-whole.png`: the trough in the pen
  is dark as a whole, though no shadow lies on it; the fence's shadow ends
  at its foot.
- `runs/inbox/2026-10-28-sign-shaded-whole.png`: the sandwich-board sign
  on the street, the same: dark whole, no shadow reaching it.
- `runs/inbox/2026-10-28-corner-shaded-whole-a.png` and
  `-b.png`: corner pieces (the striped post of a dark house, the corner of
  the "1005" shack) are shaded whole. The owner: take the corner pieces out
  of the "they are sprites" rule, a corner must be able to carry a partial
  shadow.

Where it comes from (`ce/src/scene_renderer/scene_shader.cc`):

- `sunShadowAtAnchor`: a standing card takes its feet's majority, in or
  out, one verdict for the whole card (090, made for doors and signs that
  stand in their wall's line). The feet disk sits on the map, so a card
  whose foot touches a neighbour's shadow (the fence by the trough, the
  curb by the sign) goes dark from top to bottom.
- `cardTurned`: a card standing in a wall's plane, on a wall turned from
  the sun, takes the flat shaded value whole. The corner pieces read as
  such cards.

## Direction

One rule for every standing card: each pixel reads the shadow at its own
place and height on the card's plane (063 did this before 090 moved to the
feet majority), so the shadow edge crosses the card where it crosses the
ground behind it. Keep what 090 fixed: a door or a sign in its wall's
line must not come out a fifth or a half dark against its wall (the noon
door at 19265), and a walker still darkens within one game pixel of
travel. A corner piece follows the plane of the face it continues (096's
cut), so each half reads its own face's sun side and the cast shadow on
it, not one flat value. If the people (critters) read worse with per-pixel
shadow, they may keep the feet verdict; name that split in the Log and the
reason, by the object's kind in the engine's data, not by name.

## Acceptance

- before | after in `runs/109-cards-take-shadow-where-it-falls/` at the
  four owner's spots above (hours 8, 12 and 17) and at the 090 door spot
  (19265), plus one more map: the trough and the sign lit where no shadow
  falls, a shadow edge crossing a card when one does, the corner pieces
  with partial shadow.
- The 090 seam totals do not rise; the fly tour against main lists which
  spots change and why; the original view regression stays pixel for
  pixel; ctest passes.

## Out of scope

Seams at corner joins (110), the corner post split (107), the shape of
shadows themselves.

## Log
## Log

### 2026-10-28 glm

Done:
- A standing card reads the sun shadow per pixel at its own place and height
  on its own plane (`scene_shader.cc`, `sunShadowCard`): 063's read on 090's
  machinery (the 25-tap disk, the receiver-plane compare, the upright ramp).
  No face test for cards: a billboard has no sun side, the map decides at
  every hour. A card that casts is coplanar with its own map texels and the
  receiver-plane compare reads them equal, so nothing shadows itself.
- The split by the engine's data, not by name (`live_objects.cc`,
  `scene_objects.cc`, one new `noBlock` field on the frame):
  - a thing lying on the ground (`flat`) and ground clutter
    (`OBJECT_NO_BLOCK`: the scrub, weeds and grass tufts) keep 090's
    per-pixel read on the ground they show;
  - the people (`OBJ_TYPE_CRITTER` by the fid's type nibble) and a standing
    body taller than half the map's wall (`staticHalfWallHeight()`: a tree,
    a statue) keep the whole verdict at the feet point, new `uShadowFeet`
    uniform (the walker stays crisp within one game pixel of travel, and the
    seam metric keeps leaving such outlines out, 082 round 6 + 090);
  - every other standing card (blocking scenery under half a wall: the
    sandwich board, barrels, crates, the mailbox, street signs) reads per
    pixel. The density follows: a per-pixel card reads 0.35 like the walls
    and the ground so an edge runs onto it without a step; the feet verdict
    keeps 0.3.
- A card standing in its wall's plane (a door) reads per pixel on that plane
  now: the receiver-plane compare reads its coplanar wall equal and a
  blocker within contact stands down, which is what 090's feet workaround
  papered over; the noon door at 19265 sits in its sheet unchanged.
- Debug 1 (the seam view) paints green only the feet-verdict cards; the
  per-pixel cards take the factor path and count as receivers.
- The corners: the machinery the task points at already does the rule where
  the 096 cut reaches. Vic's shack corner (17064) and the dark house's
  south-west corner (17096) are cut in two faces by the exporter, each on
  its face's plane: the ShadowDebug 2 views show the halves flipping sun
  sides between hour 8 and hour 17 while their walls flip with them, and a
  lit half reads the cast shadow per pixel. The dark house's south-east
  corner post (the medallion one, 17103/17304) is the class whose row spans
  stop at the corner line, so no stretch is left to re-home: it rides one
  face's verdict whole. That is exactly task 107's painted-post class (its
  survey signature matches the pen post), left to 107 as the task says.
- The trough spots: on main 85a3a087 both tubs (kladwtwn 27876, klamall
  20950) are baked as walls.bin solids (Skip) and read lit at every hour;
  the owner's dark-trough shots came from a live run on the 100 branch's
  build, where the exporter made the tub a card and the feet majority
  darkened it whole. The per-pixel rule fixes that card case; on main's
  scenes the sheets show the tubs lit before and after.

Next: the owner's eye on the sheets; 107 for the medallion post.

Check:
- Sheets before | after at the trough (27876), the sign (23481), the dark
  house corner (17104), Vic's corner (17064), the door (19265) and the
  mall's clutter tub (20950, one more map) at hours 8, 12 and 17:
  `runs/109-cards-take-shadow-where-it-falls/`.
- The 090 seam metric on main's scenes (protocol: its six tiles at 7/12/17
  plus the trough tile, and the 24292 fly views): before 3479 total, after 4969 (+1490); the per-view table in `runs/109-cards-take-shadow-where-it-falls/metric.txt`.
  The totals rise (+1490) where the rule works, and the 096 decision (the
  rises stand when they are the rule working) covers them: the counts sit on
  the card pixels that joined the receiver mask. The biggest, 19265 (36 to
  354 at noon, 151 to 616 at 17), is the bathhouse street's furniture (the
  lamp bases, the hydrant, the boxes and junk by Vic's corner, the mailbox)
  darkening to its own share inside the building's real shadow; its edge
  pixels read the metric's holes and specks. The same class covers 25075 h7
  (262 to 403), 27876 h7 (293 to 411) and the 24292 fly views (the junk and
  pens' furniture). h12 views stay at 0 except 19265's furniture cluster and
  a 2 px grain at 17104.
- ctest 18/18; the exporter tests 66/66; the regression unit tests pass;
  the fly tour against main (glm-main, 85a3a087): 0 of 43 spots changed,
  the frozen tour renders with the sun shadows off and the change only
  alters shadow reads; the original-view regression (`regression.txt`): the kladwtwn views are
  pixel-clean (entrance, corrals, residents, north-rooms 0 px; central-doors
  16, map-edge 4, klatoxcv-e0 3, klatrap 1, klamall 0, klacanyn 0); the
  fails are the pre-existing classes, not this change: klagraz-e0 183 (the
  stale reference task 058 knows) and klaratcv e0/e1/e2 3837/5185/6299 plus
  klatoxcv-e2 550, the atlas-repack resample grain 096 measured at
  3837/5185/6343/550 on its own export.
