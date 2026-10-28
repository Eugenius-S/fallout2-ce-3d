---
id: 091
title: A launcher installs the game onto a clean Fallout 2 in one click and pulls updates from main
milestone: M1
status: done
owner: glm
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

- 2026-10-27, glm: done, PR awaits review.
  - **Release (CI).** On every push to main `windows-game` stages the exe, the
    launcher, `ce.dat` (built from `ce/files/ce.dat` by `ce-dat-tool create`),
    the default `fallout2.cfg` and `version.txt` (the commit). The new
    `publish-latest` job packs them into `fallout2-ce-3d-win64.zip` and
    updates the rolling `latest` release (`gh release upload --clobber`;
    `version.txt` and the launcher also ride as separate assets). No game
    file and no `mod-cache` data in the payload.
  - **Standalone exporter.** `tools/scene_export/standalone.py` +
    `tools/scene_export/build_standalone.ps1`: PyInstaller onefile
    `scene-exporter.exe` (28 MB, numpy and Pillow inside). The whole
    `tools/scene_export` tree travels as data and is imported from the
    extracted copy, so every `Path(__file__)` lookup in vendor/fp and
    vendor/f2lib stays true; the stdlib surface is pinned by the entry's
    imports. New `export.py --check`: exit 0 when every requested map's
    elevation 0 scene exists, exit 3 listing the missing ones (map groups
    stay exporter knowledge; the launcher asks instead of guessing).
  - **Launcher.** `ce/src/launcher_main.cc`, CMake target
    `fallout2-ce-3d-launcher`, only Windows system libraries (winhttp,
    bcrypt, advapi32, ole32). Finds the game (GOG registry
    `GOG.com\Games\1207658924\path`, Steam `SteamPath` plus every
    `libraryfolders.vdf` library, then a folder picker; `--game-root`
    overrides everything). Pulls `version.txt` from the release, downloads
    the zip when it differs, unpacks with `System32\tar.exe` (fallback
    `Expand-Archive`) and copies: exe, `ce.dat`, exporter overwrite;
    `fallout2.cfg` only when missing; `version.txt` written. Scenes: asks
    the exporter with `--check`, exports on fresh install, update or missing
    scenes. Waits for the game, then hashes `fallout2.cfg` and the whole
    `data/savegame` tree before and after and logs both. Loopback URLs
    bypass the system proxy; the URL port is honoured (the first working
    draft connected to port 80 and could not see a local test server).
  - **Evidence** (`runs/091-launcher-updates/`), on a fresh copy at
    `<game>` (master.dat, critter.dat, data/ only;
    `game/` was never written to), fake release served on 127.0.0.1:8119:
    - `run1-install.log` + `run1-3d-kladwtwn.png`: fresh install, 7 Klamath
      maps exported by the frozen exe, 3D scene on kladwtwn, cfg and saves
      hashes unchanged across the run.
    - `run2-update.log` + shot: a091 -> b091, "fallout2.cfg: kept (the
      player's file is never overwritten)", hashes unchanged.
    - `run4-update-c091.log`: update with `--no-start`.
    - `run5-offline.log` + shot: release unreachable, "offline: starting the
      installed game as it is", exporter `--check` reports "scenes present",
      no re-export, game starts with 3D, hashes unchanged.
    - An older exporter in place (no `--check`) makes the launcher fall back
      to a full export: safe on both sides.
  - Notes: on first run the game itself creates `ddraw.ini`, `debug.log`,
    `logs/`, `mods/mods_order.txt` beside it (its own doing, not the
    launcher's). Closed through the player's X (WM_CLOSE), the dev-snapshot
    game exits 0xC0000005; the launcher logs the code and still reports the
    hashes.
  - Checks: ctest 18/18; exporter unittest 64/64; regression corrals 0
    changed pixels.
- Review fix (PR #128): the zip never carried `scene-exporter.exe` (it was
  built into `build/exporter-standalone/` while only `staging/*` got packed),
  so a fresh install had no exporter and could not make the 3D data.
  `publish-latest` now copies the exporter into `staging/` before
  `Compress-Archive` and the pack step lists the zip and fails if any of the
  six files (`fallout2-ce.exe`, `fallout2-ce-3d-launcher.exe`, `ce.dat`,
  `fallout2.cfg`, `version.txt`, `scene-exporter.exe`) is missing; the check
  runs before the upload, so a broken zip never reaches the release. The
  check block was verified locally under both Windows PowerShell 5.1 and
  pwsh: a six-file zip passes, a zip without `ce.dat` fails naming it.
