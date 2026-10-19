---
id: 016
title: Clean map edges on black
milestone: M1
status: todo
owner: none
suggested: gpt
claude-only: no
user-check: yes
depends-on: [014]
reviewed: no
---

## Goal

The map ends in straight edges on a black background instead of the ragged
teeth of diamond-shaped tiles.

## Context

- Floor tiles are diamonds, so the outer border of a map is jagged.
- Many maps limit the visible area: `ce/src/map_edge.cc` (Restoration
  Project's map edge rectangles). In 3D these limits are switched off for
  scrolling (`mapEdgeIsOverClippedArea`), but they still describe where the
  map is meant to end.

## Acceptance

- The scene is cut along straight lines: the map edge rectangle where the map
  has one, otherwise the straight envelope of the floor. Floor, walls and
  objects beyond the cut are not drawn; beyond is black.
- Before finishing, show the owner both variants of the cut on two Klamath
  maps (snapshots in the PR) and keep the chosen one.
- `[3d] clean_edges` (default on) with a menu label.
- Regression references updated with before and after images.

## Out of scope

Changing what the player may scroll to.

## Log
