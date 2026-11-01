---
id: 146
title: A background combat fixture: start a fight and capture each phase
milestone: M1
status: todo
owner: none
suggested: gpt
priority: high
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

The background test run can start a fight on any map and capture it, so a
combat change in the 3D view (135 and after) is checked without the owner.

## Context

- 135 step 1 (#211) keeps the 3D view on in combat, but nobody can capture a
  fight in the background: `tools/dev-snapshot.ps1` has no combat switch, and
  the toxic caves' geckos attack only after the hero walks in.
- Add `-Combat` to `dev-snapshot.ps1` and a matching `--dev-*` flag in
  `ce/src/devtools/`: after the map loads, start combat as the game does
  when the player presses A, then capture, in one process:
  1. the player's turn with the mouse over a hostile critter in attack mode
     (the to-hit number shows);
  2. right after one attack on it (the projectile or the hit, the floating
     damage text);
  3. the enemies' turn (a critter walking or attacking);
  4. after the fight ends (end combat).
  Each capture gets its JSON sidecar like the other snapshots, with the
  combat phase and the scene's availability (`preview policy`).
- Hostiles: take the map's own hostile critters; where a map has none,
  `-CombatTarget TILE` may place one documented test critter. Say in the
  Log which way each Klamath map uses.
- The flag never touches the owner's game or `game/mod-cache`, like the
  rest of the background run.

## Acceptance

- `dev-snapshot.ps1 -Map klatoxcv -Combat` and `-Map kladwtwn -Combat`
  each give the four captures in 3D; a strip of both in
  `runs/146-combat-fixture/`.
- The same with `-Classic` gives the 2D picture, for comparison.
- The help at the top of `dev-snapshot.ps1` describes the switch.
- ctest passes.

## Log

- 2026-11-01 Cut for 135 (no way to capture a fight).
