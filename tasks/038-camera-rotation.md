---
id: 038
title: Optional camera rotation in 90-degree steps
milestone: M1
status: done
owner: claude
suggested: gpt
claude-only: no
user-check: yes
depends-on: [031]
reviewed: yes
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

### 2026-10-20 claude
Done: `[3d] camera_rotation` (off by default, "Camera rotation
(experimental)"; the 017 panel will list it). With it on, [ and ] turn the 3D
camera a quarter round the floor point the view looks at, 360 degrees a
second (`scene::turnCamera`, `updateCameraTurn` from the main loop); a full
turn lands it. A turned camera is the fly camera's drawing (cards face it,
doors on their walls, the fly-only geometry of 033 to 040, no interface) at
the game view's framing: `scene_renderer::staticGameViewCamera` puts a plain
perspective camera (the game's 25 degrees) where it shows the same floor width
across the screen's middle as the game's view. Clicks do nothing while it is
turned (`game_mouse.cc`); it lands by itself when the full 3D view goes
(menus, dialog, combat). `dev-snapshot.ps1 -Turn N` takes a turned snapshot.
Evidence: `runs/038-camera-rotation/kladwtwn-bathhouse-0-90-180-270.jpg`.
Regression 16/16 unchanged (off by default); unit tests 10/10.
Not checked: the keys and the animation in a real game (the background test
instance takes no keys); the owner's look.
Next: 039 (all of Klamath from turned views).
Check: `tools/dev-snapshot.ps1 -HeroTile 17665 -ViewTile 17467 -Turn 2`.
