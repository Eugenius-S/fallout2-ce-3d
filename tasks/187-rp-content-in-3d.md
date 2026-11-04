---
id: 187
title: RP's new and changed maps build in 3D under the same rules
milestone: M1
status: todo
owner: none
suggested: gpt
priority: high
tier: 2
claude-only: no
user-check: no
depends-on: [184]
reviewed: no
---

## Goal

RP's new and changed maps and art export and show in 3D like the vanilla
ones: 3D never off, a plane or billboard for what cannot be built (173).

## Context

- The Fallout 2 Restoration Project (RP) is never committed to the
  repository, not even in `runs/`. Agents never touch the owner's `game/`
  folder or `game/mod-cache`; installing RP into `game/` is the owner's own
  step, from 184's guide.
- RP leans hard on sfall: its scripts use many sfall opcodes and hooks and it
  reads its own `ddraw.ini` settings. CE implements only part of sfall
  (`ce/src/sfall_*.cc`), so most of the work is the missing pieces.
- The exporter reads the scratch copy's DATs and mod folders in CE's own
  order; RP's art overrides vanilla art of the same name.
- Export into your own cache, never `game/mod-cache`.

## Acceptance

- Every RP map exports without errors; RP's restored areas get fly tour
  spots, kept in a separate spots file that the tour runs only when an RP
  game root is given (RP is not in CI).
- A walk through two restored areas stays in 3D.

## Out of scope

sfall gaps (186).

## Log
