---
id: 184
title: The Restoration Project installs into a scratch game copy and boots in CE
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: high
tier: 1
claude-only: no
user-check: no
depends-on: []
reviewed: yes
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

### 2026-11-05 gpt
Done: Added the scratch-game-root option to `tools/dev-snapshot.ps1`, the install/backup/rollback guide at `docs/restoration-project.md`, and the runtime evidence in `runs/184-rp-install-and-boot/`. A background new-game run on the scratch copy started at Arroyo Temple and loaded Arroyo Village and Klamath Town. The scratch copy was made from the existing installed RPU 2.4.34 game; the official installer was not rerun. No RP files are committed.
Next: Wait for CI on PR #283 at the final SHA.
Check: CTest 20/20; scene-export unit tests 125/125; regression unit tests 12/12; live run logs and three map snapshots in `runs/184-rp-install-and-boot/`.

Official release checked 2026-11-05: RPU v2.4.34, published 2025-10-18. Installer: https://github.com/BGforgeNet/Fallout2_Restoration_Project/releases/download/v2.4.34/rpu_v2.4.34.exe (published SHA-256 `226b2252840a9230bb4ced1550becb2bd69d3580c15ee5c2148d5dc0a750f898`). Latest page: https://github.com/BGforgeNet/Fallout2_Restoration_Project/releases/latest.
