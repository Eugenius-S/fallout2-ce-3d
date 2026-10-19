---
id: 003
title: Regression snapshot test
milestone: M0
status: doing
owner: gpt
suggested: gpt
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

One command tells any agent whether the 3D picture changed, and shows where.

## Context

- `tools/dev-snapshot.ps1` runs the build in the background as
  `game/fallout2-ce-test.exe` (off screen, no focus), starts a new game on a
  map with `--dev-map`/`--dev-elevation`, takes one snapshot
  (`--dev-snapshot-once`, code in `ce/src/devtools/devtools.cc`) and closes
  only its own process. The PNG and a JSON land in `game/`.
- The hero always stands at the map entrance; the camera follows the game's
  view center. There is no way yet to look at a chosen place.
- Things move between runs: critters walk and idle, palette colors cycle
  (`ce/src/cycle.h`: `colorCycleDisable`), the camera glides.

## What to build

- A dev flag that centers the view on a given hex after the map loads (for
  example `--dev-view-tile=<n>`), with the glide finished before the shot.
- A dev flag for still snapshots: critters do not move or animate, color
  cycling is off, so two runs give the same pixels.
- `tools/regression/spots.json`: a list of spots (name, map, elevation, tile).
  Start with about six spots on KLADWTWN elevation 0 that cover walls, doors,
  furniture, people, the map edge; task 011 adds the other Klamath maps.
- `tools/regression.ps1`: for every spot takes a still snapshot with the
  existing build, compares it with `tools/regression/reference/<name>.png`,
  writes difference images and a summary to `runs/regression/<time>/`, and
  exits non-zero on a difference above a small tolerance. `-Update` rewrites
  the references; `-Spot <name>` runs one spot.

## Acceptance

- Two runs in a row on an unchanged build pass.
- A deliberate change (for example, not drawing the floor) fails with a
  difference image that shows it; reverting passes again.
- `-Update` works; the references are committed.
- `AGENTS.md`, section "Build and check", gets the command.
- The owner's open game is untouched during runs.

## Out of scope

CI integration (needs the game files, which CI does not have).

## Log

### 2026-10-19 gpt
Done: claimed task 003; read AGENTS.md, the roadmap and the snapshot launcher.
Next: add a fixed view tile and still-snapshot mode, then capture six Klamath
references and implement the regression runner.
Check: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/tasks.ps1`.

### 2026-10-19 gpt
Done: added `--dev-still-snapshot`, exact `--dev-view-tile`, fixed RNG seed,
stopped background tickers/color cycling and bypassed camera glide for stills.
`tools/dev-snapshot.ps1` accepts `-Still -ViewTile N -OutputDirectory DIR`,
matches snapshots by a unique run token, rejects non-windowed configuration
and cleans up only its own test PID in `finally`. Two tile-25075 launches have
identical pixels (`runs/003-regression-snapshots/still-a`, `still-b`). Release
build, 4 engine tests and 30 exporter tests pass.
Next: add six named spots, image comparison, references and failure/recovery
evidence; document the single regression command.
Check: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/dev-snapshot.ps1 -ViewTile 25075 -Still`.

### 2026-10-19 gpt
Done: added `tools/regression.ps1`, six spots and PNG/JSON references,
decoded-RGBA comparison, difference/highlight images and metadata validation.
All six captures passed the explicit `-Update` run. Added 9 image/validation
unit tests and the command to AGENTS.md. Fixed the launcher exit race by
waiting for its own PID and copying the test EXE only when its hash changes.
The gallery is `runs/003-regression-snapshots/gallery.jpg`.
Next: run two complete unchanged-build regressions, prove a deliberate floor
rendering failure is detected, restore/rebuild and prove recovery; then PR.
Check: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/regression.ps1`;
`py -3 -m unittest discover -s tools/regression -p 'test*.py' -v`.
