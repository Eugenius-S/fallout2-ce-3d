---
id: 127
title: An in-game FPS counter the player can turn on and off
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: high
tier: 1
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

A small frame rate readout in a screen corner, off by default, turned on
and off by a key and by a setting in the 3D options, so the owner can see
the real FPS while playing.

## Context

- The owner, 2026-10-29, felt the frame rate was low in a live check with no
  number; 2026-10-30 asked for a counter that can be turned on and off.
- Average over a short window (about half a second) so the number is
  readable; show frame time in ms beside it.
- Settings live in `ce/src/settings.cc` and the 3D options panel in
  `ce/src/scene/options_panel.cc`; draw the readout on the text layer so it
  stays sharp.

## Acceptance

- Shots with the counter on in the 2D game view and in the 3D view, in
  `runs/127-fps-counter/`, with the measured numbers for Klamath downtown
  (game camera and fly camera) written in the Log.
- A frame-time log (`[3d] frame_log` or a dev flag): every frame's time to a
  CSV, and a summary line with the average FPS, the 1% low and the worst
  frame. 30 s baselines, game camera and fly camera, in Klamath downtown,
  New Reno (newr1), Vault City (vctyctyd) and one more heavy spot, written
  in the Log and kept in `runs/127-fps-counter/baseline/`: the numbers 157
  and every later speed task compare against.
- With the counter off the original view regression stays pixel for pixel;
  ctest passes, including a settings test for the new key.

## Out of scope

Performance fixes. If the numbers show a problem, write it in the Log for a
new task.
