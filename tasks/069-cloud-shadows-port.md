---
id: 069
title: Cloud shadows move onto the new shadow code
milestone: M1
status: doing
owner: gpt
suggested: glm
claude-only: no
user-check: yes
depends-on: [062]
reviewed: no
---

## Goal

Task 023's drifting cloud shadows (PR #74, branch `task/023-cloud-shadows`)
land in main on top of the shadow code as it is after 063 and 062.

## Context

- 023 was built on an early 045; main's shadow code has been rewritten
  since (the AA ring, the rim fade, textured casters, a 4096 map), so the
  branch no longer merges (14 conflicts, the shader's `sunDim` among them).
- Port the idea, not the diff: the procedural cloud noise, the [3d]
  `cloud_shadows` key with its menu label, `-Shadows` turning it on in
  background runs. Close PR #74 when the new one merges.

## Acceptance

- Shots at noon with and without clouds under `runs/069-cloud-shadows/`.
- The regression with shadows off stays where it is.

## Out of scope

Anything 062 owns.

## Log

### 2026-10-30 gpt
Done: Ported procedural cloud shadows to the current shadow renderer. Added the `[3d] cloud_shadows` setting and menu label, independent cloud-only and cloud-phase dev snapshot options, and cloud masking that scales ambient sunlight while leaving point-light contribution unchanged. Noon on/off and phase captures are in `runs/069-cloud-shadows/`; night and underground controls showed identical on/off pixels. The final one-line guard in `scene_draw.cc` clears the cloud uniform for floor-only previews.
Next: Re-run the fly tour and full direct pixel regression on the exact final build. The latest regression attempt stopped during `klagraz-e0`, so acceptance is not complete. Keep this PR draft until those checks finish.
Check: Release build and `ctest --test-dir build --output-on-failure` passed 18/18 after the final guard. Earlier tour (before that guard) reported 0/43 changed spots. Earlier full main-vs-branch comparison (also before the guard) was pixel-identical at all 16 spots; current baseline reference run has known drifts at `klagraz-e0` (183 px) and `klaratcv-e2` (6299 px). Latest attempted regression used the explicit `build/scene-cache`; it stopped during capture. Do not treat earlier tour/regression as final-code validation.
