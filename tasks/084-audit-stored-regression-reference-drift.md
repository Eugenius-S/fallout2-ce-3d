---
id: 084
title: Audit stored visual regression references against the current base
milestone: M1
status: todo
owner: none
suggested: claude
priority: normal
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

Make the stored 16-spot regression useful again by explaining the image drift
since its last reference update, preserving protection against real regressions.

## Context

- The references were captured on 2026-10-22, source `32f0fe2b`, and last updated
  by `8543064e` (PR #34). Several rendering changes have since merged.
- Task 066 encountered 13 mismatches and 3 passes against those references.
  Its base-engine control at `04dbdf88` reproduces all 13 mismatches; all 16
  task captures match that control exactly, with no channel tolerance.
- Evidence and per-spot counts:
  `runs/066-dialog-black-background/baseline-comparison.json`.
- A changed triangle/page count in the entrance metadata and differences near
  scene objects show that this audit must consider renderer and scene-cache
  changes, rather than assume a dialogue/UI problem.

## Acceptance

- Identify the merged changes responsible for each class of image difference.
- Separate intended changes from actual bugs; do not automatically accept all
  current pixels or loosen tolerances to silence the check.
- Correct unintended changes in bounded tasks. Update references only for
  explained, intended changes, with before/after evidence and the repository's
  required review for reference updates.
- The full 16-spot regression passes using the committed references and the
  documented resource/cache configuration.

## Out of scope

Dialogue backdrop 066, unrelated visual redesign, and changing the owner's
running game or resource files to make the comparison pass.

## Log

### 2026-10-25 gpt

Done: isolated the reference drift from 066 with 16 base-engine control captures.
Next: audit differences from source `32f0fe2b` to the current main branch.
Check: task 066's committed `baseline-comparison.json` records 0 task-vs-control
changed pixels at every spot and the independently reproduced reference drift.
