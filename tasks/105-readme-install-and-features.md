---
id: 105
title: The README tells a player how to install with the launcher and shows what works today
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
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

### 2026-10-28 gpt
Done: Rewrote `README.md` for the current launcher, supported features and build steps. The video link stays. Removed the obsolete prototype and next-on-plan text.
Check: Queried the real GitHub `latest` release. It was published 2026-10-27 and points to version `623e380cd5f87fbc3234e05bbca92ccd426b8d60`. The launcher asset SHA-256 is `fe9e42ffdc9213fa7b527ab8b2a7cb26b502a00e6843292697e7212cdc1e6d52`; the zip SHA-256 is `f2b16a1cdad1b093599a18581d73da1dbbbe0cb10323f15a9bfddc81b3b0c87e`. The downloaded launcher installed that release into an isolated clean copy, exported seven Klamath map scenes and started the installed exe. Installed exe SHA-256 `b795d07a6fee13d112a1437305bc772c35e57e1eafcddfcd496af847db49bad7` matches the release zip. The 800x480 sidecar at `runs/105-readme-install-and-features/latest-launch-kladwtwn.json` records `KLADWTWN.MAP`, elevation 0, `scene_3d_enabled=true`, `scene_view=full`; PNG: `runs/105-readme-install-and-features/latest-launch-kladwtwn.png`.
Check: `runs/105-readme-install-and-features/real-latest-run.log` records `fallout2.cfg` SHA-256 unchanged at `892a30a9cddc20815142047b46b844e9aa1a8a6e39aea2639d3f51aa213b77de`, empty save-tree SHA-256 unchanged at `6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d`, and game exit code 0 after WM_CLOSE was sent to the isolated test window. The clean copy did not include `fallout2.exe`, so there was no original-exe hash to compare; launcher code installs only `fallout2-ce.exe`, `ce.dat`, `scene-exporter.exe`, a default config only when missing, and `version.txt`.
Check: An update run with the existing non-empty config is in `runs/105-readme-install-and-features/update-preserves-config.log`. The launcher logged `fallout2.cfg: kept`, reinstalled the latest build, and the config SHA-256 stayed `892a30a9cddc20815142047b46b844e9aa1a8a6e39aea2639d3f51aa213b77de`. Absolute paths in the evidence logs were replaced with `<game>`; other log content was left byte-for-byte intact.
Next: Review this PR. Do not merge before review.
