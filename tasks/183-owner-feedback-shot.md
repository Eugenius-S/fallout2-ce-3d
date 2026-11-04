---
id: 183
title: One key in the game saves a screenshot with everything an agent needs to read it
milestone: M1
status: todo
owner: none
suggested: gpt
priority: high
tier: 1
claude-only: no
user-check: no
depends-on: [127]
reviewed: no
---

## Goal

The owner plays, presses one key and gets a PNG with a JSON next to it.
Agents later read a batch of these shots with the owner's notes, location
by location, and turn them into tasks.

## Context

- Extend the existing F9 dev shot (`game/shots/dev/`, `devtools::takeSnapshot`,
  docs/p1-devtools.md); reuse 127's frame-time numbers.
- The file name sorts by time and shows the map:
  `YYYYMMDD-HHMMSS-<map>-e<elevation>.png` and `.json`.
- The JSON holds:
  - the current FPS, the average frame time and the 1% low over the last
    few seconds;
  - the map name and file, the elevation, the city or area;
  - the player's tile, hex and world position;
  - the camera position, rotation and zoom;
  - the resolution, the scale and every render setting;
  - the build commit and the timestamp;
  - the object under the cursor, if any (prototype id, name, tile).
- The shot must not stall the frame noticeably: read the frame back and
  write the files off the main thread.

## Acceptance

- Three shots on different maps in `runs/183-owner-feedback-shot/` with
  their JSON; the frame-time log shows no frame over 50 ms at the key press.
- `docs/owner-shots.md`: how an agent reads a batch of shots with the
  owner's notes (match a note to a shot by time and map, open the JSON first,
  the picture only when needed, one task per cause).
- ctest passes.

## Out of scope

Analysing the shots (a later task).

## Log
