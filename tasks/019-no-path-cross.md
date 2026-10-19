---
id: 019
title: The no-path cross stands off the hex cursor in 3D
milestone: M1
status: done
owner: claude
suggested: claude
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

In 3D, in walk mode, the red no-path cross is where the game shows it in 2D
relative to the hex cursor.

## Context

- The owner's screenshots (`runs/019-no-path-cross/`): over an unreachable
  hex the cross floats about 55 game pixels above the red hex cursor, both
  before and after PR #8.
- In walk mode `_gmouse_3d_move_to` (`ce/src/game_mouse.cc`) puts the hex
  cursor on the hex under the mouse and moves the cross object
  (`gGameMouseBouncingCursor`) with `_obj_move` to the hex's screen position
  plus (16, 15). In 3D both are drawn by `_obj_render_scene_overlay`
  (`ce/src/object.cc`), shifted by the hex's `tileSceneShift` since PR #8, so
  the gap is not the shift.
- To find first: where the cross stands in 2D at the same hex (run without the
  scene), and how `_obj_move` with screen coordinates sets the object's tile
  and offsets, versus how `_obj_render_object` places it.

## Acceptance

- Over an unreachable hex the cross sits where it does in 2D, at several
  places on screen and with roofs on and off; snapshot pairs in the PR.

## Out of scope

The hex cursor itself (it is right).

## Log

### 2026-10-19 claude
Done: the cross stands in the red hex cursor, as in 2D. In walk mode
`_obj_move` puts the cross off the grid (tile -1) at a screen point
(`sx`, `sy`); `_obj_render_object` draws such an object there and ignores
`x`, `y`, so PR #8's hex shift reached the hex cursor only. The overlay now
shifts `sx`, `sy` for objects off the grid.
Evidence: `runs/019-no-path-cross/owner-before-wall-corner.png` (the
owner's, before) and `after-store-counter.png` (the probe's pointer on a
blocked hex at the store counter, after).
Check: build, `ctest --test-dir build` (8/8); `dev-snapshot.ps1 -Probe
-HeroTile 21678 -ViewTile 21678` (the probe points at blocked hex 22276).

### 2026-10-19 claude
Done: the owner checked it in the game and accepted: the cross is in place.
