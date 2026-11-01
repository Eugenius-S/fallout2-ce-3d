---
id: 143
title: Cloud shadow edges stay soft on the ground
milestone: M1
status: todo
owner: none
suggested: gpt
priority: high
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
