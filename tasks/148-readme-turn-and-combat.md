---
id: 148
title: The README shows the camera turn and combat in 3D
milestone: M1
status: todo
owner: any
suggested: any
priority: normal
tier: 3
claude-only: no
user-check: yes
depends-on: [146]
reviewed: no
---

## Goal

A player reading the README (and README.ru.md) learns that the camera turns
with `[` and `]` in 10-degree steps up to 30 (more with the unlimited
option) and that fights now play in 3D, each with one picture.

## Context

- Batch: the README (2026-11-04 inbox sweep). Folded in, the inbox line of
  2026-11-01: put the two night projectile clips from 097 (laser pistol,
  plasma rifle) side by side in one video, compressed for GitHub, in the
  middle of the README.
- 144 (#205, #207) added the turn steps; 135 step 1 (#211) keeps 3D on in
  combat. 105 wrote the README's feature list.
- Pictures: the 144 snapshots (`runs/144-camera-turn-steps/`) and one 146
  combat capture, compressed for GitHub, inside the repository's size rule.

## Acceptance

- Both READMEs carry the two features with a picture each, in the existing
  feature list's style; README.ru.md is the only Russian file.
- The runs-size check passes.

## Log

- 2026-11-01 Cut with the 146 pool.
