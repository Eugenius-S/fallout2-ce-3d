---
id: 097
title: A showcase video for GitHub shows the original next to the 3D view and every feature
milestone: M1
status: todo
owner: none
suggested: claude
priority: low
claude-only: yes
user-check: yes
depends-on: []
reviewed: no
---

## Goal

One short video for the GitHub page: what the game looked like, what it looks
like now, and each 3D feature in turn, with a caption in the corner saying
what is on screen.

## Context

- The owner, 2026-10-27: a video near the end of the Klamath slice. Claude
  records and writes it in a chat with a full weekly limit; the edit is made
  with the Higgsfield skills (`higgsfield:higgsfield-video-explainer`,
  `higgsfield:higgsfield-generate` for the pieces they cover).
- Take it when the slice is close to done, so the shots show the finished
  look; until then this task waits.
- Recording goes through the dev tools (`tools/dev-snapshot.ps1` and the dev
  launch), never by driving the owner's running game.

## What the video shows

Each segment is the original (Ctrl+F10) and the 3D view of the same place,
side by side or one after the other:

1. Any resolution: the same map at several window sizes, the picture and
   the interface fit each.
2. Smoothing: edges and textures in the 3D view against the original.
3. Shooting: a laser shot and a plasma shot in flight, with their glow.
4. Signs: a lit sign switching on and off, its glow following.
5. Light: burning barrels and the slime glowing in the dark.
6. Shadows: the shadows of buildings turning as the sun crosses the day.
7. Camera: isometric tile by tile movement against the perspective view and
   its smooth movement.
8. Turning the camera inside the safe zone in the 3D view.
9. A closing run through Klamath: before and after.

## Acceptance

- One video, 1 to 2 minutes, 1080p, a caption in the corner for each
  segment in English, no music without a licence.
- The video sits where the GitHub page can show it (a release asset or a
  link in `README.md`), not committed into the repo; the raw clips and the
  edit plan in `runs/097-showcase-video/` within the runs size limit.
- The owner watches it and says it goes up.

## Out of scope

A voice-over, a trailer for anything beyond Klamath, other languages.

## Log
