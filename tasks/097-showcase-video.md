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
depends-on: [118]
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
- The owner, 2026-11-01: the 3D options screen is not shot for now;
  segment 11 stays out of the video until task 130 finishes the screen.
- The owner, 2026-11-01: the music is the game's own Vault City track.
- Take it when the slice is close to done, so the shots show the finished
  look; until then this task waits.
- Recording goes through the background test instance
  (`tools/dev-snapshot.ps1`), never by driving the owner's running game;
  `tools/dev-launch.ps1` only when the owner asks for it in chat.

## What the video shows

About 100 seconds, in this order; the caption in the corner is in brackets.

The caption always says plainly what is on screen. Whenever the original
is shown it reads "Original"; the 3D view is then built up one layer at a
time, each with its own caption: "Perspective", "With shadows", "With
clouds" and so on, so the difference is obvious without explanation.

1. 0-8 s: Klamath at 640x480 in the original (Original), then the same
   spot step by step: the perspective view (Perspective), the sun shadows
   (With shadows), the sky with clouds (With clouds).
2. 8-15 s: the launcher installs onto a clean Fallout 2 in one click and the
   game starts (One-click install).
3. 15-24 s: the window changes size and the picture fits each, then the
   interface scale switches from 1x to 2x (Any resolution, any scale); the
   main menu and the death screen with the owner's widescreen art.
4. 24-32 s: a dialogue and the character screen in the original font
   (Original), then in the new sharp font (New fonts).
5. 32-42 s: Ctrl+F10 back and forth on one spot in Klamath: walls, roofs,
   corners (Original / 3D).
6. 42-50 s: the camera turns up to 45 degrees, the characters turn with it
   (Camera rotation).
7. 50-57 s: the fly camera over the map, the sky above (Free camera).
8. 57-66 s: a sped-up day, the sun crosses overhead and the building shadows
   turn (Sun and shadows).
9. 66-75 s: night: burning barrels, the glowing slime, a sign switching on
   and off (Light and glow).
10. 75-83 s: a laser and a plasma shot: the muzzle flash and the glow of the
    shot in flight (Weapon light).
11. Not shot for now (waits for task 130). 83-92 s: the options menu: each 3D feature switched off one by one down
    to the original look, a slider moved (Every feature can be switched off
    and tuned).
12. 92-100 s: a closing run through Klamath, split screen original and 3D,
    then the title and the GitHub link (fallout2-ce-3d).

## Acceptance

- One video, 1 to 2 minutes, 1080p, a caption in the corner for each
  segment in English, the Vault City track under it.
- The video sits where the GitHub page can show it (a release asset or a
  link in `README.md`), not committed into the repo; raw clips stay in
  `game/shots/`, and `runs/097-showcase-video/` holds only the edit plan and
  small JPEG previews.
- The owner watches it and says it goes up.

## Out of scope

A voice-over, a trailer for anything beyond Klamath, other languages.

## Log

### 2026-11-01 claude
Done: `tools/record-clip.ps1` records a clip of a separate test game
(build/record-run, its own exe and config, never the owner's game) into
`game/shots/097/<name>.mp4`, 1920x1080 at 30 fps, driving it with timed keys
sent as window messages; test clips `test-01`, `test-02` (Ctrl+F10 toggles
mid-clip). The Vault City track is decoded with the engine's own ACM decoder
to `game/shots/097/audio/22vcity.wav` (197.8 s, 44.1 kHz stereo; ffmpeg's
ACM decoder fails on this file). Task 118's code is already on `main`.
Next: in a new chat with the After Effects MCP loaded, check `ae_project_info`,
record segments 1-10 and 12, edit.
Check: `tools/record-clip.ps1 -Name test -Seconds 6`.

### 2026-11-01 claude
Done: the owner wants the game in English with an English interface. The
Russian lettering on the panel (PUNCH, Skilldex, HP/AC, MAP/CHA/PIP) came from
the Restoration Project Russian packs in game/mods (`rpu_russian.dat`,
`upu_russian_sound.dat`), not from master.dat. An English clip now runs from
build/record-run/game, links to game/ with its own mods_order.txt without the
*_russian packs; the owner's files are not written. `test-03` shows the
English panel.
Next: as above.
Check: `tools/record-clip.ps1 -Name test -Seconds 4`, the panel reads PUNCH.
