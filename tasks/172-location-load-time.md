---
id: 172
title: A location loads in 2 s from a warm cache
milestone: M1
status: todo
owner: none
suggested: gpt
priority: high
tier: 2
claude-only: no
user-check: no
depends-on: [127]
reviewed: no
---

## Goal

Walking into a local map with its scene cache already built shows it in 3D
in 2 s or less, measured from the map change to the first full 3D frame.

## Context

- The owner, 2026-11-04: fast loading is critical; a location loads in a
  couple of seconds. The cold cache build (the first export of a map) is
  one-time and is measured separately, not held to 2 s.
- Measure first: time every phase of a map change (the engine's own map
  load, reading the cache files, texture and buffer upload, shader and
  render target setup, the first frame). `ce/src/scene/scene_view.cc` loads
  the floor, walls and projection from `mod-cache/scene/<map>/<elevation>`.
- Likely wins: reading the cache once and in parallel, uploading textures
  without stalls, keeping shaders and render targets across maps.

## Acceptance

- A phase table (before and after) for kladwtwn, newr1, vctyctyd and one
  cave map in the Log and in `runs/172-location-load-time/`.
- A warm-cache map change at 2 s or less on each of them; the cold cache
  build time written next to it.
- ctest and the regression pass.

## Out of scope

Frame rate in play (157), settings changes (156).

## Log
