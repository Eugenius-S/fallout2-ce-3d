---
id: 186
title: Fill RP's sfall gaps, the blocking and most frequent first
milestone: M1
status: todo
owner: any
suggested: any
priority: high
tier: 1
claude-only: no
user-check: no
depends-on: [185]
reviewed: no
---

## Goal

RP plays in CE: every gap from 185 that blocks play is implemented, then the
rest by how often they fire.

## Context

- The Fallout 2 Restoration Project (RP) is never committed to the
  repository, not even in `runs/`. Agents never touch the owner's `game/`
  folder or `game/mod-cache`; installing RP into `game/` is the owner's own
  step, from 184's guide.
- RP leans hard on sfall: its scripts use many sfall opcodes and hooks and it
  reads its own `ddraw.ini` settings. CE implements only part of sfall
  (`ce/src/sfall_*.cc`), so most of the work is the missing pieces.
- Work from 185's list and the tasks it cut; this task tracks the order.
  Follow sfall's own documented behaviour for each opcode and hook.
- New sfall code lives in CE's sfall modules, not spread through upstream
  files (AGENTS.md).

## Acceptance

- 185's tour on RP runs with no blocking gap left; the Log shows the gap
  count before and after.
- ctest passes; a vanilla game still plays (the regression passes).

## Out of scope

3D of RP's content (187).

## Log
