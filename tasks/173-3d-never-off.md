---
id: 173
title: 3D never turns off on a local map; what cannot be built stands on a plane
milestone: M1
status: todo
owner: none
suggested: gpt
priority: high
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

- The owner, 2026-11-04. The world map, dialogs and menus stay 2D; this is
  about local maps only.
- Find every place the 3D view falls back to 2D or switches itself off on a
  local map: a missing or failed cache, an export error, an unsupported
  object or tile, a feature's "switches itself off if it fails to start"
  path that takes the whole view with it, map transitions (147). List them
  in the Log first.
- A map without a cache builds one on entry (or shows its 2D art on the
  ground plane while it builds), never the 2D view.
- An unsupported object: its art as a billboard (critters and items already
  stand as cards) or on a plane fixed to its tile; pick by the object's kind
  and write the rule.

## Acceptance

- The Log lists every fallback found and what replaced it.
- A scripted walk through a map of each finished area (Arroyo, Klamath, Den,
  Modoc, Vault City, Gecko, Broken Hills, New Reno) stays in 3D the whole
  way; the game log shows no fallback line.
- ctest and the regression pass.

## Out of scope

The world map, dialogs, menus.

## Log
