---
id: 052
title: Shots and logs go to subfolders, not next to the exe
milestone: M1
status: todo
owner: none
suggested: glm
claude-only: no
user-check: yes
depends-on: []
reviewed: no
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
