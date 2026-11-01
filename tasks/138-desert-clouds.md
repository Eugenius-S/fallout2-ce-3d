---
id: 138
title: Cloud shadows fit a desert sky, fewer, smaller and soft-edged
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: high
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

The owner, 2026-11-01: the cloud shadows read as big dark blotches over the
street (`runs/inbox/2026-11-01-cloud-blotches.png`, Klamath downtown, 08:25)
and hide the buildings' sun shadows. Klamath is desert: the sky holds few
clouds. Fewer clouds, each smaller, and every edge a smooth fade with no
sharp step from shade to sun. The buildings' own shadows stay readable
under and between the clouds.

## Context

- Cloud shadows came with #169 (`[3d] cloud_shadows`, on by default); the
  pattern lives in `ce/src/scene_renderer/` (scene_shader.cc, scene_draw.cc,
  shadow.cc). A subagent checked on 11-01 that the blotches are the clouds,
  not a regression: main a584586c and e434c327 draw the same picture.
- Task 137 (a long straight hard shadow edge across the downtown street) may
  be a cloud edge too; check it while here and note it in 137's Log.
- The owner plays with `sun_swing=44` (the default becomes 40 with #197):
  check at 08:00, 12:00 and 17:00.

## Acceptance

- before | after strips of Klamath downtown at 08:00, 12:00 and 17:00, three
  cloud phases each, in `runs/138-desert-clouds/`: the clouds cover a small
  share of the ground, smaller patches, soft edges, building shadows clear.
- The look is set by a few named constants or settings in one place, so the
  owner can ask for "a bit more" without a code hunt.
- `cloud_shadows=0` still turns them off; no clouds at night or indoors.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

A visible sky or cloud bodies (113), the sun's own shadows.

## Log

### 2026-11-01 gpt
Done: Reduced cloud cover and darkness, made the broad noise cells smaller,
and widened the soft transition. The named tuning values are together in
`ce/src/scene_renderer/scene_shader.cc`.

Evidence: Three before/after strips and all 18 full-resolution JPEG captures
with sidecars are in `runs/138-desert-clouds/`. Each uses KLADWTWN, view tile
24678, 1920x1080, at 08:00, 12:00 or 17:00 and cloud phase 0, 12 or 24.
The building shadows remain visible. `137-cloud-edge-check.jpg` records the
matched cloud-only and shadows-off diagnostic; its limits are written in
task 137's Log.

Check: Release build passed. CTest passed 18/18; scene export tests passed
88/88; regression tool tests passed 12/12. The full pixel regression passed
16/16 with zero changed pixels. Captures used `build/scene-cache`; no
`game/mod-cache` path was used.

Next: None.
