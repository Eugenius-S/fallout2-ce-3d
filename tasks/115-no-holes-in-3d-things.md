---
id: 115
title: No holes in the 3D things: transparent texels take the colour of the edge
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

The 3D solids (tubs, junk, furniture) and wall faces show no holes through
to the scene behind, in the game's angle and in flight. Where the art is
transparent on a face of a solid, the face is filled in the colour of the
nearest opaque pixels.

## Context

- The owner, 2026-10-28, live on main `ff5f4ed0`:
  - `runs/inbox/2026-10-28-tub-holes-iso.png` and `-tub-holes-fly.png`:
    the bathtub by the cow pen shows the ground through itself, even at
    the game's own angle.
  - `runs/inbox/2026-10-28-inside-holes.webp`: holes inside a building
    (also the walls-from-inside work of 102, PR #150).
- The owner's rule: extend every texture so nothing has holes; fill a
  transparent texel of a face by the colour of the last opaque pixel, an
  average of its neighbours (edge padding / dilation), not by a flat tint.
- History: 061 asked for the same patch and was folded into 075; the holes
  are still there. Read what 075 and 061 tried before choosing.
- Where a cut-out is the shape itself (the open space between a tub's legs
  seen at the game's angle), the game view keeps the original's pixels:
  the fill closes faces the 3D builder made, not the original's outline in
  the game camera.

## Acceptance

- before | after of the tub (game angle and flight), two junk piles and
  one interior wall in `runs/115-no-holes-in-3d-things/`.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

New shapes for things (116), wall backs (102).

## Log

- 2026-10-29, glm, work in progress, handed over mid task:
  - Survey: the njunk arts carry no texture holes (061's junk holes were
    fitter gaps, closed by 075). The real holes are enclosed transparent
    texels inside solid and wall art: 12984 across kladwtwn/klamall/
    klacanyn (Beds 842 each, Shelves 714, Cars 696, Bars 113, Crates 70,
    Bath Tub 70, Bookcases 162; 94 solids over 20 texels). The owner's
    `inside-holes.webp` sparkle is wall inner faces with pinholes.
  - Fix: `props.fill_holes()` in the exporter, flood fill from the border
    to find enclosed transparent texels, then onion peel: every new texel
    takes the rounded average of its already filled 4 neighbours, alpha
    255; border and shade band untouched. Applied at two choke points in
    `scene.py`: build_props for solids and their rests, and build_walls
    right after `walls.unshear` (backs, ends and trims read their
    material from the atlas image, so they inherit the fill; doorway
    openings stay transparent, they touch the border).
  - Unit tests: `FillHolesTests` in `test_export.py`, 3 tests, 70 pass.
  - Caches: `build/scene-cache` re-exported with the fill (kladwtwn,
    klamall, klacanyn); `build/scene-cache-before115` keeps the pre fill
    export for before | after pairs (use it as the `-SceneCache` of the
    "before" shot).
  - Shots taken, pairs not yet diffed or viewed: `runs/_115_shots/`
    {tub,tubF,tubS,bed,bedF,bedT,boxes,nook,inside}_{before,after}.
  - Left to do: view the pairs and pick the acceptance set into
    `runs/115-no-holes-in-3d-things/` (tub game angle and flight, two
    junk piles with the 061/075 note, one interior wall); ctest;
    regression (`-Update` retake: solid and wall textures changed, so
    expect diffs wherever filled holes were visible; keep `reviewed:
    no`); fly tour vs main; rebase on fresh origin/main first (113 sky
    merged while this ran); status: done; PR.
