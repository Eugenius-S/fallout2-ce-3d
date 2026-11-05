---
id: 200
title: Refine installer spacing and control shapes
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: high
tier: 1
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

Refine the installer layout from the owner's 2026-11-05 screenshot.

## Context

- Follow-up to completed task 199, split out because the owner gave a new direct visual correction after #285 merged.
- Keep the existing installer behavior and bilingual layout.
- Reference: the owner's screenshot attached in the task conversation.

## Acceptance

- The content has equal outer margins, with no outer inset frame.
- The logo is smaller and closer to the game-folder path field.
- Buttons and fields have 6 px corner radii.
- The log panel and progress bar have 6 px corner radii.
- Path text and language dropdown text are vertically centered.
- Current build is shown in verified English and Russian screenshots.
- Build and relevant tests pass.

## Out of scope

Installer behavior, translation licensing, update and rollback flow.

## Log

### 2026-11-05 gpt
Done: Removed the inset gold frame and rounded the path field, language button, buttons, log panel, and progress track. Reduced the logo and its gap to the path field, and centered the path and language labels. Added `--no-activate` for capture only; normal launcher activation is unchanged.
Check: Release build passed. Fresh English and Russian captures are in `runs/200-installer-layout/` and match binary SHA-256 `986A4656D3DC1DDD896661D960EC4D7AB45D2321AB243A1128D4C80E2BEACB88`. Captures use a private settings file and display-only install path; no install started. Programmatic path edit round-trip passed, the language popup opened, and the progress control returned a test position of 65. Actual worker log output and autoscroll were exercised in task 199's install/update/rollback run (`runs/199-installer/install-cycle.jpg`); this change does not modify the logger or append/scroll code and retains the multiline/autoscroll/vertical-scroll styles. A current-build rerun was blocked by shell policy before the worker started.
Attempt: Two attempts to rerun actual multiline worker output through an isolated temporary game folder were rejected by shell policy before the launcher worker started. The owner's game and cache were not involved.
Next: Commit this final log note and await CI for PR #290 before marking it ready.
