---
id: 206
title: The game hangs when entering the Enclave tower map
milestone: M1
status: open
owner: any
suggested: owner
priority: high
tier: 0
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

The owner, 2026-11-07: a fast jump to the Enclave tower map hung the game.
Find why entering a map can hang the game at all, and fix that class,
starting with this map.

## Context

- The owner plays from `game/` on the RPU mod set (debug.log lists 12 mods
  loaded). The hang happened on their install, normal launch.
- "Enclave tower": the Enclave sight on the world map (the oil rig side, or
  the Navarro tower); the exact map name needs one repro: travel there in
  the background test game, or `--dev-map` over the candidate names.
- Known missing-scene-data cases: some maps have no `floor.bin` in the
  owner cache (kladwtwn e1, artemple, vault15). A silent wait on missing
  scene data at map entry is the prime suspect for a game that just freezes.
- 196 built live checks that never hang for the test rig; the same watchdog
  idea (timeout + named missing asset) may belong on the map-entry path
  itself.

## Acceptance

- The background test game enters the Enclave tower map without hanging.
- A map entry that cannot build its scene fails loud (a timeout message
  naming the map and the missing asset) instead of freezing the game.

## Out of scope

- Making the map itself 3D when its export does not exist: that belongs to
  the encounter-maps batch, not to the hang fix.

## Log

- 2026-11-07: owner report in the morning; task cut the same morning.
