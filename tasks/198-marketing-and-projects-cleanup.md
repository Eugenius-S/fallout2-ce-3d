---
id: 198
title: Marketing files in one folder, the projects folder cleaned up
milestone: M1
status: todo
owner: claude
suggested: claude
priority: normal
tier: 4
claude-only: yes
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The showcase project, its final videos and the logo sit in one git-excluded
`marketing/` folder of the main checkout, and the projects folder holds only the
live checkouts and the folders the owner keeps. A local machine task: no
game or code change.

## Context

- The owner, 2026-11-04: gather the video project, the final video and the
  logo into a marketing folder, and clean up the cluttered root.
- Stands alone: no open task covers the local machine.
- What is there (2026-11-04):
  - the projects folder holds 96 entries: about 85 stale `fallout2-ce-3d-NNN`
    worktree folders, `f129*`, `f2ce-bisect-103`, `fo2-main129`, stray logs
    and scripts (`before-build.log`, `f129-build*.log`, `regression-129.log`,
    `tour-129.log`, `f129-roofs.py`), and the owner's `Showcase_2.mp4` (the
    1080p showcase, on YouTube as rFMGHM7ZNDc), `logo_final.png`,
    `logo_final.psd`. `git worktree list` shows 99 worktrees.
  - `marketing\showcase-097` already exists in the main checkout, excluded
    through `.git/info/exclude`: `showcase-v1.mp4`, `showcase.aep`, footage,
    flights.
  - The live After Effects project is `game\shots\097\showcase.aep` (saved
    2026-11-04 18:32), its footage folders under `game\shots\097`.
  - The repository's copy of the logo is `docs/images/logo.png`.
- Part A, `marketing/` (git-excluded, never committed):
  - `showcase/project`: the current .aep with every piece of footage it uses
    (After Effects Collect Files, or move and relink), so it opens with no
    missing media;
  - `showcase/final`: `Showcase_2.mp4` and `showcase-v1.mp4`;
  - `logo`: `logo_final.psd` and `logo_final.png`.
  - Before moving anything out of `game\shots\097`, confirm the collected
    project opens with nothing missing; the sources stay until then.
- Part B, the projects folder:
  - Keep `fallout2-ce-3d`, `-claude`, `-gpt`, `-glm` and every worktree whose
    branch has an open PR or uncommitted work.
  - Every other worktree: `git worktree remove` when its branch is merged
    or gone and its tree is clean; otherwise list it. Then
    `git worktree prune`.
  - Stray logs and scripts move to an archive folder, never deleted.
  - Folders that are not worktrees (`fallout2-art`, `f2ce-bisect-103` and
    the like): list them and ask the owner.
  - Before deleting anything, show the owner the keep / remove / ask lists
    and wait for an OK. Nothing with uncommitted work or the owner's files
    is deleted.

## Acceptance

- the projects folder holds only the live checkouts and the folders the owner
  approved; the before and after listings are in the Log.
- `marketing/` is in place and the After Effects project opens clean.

## Out of scope

The video edit itself (097), the repository's logo.

## Log
