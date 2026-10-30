---
id: 069
title: Cloud shadows move onto the new shadow code
milestone: M1
status: done
owner: gpt
suggested: glm
claude-only: no
user-check: yes
depends-on: [062]
reviewed: yes
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

Done: Ported procedural cloud shadows to the current renderer, added the `[3d] cloud_shadows` setting and menu label, dev controls for cloud-only and fixed cloud phase, and guarded floor-only previews from cloud shadows.

Check: Release build from `e4ce37b4` passed; branch EXE SHA256 `c720405430a9ba3bec12eef9394a19906e9980d2600ae3c0a54a6d4cb8c97fea`. CTest passed 18/18, exporter tests 67/67, regression tests 12/12. Exported 13 map/elevation caches from fresh `origin/main` and this branch; scene fingerprints matched. Direct 16-spot comparison against `origin/main` (`8e78b4e9`, EXE SHA256 `65e22dba52ce9f28748988edecd89f1a047bf3ec1f8a4ad1dce41de5a483e482`) was pixel-identical: 0 changed pixels. References were not changed.

Check: Final fly tour compared 140 before and 140 after sidecars with the matching main and branch EXEs: 0 of 43 spots changed with shadows off. Cloud-only captures at KLADWTWN tile 25075, noon, differ by 820,652 pixels off-to-phase-0 and 972,464 pixels phase-0-to-phase-12. Floor-only off vs phase-12, night off vs phase-12 (sun strength 0), and KLATOXCV elevation 2 off vs phase-12 each match at 0 pixels. Evidence: `runs/069-cloud-shadows/final-guard-cloud-phases.jpg` and `runs/069-cloud-shadows/fly-tour-final/changed.jpg`.

Next: Merge PR #169, then close superseded PR #74 as requested by this task.
