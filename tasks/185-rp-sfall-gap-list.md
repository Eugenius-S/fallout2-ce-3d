---
id: 185
title: Every sfall opcode, hook and ini key RP needs and CE lacks, counted
milestone: M1
status: todo
owner: any
suggested: any
priority: high
tier: 1
claude-only: no
user-check: no
depends-on: [184]
reviewed: no
---

## Goal

A complete list of what RP asks of sfall that CE does not implement, with
how often each gap fires and where.

## Context

- The Fallout 2 Restoration Project (RP) is never committed to the
  repository, not even in `runs/`. Agents never touch the owner's `game/`
  folder or `game/mod-cache`; installing RP into `game/` is the owner's own
  step, from 184's guide.
- RP leans hard on sfall: its scripts use many sfall opcodes and hooks and it
  reads its own `ddraw.ini` settings. CE implements only part of sfall
  (`ce/src/sfall_*.cc`), so most of the work is the missing pieces.
- Log every unimplemented opcode, hook and `ddraw.ini` key (a dev log line
  at the point CE skips it, if one is not there yet).
- Run: boot, a new game, then a short tour: Arroyo, Klamath, the Den and the
  areas RP restores (list them from RP's own readme).

## Acceptance

- The Log holds one line per gap: the opcode, hook or key; the count; the
  maps or scripts where it fired; whether it blocks play (a crash, a stuck
  script, a missing dialogue) or only changes a detail.
- The follow-up tasks for 186 are cut from this list in the same PR (one
  task per group of related gaps, the blocking ones first).

## Out of scope

Implementing the gaps.

## Log
