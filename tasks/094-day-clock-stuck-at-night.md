---
id: 094
title: Winding the clock past night never brings the morning back
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

Winding the game's time forward goes through night to morning: the picture
gets light again and the sun comes back.

## Context

- The owner, 2026-10-27, in the live game: the clock winds to night, then
  does not wind on to morning; the moon's shadows go round in a cycle, but
  it never gets light.
- The sun and moon follow game time in `ce/src/scene/sun.cc`
  (`sunAt`: hour = game time mod 24; `SunTracker::update` treats a
  backward step or a long step as a jump). Suspects: the time wraps at
  midnight (a smaller game time than the last frame, or a day counter) and
  the tracker or the daylight flag stays on night; or the control that
  winds time stops at night.
- Reproduce first: name which control the owner used (the dev panel's
  hour, waiting in the Pip-Boy, or the in-game clock) by trying each, and
  log the game time and the daylight flag across the night.

## Acceptance

- A test in ctest for the night to morning step in the sun's code.
- Shots in `runs/094-day-clock-stuck-at-night/`: the same spot at 22, 2, 6
  and 9 hours, wound forward in one go, light again at 9.
- The original view regression stays pixel for pixel.

## Out of scope

How the night looks (the moon's strength and colour).

## Log

### 2026-10-27 gpt: reproduced and fixed Ctrl+T

The owner confirmed Ctrl+T. In the isolated KLADWTWN run the real handler
wrapped 23:00 to the same day and left ambient at 28426 through 09:00 while
the daylight flag stayed true; the sun strength was only 0.245. Running the
native map-update procedure after each step restored ambient to 65536 at 06:00.
Ctrl+T and Alt+T now step the actual date by one hour and run that map update.
The new sun CTest covers the night-to-morning step and midnight in both
directions. A single no-save Ctrl+T ladder produced the 22/02/06/09 sheet in
runs/094-day-clock-stuck-at-night/ and confirms strength 1.0 at 09:00.
Next: complete the original-view regression, full fly tour and CI; then merge
094 before taking 095 as requested.

### 2026-10-27 gpt: claimed

Owner confirmed Ctrl+T in the live 3D view as the control that reproduces
the stuck night. Starting from main 410e2d9c in the separate worktree
E:/Projects/fallout2-ce-3d-gpt-094. First reproduce that control in an
isolated test run, recording time, ambient and daylight through midnight;
then fix the cause, add the night-to-morning test and capture the four hours.
