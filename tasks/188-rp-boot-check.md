---
id: 188
title: A check that RP still boots, without committing RP
milestone: M1
status: done
owner: gpt
suggested: any
priority: high
tier: 1
claude-only: no
user-check: no
depends-on: [184]
reviewed: yes
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

### 2026-11-05 gpt
Done: Added `tools/rp-check.ps1`, a local checker for isolated RP scratch
copies, and named its invocation in `AGENTS.md`. The Release build from
`dd4f4102` booted and loaded six representative maps, with each loaded map
checked against snapshot metadata. A copied executable with a corrupted PE
signature was rejected with exit code 1. Evidence: `runs/188-rp-boot-check/`.
An optional wider sweep reached the `HUBSTASH` startup marker but did not
progress to a snapshot within 30 seconds; it is explicitly unverified.
Next: Wait for PR #288 CI on the final SHA.
Check: `tools/rp-check.ps1` positive and negative runs; `ctest --test-dir
build --output-on-failure` (20/20 passed).

