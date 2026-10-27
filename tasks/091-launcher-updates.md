---
id: 091
title: A launcher installs the game onto a clean Fallout 2 in one click and pulls updates from main
milestone: M1
status: todo
owner: none
suggested: glm
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

A player with a clean Fallout 2 (Steam, GOG or a plain copy) runs one small
launcher. The first time it finds the game, installs our build and makes the
3D data for the maps; every later time it checks GitHub, pulls a newer build
from main if there is one, and starts the game.

## Context

- The owner, 2026-10-27: an installer that puts CE and all of ours onto a
  bare Fallout 2 in one click, takes the current main from GitHub and pulls
  updates later.
- The repo is public, so release files download without a token.
  `.github/workflows/verify.yml` already builds the game on Windows; only
  checked work reaches main (protected, squash merge).
- The engine reads `master.dat`, `critter.dat` and `data/` from the folder
  its exe sits in; `fallout2.cfg` beside the exe holds the settings,
  `[3d]` among them.
- The 3D data (`mod-cache/scene/<map>/<elevation>/`) is made from the
  game's own files by `tools/scene_export/export.py` (`--game-root`,
  `--maps`, `--cache-root`); today it covers the Klamath maps. It is
  derived from Interplay's maps, so it never goes into a release: the
  launcher makes it on the player's machine from their copy.
- The plan the owner chose, in three parts:
  1. **Release.** A CI job on every push to main updates one rolling GitHub
     release, `latest`: a zip with `fallout2-ce.exe`, the default
     `fallout2.cfg`, a `version.txt` with the commit, and the exporter
     packed as a standalone exe (PyInstaller or the like) so the player
     needs no Python.
  2. **Launcher.** A small C++ exe in our CMake tree, no new libraries
     (WinHTTP or the like for the download). First run: find Fallout 2 (the
     Steam and GOG registry paths, else a folder picker), check
     `master.dat` and `critter.dat` are there, install. Every run: compare
     its installed commit with `latest`; if newer, download and unpack;
     after an install or update run the exporter for the maps it covers;
     then start the game. No network: start the game as it is.
  3. **The player's things stay theirs.** Their `fallout2.cfg` is never
     overwritten (missing `[3d]` keys may be appended), saves and other
     files are never touched, the original `fallout2.exe` stays.
- The licence (`LICENSE.md`, Sustainable Use) allows non-commercial
  sharing; no game file goes into the repo or a release.

## Acceptance

- The `latest` release is updated by CI after a merge to main and holds no
  game file and no `mod-cache` data.
- On a clean copy (a fresh folder with `master.dat`, `critter.dat` and
  `data/` copied in, never the owner's `game/`), the launcher installs,
  makes the 3D data and starts the game with 3D on a Klamath map: a shot
  and the launcher's log in `runs/091-launcher-updates/`.
- A second run against a newer release updates; a run with the network off
  starts the installed game; the player's `fallout2.cfg` and saves are
  byte for byte the same before and after (hashes in the log).
- The original view regression stays pixel for pixel.

## Out of scope

Building the 3D data inside the game itself (Claude, later), code signing,
the launcher updating itself, maps the exporter does not cover yet.

## Log
