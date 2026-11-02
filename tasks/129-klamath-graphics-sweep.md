---
id: 129
title: Klamath graphics sweep, find and fix every visible defect
milestone: M1
status: doing
owner: claude
suggested: claude
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

### 2026-11-01 glm: the cause group fixed on task/129-box-tops

Done: the corner caps (_corner_cap, the pieces that close the notch where
the sheared tops end at a corner) took one texel of their lender row's top
band, so every cap was one flat colour; in the cave art that reads as a
grey or pink-brown plastic sheet (finds 1 and 2), and some lenders' bands
sampled clear edge texels. Now the cap takes the art's own top band the
way the wall's top takes it (_wall_front): each corner samples the texel
the game drew it on; past the band's width the piece runs on in whole runs
of the band's columns (_corner_fill's repeat), the run whose brightness is
nearest the band's median (task 076); a column counts only opaque over the
rows the piece's depth reaches (the soft rise rows never take part). The
cap's lender is a row at least half as wide as the columns the piece
needs, else the widest banded row of its line, never a corner piece (task
076). Find list after the fix:

1. fixed (this PR): runs/129-box-tops/find1-klatoxcv-e1-19091-p45.jpg.
2. fixed (this PR): runs/129-box-tops/find2-klatoxcv-e1-22924-p45.jpg.
3. not a defect, unchanged.
4. open, next round.

