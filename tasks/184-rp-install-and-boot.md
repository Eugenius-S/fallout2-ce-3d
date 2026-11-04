---
id: 184
title: The Restoration Project installs into a scratch game copy and boots in CE
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

RP is installed into a scratch copy of the game outside `game/`, and CE
starts a new game on it and loads maps. The owner gets a written guide (and,
if it is simple, a script) to install RP into their own game.

## Context

- The Fallout 2 Restoration Project (RP) is never committed to the
  repository, not even in `runs/`. Agents never touch the owner's `game/`
  folder or `game/mod-cache`; installing RP into `game/` is the owner's own
  step, from 184's guide.
- RP leans hard on sfall: its scripts use many sfall opcodes and hooks and it
  reads its own `ddraw.ini` settings. CE implements only part of sfall
  (`ce/src/sfall_*.cc`), so most of the work is the missing pieces.
- The scratch copy lives outside the repository or under an ignored path
  (for example `../f2-rp-scratch/`); the test instance runs on it through
  `tools/dev-snapshot.ps1`'s game root option or the equivalent.
- Take the latest RP release from its official source; write the version
  and the download URL in the Log.

## Acceptance

- `docs/restoration-project.md`: where to get RP, the install steps into an
  existing CE game, what to back up first, how to go back.
- If the install is a few file copies: `tools/install-rp.ps1` the owner runs
  on their own game folder, with a dry-run mode.
- The test instance on the scratch copy starts a new game (Arroyo temple)
  and loads two more maps; the log of that run in
  `runs/184-rp-install-and-boot/`.

## Out of scope

Filling sfall gaps (186 on), 3D of RP's content (187).

## Log
