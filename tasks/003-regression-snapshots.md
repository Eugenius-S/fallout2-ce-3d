---
id: 003
title: Regression snapshot test
milestone: M0
status: todo
owner: none
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