Checks: the exporter unit tests and the 12 regression unit tests pass, the
18 ctest tests pass. The game view: the walls.bin game batches are
byte-identical on all eleven Klamath maps against the origin/main
exporter's; the regression run passes 12 of 16 spots with 0 changed
pixels - the 4 fails (central-doors, map-edge, klagraz-e0, klamall-e0)
reproduce on origin/main: their references were taken today from the
unmerged task/125-widen-back-views stack (ce a8d0b83e), whose engine
differs from origin/main. The fly tour against the origin/main exporter's
cache: 38 of 43 spots changed, every change is a corner cap taking its
band (the sheets in runs/129-box-tops/tour-changed*.jpg); 5 caps on
see-through band art (klamall's wire fences) are dropped - a solid sheet
over a see-through top read wrong.

Next: sweep round 2 (interiors, shapes-eval solo shots).
Check: the crops and sheets above.

- 2026-11-01 glm is out; box tops merged (#194). Back to todo for claude; continue from `task/129-klamath-graphics-sweep`.

### 2026-11-02 claude: sweep round 2, interiors and hours

Done: fresh export of the seven Klamath maps from origin/main's exporter;
the hero put inside every building (roofs open, `-HeroTile`) and shot from
the fly camera at -45, 0, +45, pitch 50, distance 8: kladwtwn 12 buildings,
klamall 14 spots over the town block, klagraz 1, klatrap 1 (the other klatrap
roof sits in the map void); kladwtwn at hour 7 over four tour spots. The caves
have no roofs and no interiors, round 1's tour covers them. Find list (1 to 4
are round 1's):

1. fixed (#194), 2. fixed (#194), 3. not a defect, 4. superseded by this entry.
5. klamall e0, view 21490 (hero 21292), fly 0 and +45, noon: the big pale
   green mattress-like thing with a pipe stands upright, leaning against the
   wall like a billboard; the floor items lying beside it (blankets) lie
   flat. Crop: runs/129-klamath-graphics-sweep/finds/klamall-e0-21490-p0-p45-mattress-stands.jpg.
   Cause: a floor-lying object's art is taller than the 20 pixel limit for
   cards that lie flat, so it takes the standing card path. Needs a "lying
   on the floor" kind that the owner's frozen shape labels do not have.
   Prototype: scenery "Junk", fid 0x20000a0 (97 x 81 px art, 5 on the
   Klamath maps, all kladwtwn/klamall); its look-alikes 0x20000a6 and
   0x20000a1 (leaning slabs, 68 x 108 and 42 x 73) really stand.
   Fix tried twice, both blocked (STATUS: BLOCKED, 2 attempts): a rule
   "drawn lying on the floor" from the art's silhouette. (1) the share of the
   convex hull's edge length that follows the 0.32..0.7 iso slope, over 170
   distinct item and scenery arts of the Klamath maps: the mattress scores
   0.17 (its pipe and burn marks break the hull) and tables, rocks and a
   pole score higher. (2) the same after a 7 px opening that drops the pipe:
   0.05, still below tables (0.46..0.61), cars and rocks. The silhouette does
   not separate a shaded slab from a box or a heap. Needs another cue (the
   proto, the shaded top face, or the FRM label set) or the owner's say.
   owner: which cue, since the art's own look is the only evidence left.
6. klamall e0, view 16098, fly -45, noon: the ragged dark moss fringe along a
   roof's edge lies flat on the roof (101's "fringe lies flat"). Crop:
   .../klamall-e0-16098-p0-roof-fringe-flat.jpg. Open, belongs to 101.
7. kladwtwn e0, view 24268, fly -45 and +45, hour 7: an orange translucent
   band over the ground at the map edge, ending in a straight slanted line.
   Crop: .../kladwtwn-e0-24268-h7-translucent-edge-quad.jpg. Already task 145.
8. No defects in: the wall tops through roofs (101), corner seams (110) and
   roof ends in the 36 interior and roof views; the klagraz and klatrap
   closer views (the near dead trees read as black blocks, the vanilla art
   magnified, not a geometry fault); the interiors of all kladwtwn and klamall
   buildings seen (floors, wall backs, doors, furniture, posters).

Next: nothing in the sweep has a rule-level fix this round (5 needs the
owner, 6 is 101, 7 is 145). Round 3: hours 7 and 18 over klamall and the
caves, and the 18:00 sheet of kladwtwn (shot, not read).
Check: the views were shot by a scratch script (not committed);
re-run with `dev-snapshot.ps1 -HeroTile T -ViewTile T -Fly -FlyPitch 50`.

### 2026-11-02 claude: round 2, remaining sweeps

Done: the 18:00 sheet read; klamall at hours 7 and 17 (four spots, -45 and
+45); the caves (klatoxcv e0-e2, klaratcv e0-e2, nine spots) at distance 4,
0 and +45, hour 12, klaratcv e0 again at hour 7.
9. Tool notes, not defects: the game clock's 18:00 has the sun below the
   horizon (strength 0, light as at night), so evening views use 17:00; the
   caves are lit by their own ambient, hour 7 and hour 12 give the same
   picture there.
10. klamall e0, view 27886, fly -45 and +45, hour 17: the ground past the map
    edge is a green translucent quad (orange at hour 7 on kladwtwn, find 7):
    the same map-edge plane tinted by the sun colour. Belongs to 145.
11. No defects in the 18:00 and 17:00 sheets (building shadows follow the
    sun, roofs and walls clean) or in the caves at close range: floors, rock
    walls, barrels, slime, props read right; the spots the tour uses look at
    rock and void, so the pictures are mostly dark.

Status: find 5 BLOCKED after two attempts (above), 6 is 101, 7 and 10 are
145; no fix PR this round.
Next: decide the cue for find 5 (owner or a new task); 145 closes 7 and 10.
Check: the crops in runs/129-klamath-graphics-sweep/finds/.

### 2026-11-02 claude: find 5, the owner's decision, blocked on the selector

Owner's decision on find 5: a short card lying flat on the floor needs no
geometry, stays a sprite and must not turn to the camera; a standing flat
item (the mattress, fid 0x20000a0, and its leaning look-alikes 0x20000a6,
0x20000a1) becomes a fixed vertical plane in world space at the angle its art
is drawn at, no extruded geometry; the sun hours stay as they are.
The mechanism is clear: `addStaticObject` (live_objects.cc) calls
`faceFlyCamera` for every standing card that is not a flat or a plane (door)
object; a fixed plane is the card's own game-view plane without that call.
What is missing is the rule that picks the cards of that kind on every map.
Tried on the 170 distinct item and scenery arts of the Klamath maps, each
against the three known ones: the hull's iso-edge share (0.17 for the
mattress), the same after dropping thin parts (0.05), and the wall-slope fit
of the lower and upper edges that `things.image_axis` uses (lower edge
0.08, upper 0.07 for the mattress; signs and mirrors score 1.0, tables 0.7,
rocks and weeds 0.4). None separates a mattress from the round things that
must keep facing the camera (trees, bushes, rocks, barrels), so any
selector built on the silhouette turns plants edge-on or leaves the mattress
as it is. STATUS: BLOCKED, no branch task/129-fixed-flat-cards was cut.
Needs: the set that must stop facing the camera named by something other than
the outline (for example the scenery kinds "Junk" and "Bed" by proto, or a
per-FRM flag in the frozen label set), or the owner saying which.

### 2026-11-02 claude: find 5 fixed (#222)

Owner exception, 2026-11-02 ("Да, списком"): find 5 may be solved by a
hand-kept list of FRMs, an explicit exception to the no-per-object-override
rule for this kind only.
Done: PR #222 `129: flat cards stay put`, merged. List in
tools/scene_export/shapes/fixed_cards.json (njunk1, njunk7, njunk2, one
reason each); baked mode FIXED_CARD, baked.bin version 7; walls.cc reads it;
`addStaticObject` skips `faceFlyCamera` for those cards. ctest 18/18,
exporter tests 89 OK, original-view regression 16/16 with 0 changed pixels,
fly tour vs main 2 of 43 spots changed (klamall 23145, kladwtwn 17104), both
the listed cards. Crops: runs/129-flat-cards/. `reviewed: no` (format change).
Next: 6 is 101 (flat roof moss fringe); 7 and 10 are 145 (map-edge quad).
Check: runs/129-flat-cards/mattress-floor.jpg.

### 2026-11-02 claude: find 6 fixed (#223)

Done: PR #223 `129: roof fringe stands`, merged. Roof tiles that are sparse dark
ragged growth (told by their art, no list: roof_fringe.py) stand up as two
crossed upright sheets under the fly camera; floor.bin version 3 and
roof_fringe.png carry them; the game's camera keeps the flat squares. Found on
klamall e0 and kladwtwn e0. Codex's depth comment fixed (roofs write depth
before the sheets). ctest 18/18, exporter tests 93 OK, original-view
regression 16/16 with 0 changed pixels, fly tour 25 of 43 spots changed (the
fringe on those two maps). `reviewed: no` (format change). Not changed: the
torn thatch edge tiles (solid roof with ragged holes) stay flat, they are roof.
Next: 7 and 10 are 145 (map-edge quad).
Check: runs/129-roof-fringe/klamall-16098-before-after.jpg.

