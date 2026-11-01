---
id: 097
title: A showcase video for GitHub shows the original next to the 3D view and every feature
milestone: M1
status: doing
owner: claude
suggested: claude
priority: low
claude-only: yes
user-check: yes
depends-on: [112, 118]
reviewed: no
---

## Goal

One short video for the GitHub page: what the game looked like, what it looks
like now, and each 3D feature in turn, with a caption in the corner saying
what is on screen.

## Context

- The owner, 2026-10-27: a video near the end of the Klamath slice. Claude
  records and writes it in a chat with a full weekly limit.
- The owner, 2026-11-01: the edit is made in After Effects through the
  Higgsfield use-after-effects MCP (`higgsfield-use-after-effects`, npm
  `fnf-after-effects-mcp`, entry skill `ae-clean-rig`). Recording starts once
  tasks 112 (options panel) and 118 (widescreen art) are on `main`.
- Take it when the slice is close to done, so the shots show the finished
  look; until then this task waits.
- Recording goes through the background test instance
  (`tools/dev-snapshot.ps1`), never by driving the owner's running game;
  `tools/dev-launch.ps1` only when the owner asks for it in chat.

## What the video shows

About 90 seconds, in this order; the caption in the corner is in brackets.

1. 0-6 s: the original at 640x480 in Klamath, a wipe turns the same spot
   into the 3D view (Fallout 2, now in 3D).
2. 6-14 s: the launcher installs onto a clean Fallout 2 in one click and the
   game starts (One-click install).
3. 14-22 s: the main menu and the death screen with the owner's widescreen
   art while the window changes size (Any resolution, widescreen art).
4. 22-32 s: Ctrl+F10 back and forth on one spot in Klamath: walls, roofs,
   corners (Original vs 3D).
5. 32-42 s: the camera turns up to 45 degrees, the characters turn with it
   (Camera rotation).
6. 42-50 s: the fly camera over the map, the sky above (Free camera).
7. 50-60 s: a sped-up day, the sun crosses overhead and the building shadows
   turn (Sun and shadows).
8. 60-70 s: night: burning barrels, the glowing slime, a sign switching on
   and off (Light and glow).
9. 70-78 s: a laser and a plasma shot: the muzzle flash and the glow of the
   shot in flight (Weapon light).
10. 78-84 s: the options menu on its 3D panel, graphics options switching
    (3D options).
11. 84-92 s: a closing run through Klamath, split screen original and 3D,
    then the title and the GitHub link (fallout2-ce-3d).

## Acceptance

- One video, 1 to 2 minutes, 1080p, a caption in the corner for each
  segment in English, no music without a licence.
- The video sits where the GitHub page can show it (a release asset or a
  link in `README.md`), not committed into the repo; raw clips stay in
  `game/shots/`, and `runs/097-showcase-video/` holds only the edit plan and
  small JPEG previews.
- The owner watches it and says it goes up.

## Out of scope

A voice-over, a trailer for anything beyond Klamath, other languages.

## Log
