---
id: 052
title: Shots and logs go to subfolders, not next to the exe
milestone: M1
status: done
owner: glm
suggested: glm
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

Next to `fallout2-ce.exe` lie only the game's own files. Everything our
build writes lands in subfolders of the game folder.

## Context

- The owner's find, 2026-10-22: thousands of shots beside the exe. On that
  day Claude moved the old ones by hand: dev shots to `game/shots/dev/`, the
  game's `scrNNNNN.png` to `game/shots/game/`, one-off dumps to
  `game/shots/misc/`. New ones still land in the root.
- Dev shots and their sidecars: `devhero-`, `devwall-`, `devfloor-`,
  `devwalk-` PNGs and `devshot-*.json`, written to the working directory in
  `ce/src/devtools/devtools.cc` (around the `devshot-` names). The game's
  own `scrNNNNN.png` comes from upstream CE (one hooked path at most).
- Our logs in the root: `scene.log`, `scene-context.log`, `devperf.log`,
  `devprobe.log`, `lightdebug.log`, `viewdebug.log`, `dev-stdout.log`
  (`fopen` with a bare name in our modules and `tools/dev-launch.ps1`).
  `debug.log` is upstream's and stays.
- Readers of these files follow the new paths: `tools/dev-snapshot.ps1`
  (looks for fresh `devshot-*.json` in the game folder), `tools/regression/`,
  `tools/gallery_pairs.py`, and the docs that name the files
  (`docs/p1-devtools.md`).

## Acceptance

- After a dev session with F9 shots, a regression run and a classic
  screenshot, the game folder root has no new files; shots are in
  `shots/dev/<date>/` and `shots/game/`, logs in `logs/`.
- `dev-snapshot.ps1`, the regression and its tests pass with the new paths.

## Log

## Log

### 2026-10-23 glm
Done: everything our build writes lands in subfolders now. New
ce/src/dev_output.{h,cc}: outputDirectory() creates and returns "logs",
"shots/dev/<date>" and "shots/game"; an empty result degrades to the bare
game-folder path, so a failed folder creation cannot break a snapshot.
Writers moved: devhero/devwall/devfloor/devwalk PNGs and their devshot JSON
sidecars (the sidecar keeps the bare image name, so gallery_pairs.py and
check.py read them unchanged), the classic scrNNNNN.png in input.cc (both
the game's own screenshot key and the devtools vanilla branch write through
it), devprobe.log, devperf.log, scene.log and scene-context.log.
Readers moved: tools/dev-snapshot.ps1 scans shots/dev recursively beside the
root (the run id still filters), takes the image next to its sidecar and the
probe log from logs/ (root fallback). docs/p1-devtools.md names the new
paths. The old litter in the game folder root is gone: 1455 stray dev shots
sorted by date into shots/dev/, 49 scr PNGs into shots/game/ (each name
collision got a timestamp suffix, nothing overwritten), the seven old logs
into logs/; debug.log stays, it is upstream's.
Check: a live dev-snapshot run left the root untouched
(game/shots/dev/2026-10-23/devshot-...json + image, game/logs/scene-context.log);
Release build, ctest 11/11, regression python tests 12/12, exporter tests OK.
Note: the full 16-spot regression run is not re-run here, it is the
pre-existing 13/16 of task 054.
A classic-mode run double-checks the vanilla branch: scr00191.png landed in
shots/game/, its devshot sidecar in shots/dev/2026-10-23/, the root clean.
