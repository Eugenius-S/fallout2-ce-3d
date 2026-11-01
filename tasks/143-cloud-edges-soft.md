---
id: 143
title: Cloud shadow edges stay soft on the ground
milestone: M1
status: todo
owner: any
suggested: any
priority: normal
tier: 3
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

A cloud shadow fades into the lit ground over a wide soft band. No hard
line or one-step contour at its edge, on dark ground as well as on light.

## Context

- The owner, 2026-11-01, after 138 (#199) went live: "the clouds give hard
  edges again" (`runs/inbox/2026-11-01-cloud-hard-edge.png`, the downtown
  street in kladwtwn, a curved hard line across the dark asphalt).
- 138 changed `ce/src/scene_renderer/scene_shader.cc`: the cover now runs
  through `smoothstep(kCloudCoverLow, kCloudCoverHigh, cover)`, and for the
  ground and walls (`uLightMode` 1 and 2) the cloud loss is taken in the
  5-bit colour steps (`cloudLoss5 = baseColor5 - cloudColor5`, then
  subtracted from `sunColor5`). On dark ground a 22% loss is one or two
  5-bit steps, so the whole soft band collapses into a single step: a hard
  line. That is the first suspect; confirm it before fixing.
- Also rule out a lamp's light edge: the shot has a lit patch top right.
  Take the same spot with cloud shadows off; if the line stays, it is not
  this task, write that in the Log.
- The fix is general: the cloud fade must not be quantised into a step
  where the 5-bit look is kept (dither the cloud loss, apply it before the
  step, or another general rule). No per-map or per-spot case.

## Acceptance

- At the owner's spot and on light sand, the cloud edge reads soft: no
  line, no single step, at 08:00, 12:00 and 17:00.
- Building shadows keep their contrast inside and outside a cloud (138).
- Evidence in `runs/143-cloud-edges-soft/`: before and after crops at the
  owner's spot and on light ground, and the same spot with clouds off.
- The scene tests pass.

## Log

- 2026-11-01 Cut from the owner's live find after #199.

- 2026-11-01 The owner asked gpt directly to make cloud shadows an option that is off by default for now; this task still makes the edges soft for when they are on.

### 2026-11-01 gpt
Blocked: The owner's PNG is 432x505 and has no camera or time sidecar. I used
the prior KLADWTWN/tile 24678 fixed camera as a candidate, but the best
half-scale template correlation is only 0.386. The reported hard edge did not
reproduce in that candidate, so the paired cloud-off capture cannot attribute
the edge to clouds or the lamp. No shader change was made.

Evidence: `runs/143-cloud-edges-soft/owner-vs-candidate-0825.jpg` and
`runs/143-cloud-edges-soft/diagnostic.json`. The matched 08:25 captures use
game time 303000, sun swing 44, cloud phase 505, 1920x1080, and the same
executable. Cloud-off keeps sun shadows enabled. In the best candidate crop,
on/off grayscale mean absolute difference is 1.57/255, with no pixels over
10; this is not an exact match to the owner input. A second candidate pair at
08:00 and cloud phase 0 had the same camera-mapping limitation.

Check: Release build completed (763/763); CTest passed 18/18. The exporter
read 1467 resources into `build/scene-cache`. Pixel regression was not run
because no renderer change was made. `game/mod-cache` was not used.

Next: Obtain a capture sidecar or exact camera, zoom, and time for the owner
frame, then repeat the matched cloud-on/cloud-off comparison before changing
the shader.
