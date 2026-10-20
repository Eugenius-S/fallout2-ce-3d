---
id: 021
title: The sun follows game time
milestone: M1
status: done
owner: claude
suggested: claude
claude-only: no
user-check: no
depends-on: [013, 020, 034]
reviewed: yes
---

## Goal

The scene knows where the sun is: its direction, height, color and strength
follow game time. This task draws nothing new; building shadows (045) and
characters in shadow (046) build on it.

## Context

- Game time: `gameTimeGetTime()` (36000 ticks an hour, `scripts.h`). It
  jumps on waiting, travel and save load.
- Strength and color follow the ambient the map scripts set, so the game's own
  day and night decide how bright the sun is. Night: no sun.
- `scene.json` marks maps: `indoor` (underground, no sun), `daylight` (under
  the sky).
- The azimuth is a constant for now; the owner tunes it on 045's snapshots.
- A jump of game time sets the sun at once; normal time moves it smoothly.

## Acceptance

- `[3d] sun` (default on) with a menu label; the sun is off indoors and at
  night.
- The sun's direction, height, color and strength appear in the dev snapshot
  JSON.
- Unit tests: the sun at 6:00, 12:00, 19:00 and 0:00; a time jump snaps it.
- The game view does not change (the regression passes as it is).

## Out of scope

Shadows (045), characters in shadow (046), cloud shadows (023).

## Log

### 2026-10-20 claude
Done: the research of 2026-10-19 is split into 021, 045 and 046.
Next: take it.
Check: read the Context.

### 2026-10-20 claude
Done: `ce/src/scene/sun.{h,cc}`: the sun by game time (rises 6:00, 60 degrees
at noon, sets 18:00, crosses the sky; warm when low), its strength from the
ambient the scripts set (none at the engine's lowest), faded over a second on
an ambient change and set at once on a time jump or a new map. The engine
reads `daylight` from scene.json on load (`scene_view.cc`); `[3d] sun`
(default on); the frame's sun in the snapshot JSON (`"sun"`). Nothing is drawn
with it yet. Noon azimuth `kNoonAzimuth` (225) waits for 045's snapshots.
Unit tests `ce/tests/sun` (6:00, 7:00, 12:00, 17:00, 19:00, 0:00, indoors,
night ambient, fades and jumps). Regression 16/16, unchanged.
Next: 045 (building shadows).
Check: `tools/dev-snapshot.ps1 -Map kladwtwn -Tour klaratcv:0`: "sun" up in
the town at 8:24 (35 degrees), none in the cave.
