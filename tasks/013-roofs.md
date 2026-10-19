---
id: 013
title: Roofs and the see-through circle around the hero
milestone: M1
status: done
owner: claude
suggested: claude
claude-only: yes
user-check: yes
depends-on: [010]
reviewed: no
---

## Goal

Roofs in 3D, hidden and shown exactly when the game hides and shows them;
walls and roofs in front of the hero see-through around him, as in 2D.

## Context

Neither the exporter nor the scene draws roofs yet. The game keeps roof
visibility in `gTileRoofIsVisible` and per-building roof hiding when the hero
walks in (`ce/src/tile.cc`). Roofs later cast sun shadows (task 021): decide
and write down what a hidden roof does to shadows.

## Plan (claude, 2026-10-19)

- Data is already exported: `scene.json` has `roof` (100x100 u16, cell index
  + 1 in the same `tiles.png` atlas as `floor`) and `hiddenRoof` (squares a
  solid or form stands for). Roof height: `proj.ROOF_PX` = 96 game px.
  Carry the roof layer in the floor sidecar (version 2, one atlas).
- Draw roof squares as a flat layer at roof height through the game view; the
  game's own placement (`squareTileToRoofScreenXY`) is the floor's minus 96 px,
  which this reproduces.
- Visibility per square from the live engine flags every frame
  (`gTileSquares[elevation]->tileFid` roof part: `TemporarilyHidden`,
  `AlwaysHidden`; `tile_fill_roof` flips them when the hero walks in,
  `object.cc:1532`) plus `tileRoofIsVisible()`: a 100x100 mask texture.
- Light: 2D roofs take the ambient light only (`tileRenderRoof`).
- See-through circle: 2D cuts walls and roofs around the hero with the egg
  (`gEgg`, `object.cc`; mask use in `tileRenderRoof`). 3D has none yet, so a
  wall in front of the hero hides him. Add it in the wall and roof shaders:
  the egg art as a screen mask at the hero, only for geometry nearer to the
  camera than the hero.
- Picking: check in 2D whether a roof stops object picks, and match it; the
  floor ray already passes through roofs.
- Shadows (for 021): a hidden roof keeps casting its sun shadow, so the inside
  stays shaded and lit by lamps. To be confirmed by the owner on snapshots.

## Acceptance

- Roofs on every Klamath map, matching the 2D roof tiles in place and look.
- The hero stays visible behind walls and under roofs through the
  see-through circle, as in 2D.
- Walking into a building hides its roof as in 2D; the roof toggle hotkey
  works; the mouse picks through hidden roofs.
- Regression spots inside and outside buildings.

## Out of scope

Shadows.

## Log

### 2026-10-19 claude
Done: research; findings and plan in `## Plan`.
Next: start when the dependencies are done.
Check: read `## Plan`.

### 2026-10-19 claude
Done: `floor.bin` version 2 carries the roof layer (exporter 1.8.0); roof
squares are drawn 96 game pixels above their squares, after everything else
and over it, as the engine paints them (`scene_renderer/roofs.cc`). Which
squares show comes from the engine's flags every frame (`tile_fill_roof`,
the roof toggle); the scene's square order matches the engine's (checked
against the map file: 875 of 875). The egg makes roofs see-through around the
hero with its soft mask. Picking ignores roofs, as in 2D. New dev flag
`--dev-hero-tile` (`dev-snapshot.ps1 -HeroTile`). The walls part of the
see-through circle moved to task 018.
Check: exporter tests and `ctest` pass; regression references updated (before
and after in `runs/013-roofs/`); snapshots there.
