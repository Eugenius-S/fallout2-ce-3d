---
id: 094
title: Winding the clock past night never brings the morning back
milestone: M1
status: todo
owner: none
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
