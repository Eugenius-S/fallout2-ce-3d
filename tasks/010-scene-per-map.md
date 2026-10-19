---
id: 010
title: A 3D scene for any map and elevation
milestone: M1
status: doing
owner: claude
suggested: gpt
claude-only: no
user-check: no
depends-on: [002]
reviewed: no
---

## Goal

The exporter builds, and the game shows, the 3D scene of whatever map and
elevation the player is on, starting with the seven Klamath maps.

## Context

- `tools/scene_export/export.py` builds one hard-coded map: `MAP_NAME`
  (kladwtwn) and `ELEVATION` (0), into
  `game/mod-cache/scene/<map>/<elevation>/`.
- The engine loads exactly `mod-cache/scene/kladwtwn/0` (look for
  `"kladwtwn"` in the bridge code after task 002) whatever map is loaded.
- Klamath maps: KLADWTWN, KLATRAP, KLACANYN, KLAGRAZ, KLAMALL, KLARATCV,
  KLATOXCV. `--dev-list-maps` prints all maps.

## Acceptance

- `export.py --map <name> --elevation <n>` builds one scene; `--maps
  klamath` (or a list) builds every elevation of the Klamath maps. The existing
  exporter tests pass and new ones cover the arguments.
- The game loads the scene of the current map and elevation, and reloads it on
  a map change, an elevation change and a save load.
- A map or elevation without a built scene, or with a cache that fails to
  load, is shown in 2D with one log line naming the reason; the game does not
  crash. Coming back to a built map shows 3D again.
- The regression snapshots pass; a background snapshot on each Klamath map
  shows its scene (not kladwtwn's).

## Out of scope

Fixing how the scenes look (tasks 011, 012); maps outside Klamath.

## Log
