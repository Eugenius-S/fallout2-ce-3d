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
reviewed: no
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

### 2026-11-01 follow-up
Added independent `--dev-sun-shadows` capture mode for matched cloud-only,
sun-only and combined comparisons.

The original combined shader applied ambient cloud attenuation and solar
attenuation before converting light to 128 steps and RGB555-like channels.
That quantization hid the solar contribution in 16.07% of overlap pixels.
The final compositor computes cloud-only and sun-only RGB5 contributions,
then adds their darkening deltas for the combined mode. Cloud-only and
sun-only modes are unchanged.

Matched four-mode evidence at KLADWTWN/tile 24678, 08:25, swing 44, phase 12
is in `ab-final-0825-s44-p12-shadow-modes.jpg` with mode sidecars and metric
manifest. The overlap is 86,265 pixels (4.16% of frame). Mean solar luminance
contrast is 17.20/255 without clouds, 10.66/255 with the old compositor, and
16.20/255 (94.2%) with the RGB5 additive compositor. Pixels with no solar RGB
change fall from 16.07% to 11.01%. Cloud darkening remains visible in the
overlap at 7.10/255 versus 8.09/255 cloud-only.

Tradeoff: the combined overlap is locally darker. Pixels whose maximum RGB
channel is at most 8/255 rise from 10.95% to 28.99% within that overlap mask.
Matched 08:00, 12:00 and 17:00 phase-12 crops retain visible wall and curb
detail, but the increased local darkness needs the owner's visual review.
All nine matched before/after pairs (18 individual JPEGs, <=714 KB each) use
swing 44, identical camera/time/resolution per pair, and phases 0, 12 and 24.
The three comparison sheets are
`08-before-after-composite.jpg`, `12-before-after-composite.jpg` and
`17-before-after-composite.jpg`.

Task 137 follow-up remains preliminary: the clipboard crop has no runtime
metadata; its strong image match persists with both shadow modes off, while
cloud-only changes its pixels and sun-only does not. No shadow-pass source
for that edge is established. Forced-ambient runs are excluded because the
override also changed reported sun-up state.

Check: Release build passed; CTest 18/18, scene-export tests 88/88,
regression-tool tests 12/12, and the full pixel regression 16/16 with zero
changed pixels. Captures use `build/scene-cache`; `game/mod-cache` was not
used. PR #199 remains draft and depends on #198; request owner visual review
before treating the compositor as accepted.
