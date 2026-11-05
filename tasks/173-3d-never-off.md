---
id: 173
title: 3D never turns off on a local map; what cannot be built stands on a plane
milestone: M1
status: todo
owner: any
suggested: any
priority: high
tier: 2
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

On every local map the 3D view stays on. Nothing switches a map, a room or
an object back to the 2D picture; an object the exporter cannot build in 3D
yet stands as a flat picture instead: a billboard turned to the camera, or
its art on a fixed plane.

## Context

- Linked: 193 part B puts a fading black backdrop round the 2D screens (dialogue,
  the world map, the Pip-Boy, the rest) that stay 2D on a local map.
- Folded in (2026-11-04 inbox sweep), the inbox line of 2026-10-27: the
  list of unique objects that need their own 3D model (the big stone head in
  Arroyo, the Vault City gate, the Reno signs, the vertibirds, other one-off
  landmarks, vehicles and large signs). Each stands as a billboard or plane
  here first; name each with its map and shot, ordered by how much a player
  sees it, so the remodelling can follow.
- The owner, 2026-11-04. The world map, dialogs and menus stay 2D; this is
  about local maps only.- Folded in (2026-11-05, from 169): the upper elevations of some desert
  and coast encounter maps (desert5 e1, desert6 e1, shots in
  runs/169-encdesertcoast-in-3d/finds/) export with no floor art and no
  sprites (walls.bin 24 bytes, tiles.png empty, indoor=true), so the 3D
  view is black where the game's own picture shows dim content (2D mean
  46 vs 3D 0.8). The scene ships; the elevation needs its tiles and
  light built or a plane fallback.

- Find every place the 3D view falls back to 2D or switches itself off on a
  local map: a missing or failed cache, an export error, an unsupported
  object or tile, a feature's "switches itself off if it fails to start"
  path that takes the whole view with it, map transitions (147). List them
  in the Log first.
- A map without a cache builds one on entry (or shows its 2D art on the
  ground plane while it builds), never the 2D view.
- An unsupported object, by its kind (the owner, 2026-11-04):
  - a bulky unique object (the crashed vertibird lying on the ground) is a
    billboard turned to the camera, always;
  - a flat thing (a mattress, a rug) is a plane laid the right way and fixed
    in the world.
- Fences have their own working algorithm: do not touch them.

## Acceptance

- The Log lists every fallback found and what replaced it.
- A scripted walk through a map of each finished area (Arroyo, Klamath, Den,
  Modoc, Vault City, Gecko, Broken Hills, New Reno) stays in 3D the whole
  way; the game log shows no fallback line.
- ctest and the regression pass.

## Out of scope

The world map, dialogs, menus.

## Log

- 2026-11-04 audit: `scene_view::syncToMap` gave up after any floor, wall or
  projection load error, cleared the loaded map, and logged `stays 2D`.
  Changed this path to retry the floor sidecar and keep its 3D ground plane
  when walls/projection are missing or invalid. `activeView()` now reports the
  actually loaded level (`Floor`) so presentation draws the ground-plane view.
  A missing/invalid floor sidecar still has no ground geometry to show; this
  remains an unresolved fallback and needs an on-entry cache build or a
  ground-plane source from the live map. No claim that this case is fixed.
- Other inspected fallback paths: launcher reports 2D if the exporter is
  missing or export fails; `loadScene` rejects missing floor/walls/projection;
  map transition retries per map/elevation; unsupported object cards are
  skipped when rotation, FRM lock/data/dimensions or renderer insertion fails;
  missing/hidden hero only skips the hero card; explicit 3D-off setting,
  menus/dialogs/world map, movies and iso-disabled fades are deliberate gates;
  OpenGL startup failure uses the vanilla SDL renderer. The object-card and
  hero skips currently have counters/logs but no replacement for every skip;
  inventory and representative area walks remain outstanding.
- 2026-11-04 full regression completed: all 88 references matched (`0 changed
  pixels`). First combined 16-map runtime tour proved 9 stops in full 3D, then
  caught the game during map-transition state for 7 remaining snapshots.
  Independent fresh-game captures for the 8 requested areas all reported
  `scene_view=full`, `preview_availability=available`, and zero skipped object
  cards. Two follow-up area-tour attempts failed in the harness: Arroyo stopped
  at `arvill2` because it is absent from the worldmap registry, and Klamath
  reached `klacanyn` with 3D available but produced no third snapshot; the
  helper waited for its fixed count until timeout. Only those test processes
  were stopped. The attempted tour-readiness code change was reverted. Do not
  claim scripted area-walk acceptance; next diagnostic should make the helper
  stop promptly and report the actual route/availability instead of waiting
  for an impossible snapshot count. The eight independent map captures and
  full regression remain valid.
