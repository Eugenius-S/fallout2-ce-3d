---
id: 013
title: Roofs
milestone: M1
status: todo
owner: none
suggested: claude
claude-only: yes
user-check: yes
depends-on: [010]
reviewed: no
---

## Goal

Roofs in 3D, hidden and shown exactly when the game hides and shows them.

## Context

Neither the exporter nor the scene draws roofs yet. The game keeps roof
visibility in `gTileRoofIsVisible` and per-building roof hiding when the hero
walks in (`ce/src/tile.cc`). Roofs later cast sun shadows (task 021): decide
and write down what a hidden roof does to shadows.

## Acceptance

- Roofs on every Klamath map, matching the 2D roof tiles in place and look.
- Walking into a building hides its roof as in 2D; the roof toggle hotkey
  works; the mouse picks through hidden roofs.
- Regression spots inside and outside buildings.

## Out of scope

Shadows.

## Log
