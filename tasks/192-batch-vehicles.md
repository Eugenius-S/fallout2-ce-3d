---
id: 192
title: Vehicles across every map, one rule per PR
milestone: M1
status: doing
owner: gpt
suggested: any
priority: normal
tier: 3
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

Every vehicle (cars, the vertibird, wrecks, the tanker's machinery) stands right in 3D. Multi-part locking is 174; this batch is the shape of each vehicle.

## Context

- The owner, 2026-11-04: geometry is fixed by kind, one class of objects
  across every map per task and per PR, never map by map.
- Sources: the finds of this kind in the area tasks' Logs (136, 158-171),
  the owner's screenshots (183) and the city reviews (175 on). Move each
  find here as a numbered line, then fix them by one rule.
- One run of this task is one PR in size: take the most visible cause, fix
  it everywhere, merge on green CI, then the next cause. The task stays
  open while finds of its kind keep coming.
- Speed and geometry work touch the renderer: never run this alongside a
  speed task (172, 156, 157).

## Acceptance

- Each PR: the rule it adds, before | after crops on at least three maps,
  ctest and the regression.
- The Log lists every find of this kind marked fixed (PR) or open.

## Out of scope

Other kinds of objects (their own batches), per-location fixes from the
owner's shots that are not of this kind.

## Log
