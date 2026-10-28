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

## Log

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
