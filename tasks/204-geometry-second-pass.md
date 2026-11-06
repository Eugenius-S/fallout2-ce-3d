---
id: 204
title: Geometry second pass: re-walk the 3D-live maps hunting and fixing flaws
milestone: M1
status: todo
owner: glm
suggested: glm
priority: high
tier: 3
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

A second pass over the geometry of every map that is live in 3D: walk
the spot list with fresh eyes, find what is wrong in the rendered
scene (a thing sunk, floating, poking through, split, z-fighting,
missing, an edge gap), and fix each find or hand it to its batch.

## Context

- The owner's ask (2026-11-06): a second geometry pass, hunting flaws
  and fixing them.
- Earlier sweeps each covered one slice: 075 the shapes algorithm, 129
  the Klamath graphics sweep, 168/170/171 the map batches, 190/191/192
  the roofs/furniture/vehicles batches. This pass re-walks the whole
  live set with the tooling those landed: the spot list in
  tools/regression/spots.json (154 spots, 111 maps), the owner shot's
  region mark (183) for pinning a find to hexes and objects, the
  teleport for reaching any spot in a live run.
- A find is pinned by spot, hex and frame before any fix: no
  silhouette-metric hunts (the beds' lean is the art, not a bug), and
  a proto's material line lies (the "Wooden Table" that is metal).

## Acceptance

- Every spot of the tour list reviewed; the pass's frames and the
  find register in runs/204-geometry-second-pass/ (one line per find:
  spot, hex, what is wrong, frame).
- Each find either fixed (its own PR, the gate green on the touched
  spots' stills) or folded into its batch task (190/191/192) or the
  inbox with the evidence link.
- No reference refresh without the owner's word.

## Out of scope

The floor extension pass (202), the regression harness (203), new
maps entering 3D (187), view-persistence (173, the owner's draft).

## Log
