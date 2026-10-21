---
id: 035
title: Boards, shelves and junk piles stand as the donor stands them
milestone: M1
status: done
owner: claude
suggested: claude
claude-only: no
user-check: yes
depends-on: [033]
reviewed: yes
---

## Goal

From the air the things that are neither cards nor boxes read as things: a
notice board or a shelf is a sheet with its top and sides, a low pile of junk
lies on the floor, a heap of crates has its depth, as in the donor's viewer.

## Context

- The owner's screenshots from the air (2026-10-20, before 033):
  `runs/035-donor-things/owner-*.png`. A notice board leans as fanned planes
  (it should be two planes: the top and the sides); heaps of crates and junk
  looked better in the donor's viewer; a bed shows a corner sticking out
  behind it (`owner-31.png`, right of the headboard).
- The donor decides how each thing stands (`vendor/fp/things.py`, `stand`):
  a sheet along an axis for thin things (shelves, boards, counters), a card,
  or flat on the floor for low piles; `props.py` `groups`/`compose` put back
  together things the artists cut into several objects. Our live objects
  ignore that choice and are all cards; find what reaches `scene.json` and
  use it.
- Start by comparing the owner's spots in the donor's viewer and ours.

## Acceptance

- The owner's spots, from the air, before | after in `runs/035-donor-things/`;
  the bed's corner gone.
- Regression 16/16, or changed references with before | after.

## Out of scope

Walls (034). Real 3D models (M2).

## Log

### 2026-10-20 claude (part 1)
Done: what the donor lays on the floor (`things.stand` -> "flat",
`scene.decals`: hay, trash, goo, planks, mattresses) is now drawn flat when it
is an item or scenery (critters live and die, they stay standing): `baked.bin`
version 3 gives such objects the mode FLAT, and the engine (which still reads
version 2) draws their live cards flat, as it draws the game's FLAT objects.
In the game's view a pile no longer pokes through the wall the game paints
over it: the regression's `entrance` changes at one fence post, reference
retaken (`runs/035-donor-things/regression-entrance-before-after.png`); the
other 15 cases are unchanged. Exporter tests 40/40, unit tests 10/10.
Left: the notice board and the bed's corner from the owner's screenshots need
their spots found; heaps of crates are cards, turned to the fly camera by 033.
Next: PR (part 1), then find the owner's spots.
Check: `py -m unittest discover -s tools/scene_export`, `tools/regression.ps1`.

### 2026-10-20 claude (closed after part 1)
Done: part 1 above. The owner's second round from the air
(`runs/035-donor-things/owner-2/`: the notice board, a hydrant with a hole, a
round table's stem, doors and gates turned to the camera, the bed's extra face,
chairs, a crate's stray piece, the computer stand, the stove's flickering top)
is not fixed here: the owner wants these solved by one algorithm, not by hand
one by one. They are cases of the eval set in
`docs/2026-10-20-shapes-and-rotation-design.md` and its tasks 036 to 039.
Check: `tools/regression.ps1`.
