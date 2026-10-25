---
id: 066
title: The dialogue screen hides the street behind black
milestone: M1
status: done
owner: gpt
suggested: glm
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

While a dialogue is open, the free screen area around the dialogue window is
black, so the street world does not show around it.

## Context

- The owner's find of 2026-10-24, with their reference:
  `runs/inbox/2026-10-24-dialog-black-background.png`.
- On a wide screen the dialogue window covers only the middle; the 3D scene
  keeps drawing around it.

## Acceptance

- Snapshots of one dialogue at the owner's resolution and at 1920x1080 under
  `runs/066-dialog-black-background/`: everything outside the dialogue
  window is black, the window itself unchanged.
- Leaving the dialogue brings the street back the same frame.

## Out of scope

The dialogue's own text and layout.

## Log

### 2026-10-25 gpt

Done: added an opaque full-screen backdrop owned by the dialogue background
window in `ce/src/scene/dialogue_backdrop.*`. Its synchronous destruction
refreshes the exposed world. Dialogue art and text are unchanged. Fixed the
dialogue fixture's premature readiness and normal-return checks. Evidence:
`runs/066-dialog-black-background/` (2310x1298, 1920x1080, street after goodbye).

Next: investigate the existing regression reference differences before marking
this task done or merging. Do not retake references automatically. The complete
16-spot run has 3 passes and 13 reference mismatches. A control build using
`origin/main` versions of both modified engine files produces exactly the same
entrance image as this task (0 changed pixels), including its 501-pixel mismatch
with the reference. The other 12 mismatches have not had individual controls.
No owner game was restarted or installed over. Task 065 was not started.

Check: Release build; CTest 15/15; exporter tests 52/52; regression unit tests
12/12. Raw owner-resolution before/after dialogue interior: 0 changed pixels;
outside: 1,763,236 nonblack pixels before, 0 after. Both final raw dialogue
captures have 0 nonblack pixels outside the window. A PID-scoped background
helper selected goodbye option 7 and captured the restored 3D street. Local
full regression: `runs/regression/20261025-191452-400-19392/summary.txt` (not
committed); incomplete local cache was exported and missing captures retried.
Control capture: `build/control-entrance/`; control EXE:
`build/fallout2-ce-control.exe`. Final patched EXE was rebuilt afterward.

### 2026-10-25 gpt, regression diagnosis

Done: all 16 control captures from the base engine match the task captures
exactly, with channel tolerance 0 and 0 changed pixels at every spot. All 13
stored-reference mismatches also reproduce in the control build. Recorded
per-spot counts and executable hashes in
`runs/066-dialog-black-background/baseline-comparison.json`. Stored references
date from 2026-10-22, source `32f0fe2b`; the task base is `04dbdf88`.

Next: no remaining work in 066. Task 084 independently audits the old reference
drift; it must not automatically accept unintended changes. No reference PNG
or engine data format was changed in this task. Task 065 was not started.

Check: `py -3 build/verify-controls.py` validates both sets with the standard
capture validator and strict pixel comparison. The normal regression checker
also passes 16/16 against the temporary current-base reference set:
`py -3 tools/regression/check.py --run runs/regression/20261025-191452-400-19392
--reference build/current-base-reference`. The committed old references still
have 13 mismatches; their original summary is preserved in
`build/stored-reference-summary.json`. The final patched EXE remains in build.
