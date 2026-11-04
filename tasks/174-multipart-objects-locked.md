---
id: 174
title: Objects made of several parts stand and move as one
milestone: M1
status: todo
owner: any
suggested: any
priority: high
tier: 2
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

An object the game draws from several pieces (the crashed vertibird first)
stands as one in 3D: its pieces keep their places relative to each other in
the fly camera and on a turned view, with no gaps or drift.

## Context

- The owner, 2026-11-04: lock one part to the other.
- Find how the pieces are placed (separate prototypes on neighbouring tiles
  with offsets) and why they drift apart in 3D (each piece is a card or a
  shape of its own, anchored on its own tile).
- Fix by a rule: pieces the map places together are one group, built and
  turned around one anchor. Then scan every exported map for other groups
  of the same kind and list them in the Log.

## Acceptance

- The vertibird and every other group found show whole from the fly camera
  at -45, 0 and +45, with before | after crops in
  `runs/174-multipart-objects-locked/`.
- ctest and the regression pass.

## Out of scope

New 3D models.

## Log
