---
id: 200
title: Refine installer spacing and control shapes
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: high
tier: 1
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

Refine the installer layout from the owner's 2026-11-05 screenshot.

## Context

- Follow-up to completed task 199, split out because the owner gave a new direct visual correction after #285 merged.
- Keep the existing installer behavior and bilingual layout.
- Reference: `C:\Users\jiraf\AppData\Local\Temp\codex-clipboard-b6b0e12b-c37e-4b72-ab17-1b8b17b4377a.png`.

## Acceptance

- The content has equal outer margins, with no outer inset frame.
- The logo is smaller and closer to the game-folder path field.
- Buttons and fields have 6 px corner radii.
- Path text and language dropdown text are vertically centered.
- Current build is shown in verified English and Russian screenshots.
- Build and relevant tests pass.

## Out of scope

Installer behavior, translation licensing, update and rollback flow.

## Log

### 2026-11-05 gpt
Done: Claimed the follow-up on a fresh branch from origin/main after task 199 PR #285 merged.
Next: Implement the visual changes, capture current-build RU/EN screenshots, and verify the build.
Check: branch `task/200-installer-layout`, base `3bf6634c`.
