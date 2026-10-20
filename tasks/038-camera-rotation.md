---
id: 038
title: Optional camera rotation in 90-degree steps
milestone: M1
status: doing
owner: claude
suggested: gpt
claude-only: no
user-check: yes
depends-on: [031]
reviewed: no
---

## Goal

A player who switches it on can turn the camera around the hero in steps of
90 degrees and back; off by default.

## Context

- The design: [docs/2026-10-20-shapes-and-rotation-design.md](../docs/2026-10-20-shapes-and-rotation-design.md). Read it first; its "How the agents work on
  this" holds for this task.
- `[3d] camera_rotation` (default off) with a menu line marked experimental:
  the 3D panel of 017 lists every `[3d]` key by itself, so the line comes
  with 017 and this task does not wait for it.
- Look-only: at a turned angle the game takes no clicks and shows no hex
  cursor; back at 0 degrees everything works as before.
- Cards and critters face the camera as under the fly camera (033); critters
  show the direction nearest to the turn. See-through walls (018) and hidden
  roofs (013) follow the camera's direction.
- Pick keys the game does not use; write them in `docs/p1-devtools.md` or the
  settings doc.

## Acceptance

- The option is off by default; with it off, regression 16/16 unchanged.
- With it on, two keys turn the camera by 90 degrees over about a quarter of a
  second; one view at each of the four angles in `runs/038-camera-rotation/`.
- At a turned angle clicks do nothing; at 0 degrees they work.
- A dev option to take a snapshot at a turned angle.

## Out of scope

Steps of 45 degrees. Clicking in turned views.

## Log
