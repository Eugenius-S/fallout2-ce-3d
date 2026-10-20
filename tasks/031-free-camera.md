---
id: 031
title: A free-flying camera for looking around maps
milestone: M1
status: done
owner: claude
suggested: claude
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

A developer flies the 3D camera freely over a map to look for flaws up close
and to record videos, as in the donor viewer.

## Context

- The renderer already draws the scene from any eye: the fixed preview
  camera (`StaticPreviewCamera`, `makeFloorMvp`) is a plain look-at camera.
  The game view (`game_view.h`) is the default.
- Dev hotkeys live in `ce/src/devtools/devtools.cc`. Ctrl+F11 is taken (the
  window shape 16:9 / 21:9); pick a free key and list it in `docs/p1-devtools.md`.
- In fly mode the game must not get the keys and the mouse (no walking,
  no hotkeys); the game itself may keep running or pause.
- Sprites are upright cards facing the game's camera, so from the side they
  look like cardboard: fine for this view.

## Acceptance

- One key (with `--dev-tools`) enters and leaves fly mode; leaving returns
  the game view exactly as before.
- WASD moves, the mouse looks around, Q / E go down / up, Shift is faster;
  the movement is smooth enough for a video at 60 fps.
- A dev snapshot works in fly mode.
- Not available without `--dev-tools`.

## Out of scope

Recording video inside the game (an outside recorder is used).

## Log

### 2026-10-20 claude
Done: Ctrl+F12 (with `--dev-tools`) takes the 3D view up into a free camera
and lands it again. The mouse turns it, WASD moves it, Q / E go down and up,
Shift is faster and Ctrl slower, by the time between frames. While it flies it
owns the keyboard and the mouse (`mouseDeviceSetCaptured`: the game sees the
mouse standing still with no buttons, so no clicks and no edge scrolling) and
the scene is drawn from it alone: no interface, no egg. Landing puts the game's
view back; it lands by itself when the 3D scene stops showing (combat,
dialog). It starts on the game view's ray, closer to the ground. F9 works in
the air; `dev-snapshot.ps1 -Fly` (`--dev-fly`) snapshots the take-off, without
taking the mouse. Documented in `docs/p1-devtools.md`.
Evidence: `runs/031-free-camera/klamath-from-the-air.png`.
Next: the owner flies it in the game (mouse and keys can only be checked by hand).
Check: `tools/regression.ps1` (16 pass); `ctest --test-dir build` (8/8).

### 2026-10-20 claude
Done: its acceptance held at the merge; it stayed open only for the owner's
look, which blocks nothing (AGENTS.md, "Nobody waits"). What the owner
rejects in the game becomes a new task.