### 2026-11-02 claude: find 6 reopened, #223 rejected

Owner: "This is not moss, it is concrete and rebar. They must be a continuation of the walls, going up. Stretch them the way walls are stretched. Right now they look like some trees."
Tried on branch task/129-wall-tops-up (pushed, no PR): the same art rule, each tile's art stood in the plane of the roof edge it sits on, first at the art's own screen position, then folded up about the edge (ground distance in from the edge becomes height). Both still read as rows of thin dark spikes, not concrete: the tile art is near black and mostly holes. Crops: runs/129-wall-tops-up/klamall-16098-attempts.jpg.
Also found: the dark torn roof-edge tiles (ids 436 to 467) are shingle roof with a dark ragged part in the same diamond, and stay flat; they are the flat dark streaks left on the roofs.
Next: owner decision on what the wall tops should look like (solid concrete from the art's silhouette, or a wall-coloured box).

### 2026-11-02 claude: find 6 fixed as a wall band (#224)

Owner, on the rejected trees: "Make the texture of these guts the way walls are done. Stretch it into the plane, then put it on the wall as a plane and stretch it over the plane. Around the perimeter, like a second storey of walls, roughly. Only the texture along the bottom carries this concrete and rebar."
Done: PR #224 `129: roof edges as a wall band`, merged. The wall-top tiles (same detection as #223) are split by the wall edge they sit on; the pieces of one wall line that follow each other are one run, and its art is unrolled into one strip (the picture sheared vertically until the wall line is flat: concrete along the bottom, rebar above). Under the fly camera CE stands each run up as a band on the wall top below the roof edge, in the wall's plane, half a storey high, empty texels cut out so the top is ragged, both sides drawn, one stretch per square (a hidden roof hides its stretch). floor.bin version 4 (runs); version 3 files still load, flat roofs. Not colour-matched to the wall. The game's camera keeps the flat squares. ctest 18/18, exporter tests 95 OK, original-view regression 16/16 with 0 changed pixels, fly tour 25 of 43 spots changed (klamall and kladwtwn roof edges only). `reviewed: no` (format change). Crops: runs/129-roof-edge-band/ (half and full storey in klamall-16098-band-half-and-full.jpg; the height is kBandStorey in roofs.cc).
Next: 7 and 10 are 145 (map-edge quad).
Check: runs/129-roof-edge-band/klamall-16098-before-after.jpg.

### 2026-11-02 claude: find 6, band on the wall (#226)

Owner: the band slides off to the side; it must stand in the wall's plane.
Done: PR #226 `129: the roof edge band stands on the wall`. Measured: the roof plane and the wall cap coincide (cap 1.872 to 1.887 against 1.880), so there was no vertical error; the roof edge line lies 0 to 0.25 tile from the wall's outer face (the wall body is 0.215 to 0.28 thick around a line the roof grid only approximates), and 7 of 29 runs on klamall e0 (1 of 20 on kladwtwn) have no wall under them or one a whole tile away, where the band floated. The exporter now matches each run to its wall by rule (wall_under in roof_fringe.py) and writes the outer face plane, cap height and span into floor.bin 5; runs with no wall stay flat roof. Crops in runs/129-band-on-wall/ look nearly the same as before because the old error was small except on the floating runs.
Next: if the owner still sees a slide, ask for the exact roof and angle.
Check: runs/129-band-on-wall/klamall-16098-turns.jpg.
