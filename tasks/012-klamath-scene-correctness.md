---
id: 012
title: Correct scenes on all Klamath maps
milestone: M1
status: doing
owner: claude
suggested: claude
claude-only: yes
user-check: yes
depends-on: [011]
reviewed: no
---

## Goal

Every Klamath map looks right in 3D: whatever the 2D game shows in front is in
front, nothing missing, nothing extra.

## Context

The problem list in `runs/011-klamath-gallery/README.md`. Fix the conversion
rules (exporter and scene), not single objects; record each rule in the
roadmap's architecture notes only if another task depends on it.

## Acceptance

- Every problem in the 011 list is fixed or has a written reason why not and a
  follow-up task.
- Regression references updated with before and after images in the PR.
- The owner walked the Klamath maps and accepted them.

## Out of scope

Roofs (013), light.

## Log

### 2026-10-19 claude
Found by the owner in the game (KLADWTWN, after roofs, task 013), to fix here:
- `runs/012-klamath-scene-correctness/owner-wall-corner.png`: at the corner of
  a dark building a vertical strip of the wall art stands off the corner (a
  slab end or corner post drawn on the front plane).
- `runs/012-klamath-scene-correctness/owner-roof-corner.png`: the roof's
  corner and the wall top below it do not line up; the wall top runs on
  past the roof edge.

### 2026-10-19 claude
Done: empty ground and the space beyond the map are black, as in 2D (the
backing plane under the floor and the clear colour). Three regression spots
changed and were retaken; before and after in
`runs/012-klamath-scene-correctness/`.
Next: building corners. At the entrance store's east end the roof stops short
of the east wall's top, which stays visible as a dark band; in 2D the roof
covers it. Hypothesis: the 2D roof tile art overhangs its square by more than
the atlas cell margin (6 px of 88), so the cell cuts it. Check by measuring
the roof art's overhang against the cell, then the corner strip (dark store).
Check: `tools/regression.ps1`; `dev-snapshot.ps1 -Classic -ViewTile 25075`
against the same without `-Classic`.

### 2026-10-19 claude
Done: the 3D frames the map as 2D does. It put the center hex in the middle
of the whole screen; the engine puts it in the middle of its map window above
the interface, moved by the map's edge alignment (`_tile_offx/_tile_offy`).
The view now takes that point from the engine (`tileToScreenXY` of the center
hex, `GameView::screenCenter`): the 3D sat 50 game pixels lower than 2D.
New dev flag `--dev-parallel` (`dev-snapshot.ps1 -Parallel`): no perspective;
with it the 3D lies on the 2D pixel for pixel (best alignment shift 0, 0), so
a difference image shows real conversion errors. Roof art was measured: it
overhangs its square by at most 0.027 of a square, inside the atlas margin
(0.068), so roofs are whole.
Finding: the building corners the owner saw are the perspective. Without it
the entrance store's corner matches 2D exactly
(`corner-perspective.png`: 2D, 3D now, 3D without perspective); wall tops
are laid along the game's parallel view ray (`wall_sidecar._wall_front`), so
with perspective they slide a little against roofs and corner posts.
Next: the owner chooses: keep, milder perspective, or real 3D wall tops.
Check: `tools/regression.ps1` (16 spots retaken for the framing).
