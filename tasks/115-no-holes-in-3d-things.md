---
id: 115
title: No holes in the 3D things: transparent texels take the colour of the edge
milestone: M1
status: done
owner: glm
suggested: glm
priority: high
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
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

### 2026-10-29 glm
Done: pairs viewed by eye, task closed. Acceptance set in
`runs/115-no-holes-in-3d-things/`: tub-game.jpg (17 px changed at the fixed
game camera), tub-flight.jpg (39 px), junk-boxes.jpg (50 px), junk-nook.jpg
(24 px), inside-wall.jpg (2761 px, klamall wall inner faces and the dark
door panel), window-slot.jpg (the owner note), regression-corrals.jpg
(4797 px) and regression-klamall.jpg (12388 px) against the old references,
changed.jpg plus changed-2..5.jpg (the fly tour, 5 pages). The 061 caveat:
njunk art carries no real holes; 061's junk holes were fitter seams and 075
closed them, so both junk-pile pairs show the piles themselves
pixel-identical, and the few changed pixels around them are fence plank,
crate and car pinholes. The owner's tub holes
(`runs/inbox/2026-10-28-tub-holes-iso.png`) sit on faces the fixed cameras
catch at a glancing angle: at our cameras the fill shows as single pixels on
the rim, faucet and basin.
Checks: ctest 18/18, exporter unittests 70, regression unittests 12.
Rebased on origin/main `0ebfd1f5` (#160 sky), rebuilt, regression references
retaken with `-Update` in one pass after the rebase. Note: `-Update` mode
skips the old-reference comparison by design (the log's "0 changed pixels"
is a placeholder); the counts above are old-vs-new reference pixel diffs,
`reference-before.png` kept in run folder `runs/regression/20261029-143502-070-42680`.
Fly tour vs origin/main, rendered in `fallout2-ce-3d-glm-main` (the main
checkout sits on the owner's local `look/2026-10-29` branch, so the tour
base is a detached worktree at `0ebfd1f5`, sky included, no sky caveat
needed): 38 of 43 spots changed, every diff at pinhole scale (tens to low
thousands of px per turn, on walls, fences, dark panels), no layout or
geometry shifts on the sheets.
Next: task 116 (stove and lockers as solids), its depends-on is this task.
Check: the sheets in `runs/115-no-holes-in-3d-things/`; tour sheets
`build/tour/sheets/changed*.jpg`; regression run
`runs/regression/20261029-143502-070-42680`.

### 2026-10-31 gpt review
Done: independently reviewed the exporter fill rule against current
`origin/main` `b2223f39`. The changed pixels are localized to enclosed
transparent pixels on wall and solid textures; open border-connected cuts
remain transparent. The tub's open leg shape remains visible. Classic output
is unchanged at all 16 regression locations. The 3D fly tour changed 38 of
43 spots, with changes confined to wall/solid texture holes and no geometry
or camera displacement.
Check: `ctest --test-dir build --output-on-failure` 18/18 on the current-main
CE build (the PR has no CE source changes); scene_export tests 70/70;
regression tests 12/12; PR regression 16/16; direct Classic comparison
16/16 with 0 changed pixels; current-main vs PR fly tour 43 spots, 129 angles
per side, 38 changed spots. Both sides used EXE SHA
`c7af38c23acc9d198e1a3e2543a75faafbd04860734e3e16910592828eecdb4f`,
`ce_commit=b8eb43c5`, and separate explicit `build/scene-cache` folders.
