---
id: 119
title: Lid drums render right: the raise lands on the lid, the drum matches its card, the window covers the stoves
milestone: M1
status: done
owner: glm
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

### 2026-10-31 glm: the cascade decoded; the rules stand and the regression stays 16/16 at 0 changed pixels

The "group cascade" was never groups re-composing (props.groups runs once over
the pieces; standing a thing up cannot change it). Reading the failing exports
against the baseline cache row by row showed all nine failing spots came from
ONE gate: `obj_type == OBJ_TYPE_ITEM -> seen = 1` flipped every ITEM container
(Dresser, Footlocker, Bookcase, Crate, Desk, Shelves, Fridge, Locker) from a
stood solid to a fly-only row. Each flip (a) swapped the game view's solid for
its card, (b) swapped the light bake's solid blob for the card blob
(build_light's `hidden - flyBoards` stopped excluding the board), and (c)
pulled the thing out of self.solids, which moved the rock faces and their
hiddenWalls downstream. The renderer fixes of 10-30 were already right.

The rules stand again, narrowed to what cannot move a game pixel:

- A container whose art animates and that main stood (not loose, not narrow)
  flies as its closed box: seen = 1, and every mark main's stood solid left is
  replicated - the board hidden, the thing propped, the prop blob, the blocks,
  the cast, the solids list entry - plus the new flyBoards mark (live_fly_boards)
  that keeps the light bake's blob exclusion. live_solids' centres now take every
  live row, so rests near a flipped container stay dropped as main's were; and
  baked_sidecar gives a live board in flyBoards a FLY_SKIP entry (the game keeps
  the animating card, flight skips it for the solid). Static containers stand
  exactly as main stood them.
- A thing main kept a card (loose clutter, a narrow thing) stands only as a drum
  with a lid (allow_lid threaded through props.fit into _from_floor's pick). Its
  game views keep the card either way, so the stand is game-invisible by design;
  its painted shadow row stays (fly_shadows) so the light bake reads the same
  sprite.
- The atlas pins the new stands' images to a trailing page of their own
  (atlas.pack(pinned)): every sprite any other row names keeps the texel it had,
  so the game's views sample exactly what they sampled before - no atlas-repack
  grain at all. Lesson: the scene's sprite table indexes references, the atlas
  indexes images; pin by image, and only images no other row names.

Checks: the 16 regression spots all pass with 0 changed pixels against the
EXISTING references (no retake); game triangles, object-resolved, byte-identical
on all 13 exported elevations; light.png, floor.bin, shadows.bin, tiles.png
byte-identical; the walls.bin game batches differ only in the object-table
renumbering (1268 -> 1269 style ids, resolved-identical). ctest 18/18, exporter
unittests 74. The stands: the bathhouse stove (klamall 24138), the three
downtown stoves (kladwtwn 20937, 22528, 26106), two Robot Receptacles
(klatoxcv), and the animating containers (lockers, fridges: FLY_SKIP entries)
fly as solids; everything else is bit-for-bit main's scene.

Acceptance set committed in `runs/119-lid-drums-render/`: the bathhouse stove's
drum shows through the fence gap in flight (stove-bathhouse-fly-pair.jpg; the
game's angle, camera turned two quarters, is identical either way,
stove-bathhouse-game-pair.jpg); the east stove drum stands against its building
(stove-east), the warehouse stove reads from above (stove-warehouse), the
downtown drum stands by its fence (stove-downtown); the mall lockers stand as
closed boxes on their shelf where cards lay flat (lockers-mall-fly-pair.jpg)
and the rat-cave locker flips card to box (locker-ratcave). Fly framing notes:
the orbit centres on the target's tile; the bathhouse needs FlyTurn 135 (45
looks from behind the building), the street stoves 225; FlyPitch 35-45 (25
puts the camera inside roofs at distance 6-9); -HeroTile on the target opens
interiors. The 10-30 shots stay (stove3-fly-rules-back.jpg = the black-drum
blocker pair; locker6-fly-no-solid.jpg = the pre-renderer-fix frame).

Done: the drums and boxes render at their card's size and place, the game's
own views keep their pixels everywhere (16/16 spots, 0 changed pixels), the
window covers every target. status: done; PR #167 carries the rule and the
evidence.

### 2026-10-30 evening glm: the ground truth is in; the blocker is the group cascade, not the renderer

Three render-side findings, all verified:

1. Coordinate space solved: the sidecar vertices are ground minus (50, 50)
   (floor.bin's 100x100 grid centred at (0,0)). Every earlier "the stoves are
   outside the exported window" read used the wrong space - all the targets
   are inside, their geometry reaches the sidecar (the bathhouse stove: 165
   fly vertices at its render position).
2. The stump was the wall cut: a fly-only drum skipped it (scene.py, the
   rows loop, row[15]==1) and the drum drew at its card's size and place -
   the lit pair (runs/119-lid-drums-render/, stove3) proved the shape.
3. The black was the light: a seen=1 solid has no light of its own. Fixed in
   the renderer (walls.cc): the load collects the rows a fly-only batch
   references (gFlyOnlyRows + gRowTotalRefs/gRowFlyRefs), staticObjectFlyOnly
   (tile, fid) returns true only when ALL of a row's triangles are fly-only,
   and scene_objects marks such objects full light per frame. Verified: the
   drum drew fully lit.

Then the real blocker surfaced, and it is not the renderer: with the rules
standing, 211 of klamall's 232 prop rows change. Stood-up things enter and
leave props.groups' composition, the neighbours' groups re-compose, their
fits and light models flip, and the game's own views change on eight
regression spots (87..1153 px a view: the crate by the locker bank flips its
light model twice as bright; the cave walls of klaratcv flip 2.5x when the
cave walls themselves get marked - 300 wall objects are fly-referenced by
design, their inner faces are fly-only). Two stabilisation attempts
(the seen!=2 guard for details, the all-fly-rows-only light marking) fixed
individual spots but the cascade moved. The rules stood down again: the
caches are content-identical to the pre-116 ones, the regression runs
16/16 spots with 0 changed pixels, ctest 18/18, exporter unittests 74.

The renderer fixes stay (they are correct and needed by any future version):
the fly-only row collection, staticObjectFlyOnly, the full-light marking,
and the wall-cut skip for fly-only drums.

Next (the design work, not a bugfix): make the standing rules group-safe -
fit and stand a loose thing or container standalone (its own group, no
re-composition of the neighbours), or gate them to single-piece groups; then
the light marking is already in. The lid fit's raise lands correctly (the
z1 lands on the cap: verified z1*HPX = 57 px = the cap band).

### 2026-10-30 glm, mid task, the rules are back and three findings are in

The rules are back on this branch (the loose-lid gate, the container gate, the
narrow-skip moved after the fit - the skip had been eating sstove3, a 52 px
NO_BLOCK thing). The scene rows stand: sstove3 drum a=23 px seen=1 (the wall
cut no longer shrinks it: a fly-only drum skips the wall-slab cut - its game
views are the card's, and in flight a little overlap beats a stump), the
klamall locker bank a=0.39 units seen=1.

Coordinate mystery solved: the sidecar vertices are ground minus (50, 50)
(floor.bin's 100x100 grid is centred at (0,0)); every earlier "the stoves are
outside the exported window" read was made in the wrong space. All targets
are inside.

The remaining blocker is in the renderer: at the verified fly camera the
bathhouse drum draws at the right place and size but BLACK (no light), and
the locker boxes do not draw at all (the frame is identical to the live card).
Suspicion: a seen=1 row's board is in hidden, and the fly faces reference the
board's object for lighting - the renderer finds nothing to light them with.
Next: ce/src/scene_renderer - how FLY_ONLY batches get their light and how
hidden boards interact with the fly pass.
