---
id: 183
title: One key in the game saves a screenshot with everything an agent needs to read it
milestone: M1
status: todo
owner: any
suggested: any
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

- Step 2, the region mark (the owner, 2026-11-04), after the plain shot
  works: the owner holds a modifier (Ctrl or Alt) and presses the shot key.
  Check first that the combo collides with no game or dev binding and pick a
  free one. The game freezes on the current frame; the owner drags one or
  more rectangles over the defect; Enter or a second press saves, Esc
  cancels. It saves the clean PNG, a second PNG with the rectangles drawn
  on it (`...-marked.png`), and the JSON with a `regions` list: each region's
  pixel rect, the map tiles or hexes it covers, and the objects, wall and
  roof pieces under it (id, type, art name).

- Batch: the dev tools (2026-11-04 inbox sweep). Two more steps, each its
  own PR after step 1:
  - Step 3, the teleport (inbox 2026-10-21): jump the hero to any spot of any
    map (map, elevation, hex) inside a running game, from the dev panel and
    from a command file the game watches, so an agent can do it in the
    owner's running game on the owner's word. Start options already exist
    (`--dev-map`, `--dev-elevation`, a view hex).
  - Step 4, a safe floor change (inbox 2026-10-21): the dev panel's floor
    change put the hero inside the rock on klatoxcv elevation 2. The hex
    check (`docs/p1-devtools.md`, "Tile choice on a floor change") looks at
    the floor tile and blockers within 4 hexes only. Pick a hex the hero can
    walk from (a path to the map's entrance or to a few hexes around), or
    take the hex the owner names. The teleport uses the same check.

## Acceptance

- Three shots on different maps in `runs/183-owner-feedback-shot/` with
  their JSON; the frame-time log shows no frame over 50 ms at the key press.
- `docs/owner-shots.md`: how an agent reads a batch of shots with the
  owner's notes (match a note to a shot by time and map, open the JSON first,
  the picture only when needed, one task per cause).
- Step 2: two marked shots (one and two rectangles) with their clean and
  marked PNGs and JSON in `runs/183-owner-feedback-shot/regions/`; each
  region lists its tiles and the objects under it. `docs/owner-shots.md`
  tells the agent to start from `regions` and the objects listed in them.
- ctest passes.

- Step 3: a command-file jump to klatoxcv e2 and to a hex on another map
  lands the hero on a walkable hex; the dev panel does the same.
- Step 4: the floor change to klatoxcv e2 leaves the hero able to walk.

## Out of scope

Analysing the shots (a later task).

## Log
