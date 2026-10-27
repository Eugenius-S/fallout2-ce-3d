---
id: 092
title: Test games from several worktrees run at once, each in its own folder
milestone: M1
status: done
owner: claude
suggested: claude
priority: high
claude-only: yes
user-check: no
depends-on: []
reviewed: yes
---

## Goal

Several agents take snapshots with `tools/dev-snapshot.ps1` at the same
time, from their own worktrees, without waiting on each other and without
reading or overwriting each other's files.

## Context

- Before: one global mutex and a wait for any `fallout2-ce-test.exe`, so
  every test game in every worktree ran one after another. All runs wrote
  shots, logs and dumps into the shared `game/` folder, and the script
  picked the newest snapshot there.
- The game also writes its own state through the head of the xbase chain,
  `master_patches` (`game/data`): `MAPS\*.SAV`, `MAPS\AUTOMAP.DB`,
  `worldmap.dat` and the party protos in `proto\critters` and
  `proto\items`. Every new game erases the old ones. So test runs erased
  the owner's live game state, and a parallel run could load another
  game's saved map.

## Acceptance

- `F2CE_DEV_OUT` roots every file the build writes (snapshots, logs,
  `shadowmap-dump.pgm`, `debug.log`, bmp screenshots, the game's state);
  unset, the game behaves as before.
- `dev-snapshot.ps1` gives each run its own folder,
  `build/test-run/runs/<id>`, reads results only from there, copies them to
  `-OutputDirectory` and removes the folder (unless `-KeepRunFolder`). It
  queues only behind its own worktree's test exe.
- Two runs from two worktrees started at once both finish, and their
  pictures match the same shots taken alone pixel for pixel.
- `ctest` passes; `tools/regression.ps1` shows 0 changed pixels.

## Out of scope

- Several runs from one worktree at once (they share the exe and its
  config, so they queue).

## Log

### 2026-10-27 claude
Done: `ce/src/dev_output.{h,cc}` reads `F2CE_DEV_OUT` (`outputDirectory`,
`outputFile`, `gameStateDirectory`); `debug.cc`, `input.cc` and
`scene_renderer/shadow.cc` write through it. With it set, `gameDbInit`
(`ce/src/game.cc`) points `master_patches` at `<run>/data` (the config
keeps the shared path, `settingsRedirectMasterPatches`), opens the shared
`game/data` below it read-only, and `xbaseHideGameState` (`xfile.cc`)
hides the shared saved maps, automap, world map and party protos from the
run. The content config migration is skipped in that mode. New test
`ce/tests/dev_output`. `tools/dev-snapshot.ps1`: per-run folder,
per-worktree mutex and process wait, results copied out, `-KeepRunFolder`.
Other tools (`regression.ps1`, `tour/tour.py`, `shapes/eval/eval.py`,
`klamath.py`) already read only `-OutputDirectory`, so they needed no
change. Shared and read-only: `game/mod-cache`, `mods/mods_order.txt`;
`fallout2.cfg` beside the per-worktree exe is covered by the queue.
Timings (same machine, two worktrees, the other agents' games running too):

| shot | alone | parallel pair | pair wall time |
|---|---|---|---|
| still `kladwtwn` | 12.3 s, 12.2 s | 12.3 s, 12.7 s | 13.1 s (24.5 s in a row) |
| tour `klatrap:0,kladwtwn:0`, 3 shots | 16.1 s, 15.7 s | 15.4 s, 15.4 s | 15.6 s (31.8 s in a row) |

Every parallel shot equals its alone shot byte for byte (0 changed
pixels). Two runs from one worktree still queue: 12.2 s and 24.5 s, same
pictures. `ctest --test-dir build --output-on-failure`: 18/18 (after the rebase on 088).
`tools/regression.ps1`: 16 spots, 0 changed pixels.
Next: none.
Check: start `tools/dev-snapshot.ps1 -Map kladwtwn -Still -OutputDirectory
<a>` in two worktrees at once; both finish in about the time of one, and
`game/data/MAPS` gets no new `.SAV`.
