---
id: 100
title: The junk on the bathhouse table stands on the table, not in the wall
milestone: M1
status: done
owner: glm
suggested: glm
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The things lying on a table in the 3D view stand on that table, as in the
original, and nothing of them shows through a wall from outside.

## Context

- The owner, 2026-10-27, live in the 3D view (093 leftover 4):
  - outside, a grey box sticks out of the bathhouse's long alley-side wall
    near its south corner (`runs/inbox/2026-10-27-shadow-leftover-4b-bathhouse.jpg`);
  - roof off, it is the junk of a table inside the bathhouse
    (`-4c-bathhouse-inside.jpg`);
  - from above in flight (`-4d-bathhouse-top.jpg`) the junk is not on the
    table at all: a box, a plate and cans sit against the alley wall, and
    two grey pipes lie outside it in the alley.
- 093 compared the painted crates outside that corner and found them equal
  to the original; that was a different spot. This is a scene object
  (items on a table) placed away from its table.
- The same class, same day: a round stove inside a house has its top apart
  from its body and sticks out through the wall behind it
  (`runs/inbox/2026-10-27-stove-top-apart.jpg` inside,
  `-stove-through-wall.jpg` from the street). Fix both by one rule.
- Earlier work on the same bathhouse table: 075 (things' shapes) and the
  inbox line "A table in the bathhouse sticks through its walls".

## Acceptance

- Find which objects these are (map, tile, prototype) and why the 3D view
  puts them off the table; fix the cause, not this one table.
- before | after of the three owner views (outside, roof off, from above)
  and the original view at the same scroll, in `runs/100-bathhouse-junk-in-wall/`;
  the other tables with things on them in Klamath checked the same way.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

Shadows, the corner pieces (096).

## Log

### 2026-10-28 glm: the tour retook against main 85a3a087; the corner spots are resample grain, not the corner cut

- Merged origin/main (85a3a087: 148's bottom stop, 149's board tasks), rebuilt
  Release. Exporter unit tests, regression unit tests and ctest 18/18 pass.
  Main's cache re-exported from 85a3a087 in the detached worktree
  fallout2-ce-3d-glm-main (export.py --maps klamath --cache-root
  build/scene-cache; a bare export.py exports only kladwtwn, the first attempt
  stopped there and was rerun with the group).
- The tour retook against that cache: 31 of 43 spots, the same 31, and all
  four changed sheets are byte for byte the ones already committed (cmp), so
  nothing was replaced in runs/.
- What the changes are, from raw-shot diffs at the tour's own threshold
  (strongest channel over 8). The maps that free nothing (klatrap, klagraz)
  diff 0 px in every view, so any nonzero pixel is scene content:
  - Card-sized change in 3 spots: klamall 23145 (blobs 159x38 and 66x33: the
    freed Stack of Tires, Metal Barrels and Junk piles 34-38 units out),
    klamall 21700 (a 150x20 band: the same cluster from the fence side),
    klacanyn 15909 (37x16, 21x18, 17x17, 14x26: the freed Mountain card).
  - Every other changed spot is scattered 1-3 px specks, 23-310 px a view:
    the atlas repack resampling. The five long-building corners (24268,
    24292, 20268, 20292, 20690) are among them: biggest blob 3 px wide,
    before and after indistinguishable, the corner intact in both
    (tour-corner-24268-before-after-mask.jpg). The largest structured change
    anywhere is a 13x13 sliver of a dark hill silhouette's edge at 22958's
    frame top. 096's cut is in both caches; nothing of it differs.
- The scene.json diff against the fresh main cache names the freed props:
  kladwtwn/0 six (Junk x2, Girlie Poster x2, Stove x2), klamall/0 eighteen
  (Stack of Tires x4, Junk x8, Metal Barrels x6), klacanyn/0 one (Mountain),
  klaratcv/0 one (Machinery), klatoxcv/2 three (Computer x3). No table, door
  or bed among them.
- Which spot changes and what in it (the 31):
  - kladwtwn e0, all 19 (24268, 24292, 20268, 20292, 20690, 19265, 17467,
    17104, 19108, 24678, 22958, 24481, 17666, 20736, 14505, 25075, 23481,
    22885, 17699): resampling only; the six freed props lie 70-100 units
    from every view of this map.
  - klamall e0: 23145 and 21700 show the freed tires, barrels and junk
    cards; 13050, 12666, 15450, 27886, 28493 resampling only.
  - klacanyn e0: 15909 shows the freed Mountain card; 23869 resampling only
    (its count comes from the frame margins).
  - klaratcv e0: 21511 and 13511 resampling only; the freed Machinery is out
    of both views.
  - klatoxcv e2: 15065 resampling only; the three freed Computers are out of
    its view.
- Regression after the merge: every reference passes except the two
  pre-existing board items, klagraz-e0 (183 px) and klaratcv-e2 (6299 px),
  the same counts as on origin/main. reviewed stays no.

### 2026-10-28 glm: the tour retook against a main cache rebuilt from origin/main; the corner spots are atlas resampling, not the corner cut

- Merged origin/main (ff974f91: 104's farther bottom-edge camera, 105's README,
  107's task text); the exe rebuilt; exporter unit tests, regression unit tests
  and ctest 18/18 pass. The main cache the first tour compared against had been
  exported by pre-096 code (exporter 1.10.0, fingerprint ae2680...); a fresh
  export of origin/main (detached worktree fallout2-ce-3d-glm-main) into
  build/scene-cache-main differs. The tour retook against it: still 31 of 43
  spots, the same 31 as before the merge.
- The five long-building corner spots (24268, 24292, 20268, 20292, 20690) are
  not 096's corner cut: the cut is in both caches, and none of the freed
  objects stands within their view. Their 200-400 pixels per turn are single
  speckles on wall and roof textures across the whole frame: when the freed
  props' sprites leave the atlas, the page repacks and minified surfaces
  resample differently. Invisible at 1:1. A spot changes exactly when its map
  elevation's scene changed at all: klagraz, klatrap and the other cave
  elevations, which free nothing, are all "same".
- The rule's visible footprint, objects verified as owners of removed props
  (every removed prop's nearest object at distance 0.00 is plain-flag; no
  door, table, bed or vehicle lost a solid, their props only renumber with the
  atlas):
  - kladwtwn e0: the junk piles njunk2 16478 and njunk1 16479, the girlie
    posters GIRLP005 17078 and GIRLP017 17082, the stoves sstove2 20937 and
    26106. Seen from 19265, 17467, 17666, 17104, 17699, 20268, 20292, 20690,
    24292, 22958 and 20736 (the stove right under it). The other eight
    kladwtwn spots are resampling only.
  - klamall e0: Stack of Tires 12869, 12871, 18904, 21525; Metal Barrels
    19102, 19103, 19301, 19302, 19303, 19702; Junk piles 12918, 13504, 14514,
    19936, 21684, 21686, 21887, 21925. Seen from 13050, 12666, 15450, 21700,
    23145; 27886 and 28493 resampling only.
  - klacanyn e0: the painted mountains mount04 11296 and mount05 12501; 15909
    sees one, 23869 is resampling only.
  - klaratcv e0: the cave machinery mach01-mach05 (17540, 18140, 18740, 19541,
    19739); both gallery spots sit farther than a view away, resampling only.
  - klatoxcv e2: the computers glcomp02 (13696, 13700, 13702); 15065 is
    resampling only.
- The regression after the merge failed 12 spots. A build of origin/main
  itself fails 5 of them (klagraz-e0, klaratcv-e0, klaratcv-e1, klaratcv-e2,
  klatoxcv-e2): 104's camera moved the views main's references pin. The 10
  spots my branch failed beyond that were re-blessed from the merged build's
  captures: the six kladwtwn views and klacanyn-e0, where the wider bottom
  edge shows the freed cards or the resampling, and the three cave references
  retaken on top of 104. After the re-bless the regression fails only the
  pre-existing board items klagraz-e0 (183 px) and klaratcv-e2 (6299 px), the
  same counts as on origin/main. reviewed stays no.
- Exports from now on write the worktree's own cache (export.py --cache-root
  build/scene-cache, as this run's regression and tour already do); the
  shared game/mod-cache stays as it was.

### 2026-10-28 glm: the things were never on the table; the rule is the mapper's own flag word

- What the objects are: not table things at all. The "junk" is two pieces of
  `Junk` scenery (pids 0x02000110, 0x0200010F, FRMs njunk2.frm and block.frm)
  at hexes 16478 and 16479, in the alley north of the bathhouse, leaning
  against the alley-side wall; in the original's own view the wall hides them
  from every inside or front scroll (the classic capture agrees). The stove
  is `Stove` 0x020002AB (sstove2.frm) at 20937 by the alley door's house; the
  map carries three of them. The 3D put them off any table because the prop
  builder stood them up: the junk piles' silhouettes fit boxes (cover 0.98
  and 0.91), and the stove was forced into a drum by name
  (shapes/overrides.json), whose projection wraps the open lid around the
  cylinder: from any eye but the game's the lid reads as a disc torn off the
  body, and the junk read as boxes standing where the game shows nothing.
- The rule: scenery whose object flag word carries nothing but light-through
  and its own light (0x20000000, 0x20) is the mapper's loose clutter; it
  keeps the card everywhere, no solid stands for it (build_props). The flag
  word is engine data, not a name list: on kladwtwn e0 it flips exactly the
  two junk piles and the two stove objects of the 256 props, and touches no
  table, chair, bed, crate or barrel (all furniture carries structural bits,
  0x8000 and friends); both lifts of the map (things standing on tables)
  keep their solids. The stove override entry was removed with it: a fit's
  refusal stands, an exception may relax an outline by pixels, not stand up
  a solid the sprite contradicts. sstove1 (the other stove kind) was already
  refused by the fit before this task; the kind ends uniform.
- Exporter 1.11.0. Unit tests 66/66, regression tests ok, ctest 18/18.
- Evidence in `runs/100-bathhouse-junk-in-wall/`: outside | inside | alley
  before-after, the stove inside and from the street, the elevated pair,
  the classic captures byte for byte unchanged, the wall crop that mirrors
  the owner's 4b circle, and the fly tour sheets (31 of 43 spots changed,
  every one a loose-flag prop taking its card back: the wheelbarrow of the
  mall, cave clutter, the stoves).
- The regression references of the four spots the rule moves (klamall-e0,
  klaratcv-e0, klaratcv-e1, klatoxcv-e2) were updated with -Update; their
  captures now match the classic pictures better than the solids did.
  Four other spots fail the regression before and after this task
  (klagraz-e0, klaratcv-e2, klatoxcv-e0, klatoxcv-e1): a run at 08:40,
  before this branch, shows the same spots already failing. Left for the
  board.
