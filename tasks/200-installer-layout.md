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
Done: Removed the inset gold frame, balanced the outer spacing, reduced the logo and its gap to the path field, and rounded the buttons and fields to 6 px. Replaced the square native language combo with an owner-drawn rounded dropdown whose label is vertically centered. Added `--no-activate` for nonintrusive capture only; normal launcher activation is unchanged. Evidence is in `runs/200-installer-layout/`.
Check: MSVC Release build via `tools/configure-build.ps1 Release` passed. Captured the current launcher in English and Russian from desktop pixels with a separate settings file and a placeholder game path; no installation was started. See `runs/200-installer-layout/capture.json` for the build hash, resolution, and capture details. Both JPEGs are under 1 MB.
Next: CI for PR #290.
