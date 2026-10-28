---
id: 108
title: The 3D bottom stop follows the map's drawn ground, with no black strip under it
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

At a map's bottom edge the 3D camera goes down as far as the map has drawn
ground and no farther: the exit zone is in view and nothing black shows
between the ground and the interface bar.

## Context

- 104 (PR #145) moved the 3D bottom stop down by a fixed 24 map pixels in
  `ce/src/map_edge.cc`. The exit zone is better in view, but at kladwtwn a
  black strip now shows above the interface bar
  (`runs/104-camera-bottom-edge-farther/kladwtwn-before-after.jpg`, top
  right); 48 pixels gave a wider strip. A fixed amount fits one map, not
  all: where the drawn ground ends differs per map and along the edge.
- The project rule (AGENTS.md): a 3D fix is a rule for every map, never a
  number tuned on one.

## Acceptance

- The 3D bottom stop is computed per map from where its drawn ground (the
  lowest row of floor the 3D view draws along the bottom edge) ends, so the
  screen's bottom above the interface bar reaches that ground and never
  past it. The fixed 24 goes away. Name the rule in the PR.
- Bottom-edge shots in the 3D view on kladwtwn, klagraz and one map of
  another town, before|after, in `runs/108-bottom-stop-follows-ground/`: no
  black row above the bar, the exit zone in view at least as much as with
  104.
- The other three edges and the original view stay as they are: the
  original view regression pixel for pixel; ctest passes.

## Out of scope

The other edges, the fly camera, filling the void past the map with art.

## Log