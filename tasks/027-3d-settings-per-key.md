---
id: 027
title: 3D settings report changes per key and log a failed start
milestone: M1
status: doing
owner: gpt
suggested: gpt
claude-only: no
user-check: no
depends-on: [014]
reviewed: no
---

## Goal

Each 3D feature reacts only to its own setting, and a feature that fails to
start says so in the log, as `AGENTS.md` asks.

## Context

From the review of task 014. `settingsGetThreeDRevision()` counts changes of
the whole `[3d]` section; `scene::syncSettings` turns the full view on or off
at any new revision. With a second key (light, task 020) changing it would
switch the scene on again even if the player had it off. `syncSettings`
drops the error from `scene::enable`.

## Acceptance

- A feature learns about changes of its own key only (for example a revision
  per key, or the consumer keeps the last value it saw).
- When `[3d] enabled` asks for the scene and it fails to start, one log line
  says why, and the game stays in 2D.
- Unit test: changing another `[3d]` key does not change what the scene
  lifecycle does.

## Out of scope

The options menu (017).

## Log

### 2026-10-19 gpt
Done: Claimed task 027 on main and created `task/027-3d-settings-per-key`.
Next: Add per-setting revisions and keep failed scene starts in plain 2D with one reason line.
Check: `tools/tasks.ps1`; task branch pushed to origin.

### 2026-10-19 gpt
Done: Added monotonic revisions per registered setting, incremented only on effective config reads or applied edits, and retained the aggregate `[3d]` revision. Scene settings now read the `[3d].enabled` revision; lifecycle ignores revision changes when the enabled value stays the same. Tests change the real unrelated `system.interrupt_walk` descriptor and inject an unrelated aggregate revision; no synthetic `[3d]` key was added. Kept `syncToMap` as the single failed-start reporter.
Next: Run the normal 16-spot regression and compare the owner config hash before/after; send for parent review before PR.
Check: Release build; CTest 8/8. A background ARTEMPLE snapshot with an isolated empty scene cache stayed vanilla and logged one stdout failure reason: `floor.bin is missing or unreadable`; metadata is in ignored `build/task027-proof/artemple-empty-cache/`.

### 2026-10-19 gpt
Done: Completed the normal 16-spot regression without updating references; all 16 passed with 0 changed pixels. Curated the machine-readable summary and the ARTEMPLE empty-cache fallback PNG/metadata/stdout evidence in `runs/027-3d-settings-per-key/`. The fallback remained vanilla (`scene_view: off`) and emitted one failure reason line. The primary game config hash stayed `8D95CE8F0A11FDB975C2EAFE4836248DD4F9983074161BA4578EE463F2CCEF7F` before and after.
Next: Parent review; after resume, PR and merge by parent.
Check: Release build; CTest 8/8; exporter 38 and regression 12 passed (parent independent run); normal regression summary reports `success: true`, `update: false`, 16/16 pass, 0 changed pixels; `git diff --check` clean.
