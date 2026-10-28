---
id: 105
title: The README tells a player how to install with the launcher and shows what works today
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

A player who lands on the GitHub page learns in one screen what the project
does today and how to get it: download the launcher from the `latest`
release, point it at Fallout 2, play.

## Context

- `README.md` still describes the first prototype (one Klamath map, "next:
  roofs, light and shadows, then an installer"). Since then: roofs, sun and
  moon shadows with the time of day, point lights and glow, any resolution,
  smoothing, the perspective camera with smooth movement and turning, the
  fly camera, the `[3d]` options in the game's menu, the launcher (091).
- 091's launcher was proven on a clean copy against a fake release on
  127.0.0.1 (`runs/091-launcher-updates/README.md`). The real `latest`
  release exists since #133 (zip with six files, `version.txt`, the
  launcher), but no run against it has been made.
- The game crashes with 0xC0000005 when closed with the window's X in the
  launcher's flow (091's notes); log it in `tasks/inbox.md` if it still
  happens, do not fix it here.

## Acceptance

- One run of the launcher downloaded from the real `latest` release on a
  clean copy (`master.dat`, `critter.dat`, `data/` copied in; never the
  owner's `game/`): install, 3D on a Klamath map, the log and a shot in
  `runs/105-readme-install-and-features/`. If it fails, the README waits and
  the failure goes in the Log.
- `README.md`: what the project is (keep the intro's spirit), what works now
  as a short list, how to install (which Fallout 2 copies, the launcher, what
  it changes and never touches: the player's `fallout2.cfg`, saves, the
  original exe), how to switch 3D off (Ctrl+F10, the `[3d]` options), how
  to build from source (the current steps, kept short). Remove the outdated
  "next on the plan".
- The README's video stays; task 097 will replace it later.

## Out of scope

The video (097), code changes to the launcher.

## Log
