---
id: 188
title: A check that RP still boots, without committing RP
milestone: M1
status: todo
owner: any
suggested: any
priority: high
tier: 2
claude-only: no
user-check: no
depends-on: [184]
reviewed: no
---

## Goal

A repeatable check that a CE build still boots RP and loads its maps, so
later changes do not break it.

## Context

- The Fallout 2 Restoration Project (RP) is never committed to the
  repository, not even in `runs/`. Agents never touch the owner's `game/`
  folder or `game/mod-cache`; installing RP into `game/` is the owner's own
  step, from 184's guide.
- RP leans hard on sfall: its scripts use many sfall opcodes and hooks and it
  reads its own `ddraw.ini` settings. CE implements only part of sfall
  (`ce/src/sfall_*.cc`), so most of the work is the missing pieces.
- Option 1: CI downloads the RP release into its cache (never the repo) and
  boots it headless with the test instance. Option 2, if CI cannot: a local
  check `tools/rp-check.ps1` on the scratch copy, named in AGENTS.md as a
  step before merging changes to sfall or script code. Pick and write why.

## Acceptance

- The check runs green on current main and fails on a deliberately broken
  build (one run, written in the Log).
- AGENTS.md or the CI workflow names it.

## Out of scope

Fixing what it finds.

## Log
