---
id: 069
title: Cloud shadows move onto the new shadow code
milestone: M1
status: todo
owner: none
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
