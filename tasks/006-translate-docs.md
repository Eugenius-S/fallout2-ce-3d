---
id: 006
title: Translate the repository to English
milestone: M0
status: done
owner: glm
suggested: glm
claude-only: no
user-check: no
depends-on: []
reviewed: yes
---

## Goal

Everything tracked in the repository reads in English.

## Context

On 2026-10-19 only `README.md` and seven files in `docs/` contain Russian
(`git grep -I -c -P '[\x{0400}-\x{04FF}]' -- ':!runs'`); code and scripts are
already in English. The task touches no code, so it does not need the game and
can run in its own worktree next to another agent.

## Acceptance

- `git grep -I -n -P '[\x{0400}-\x{04FF}]' -- ':!runs'` finds nothing.
- The meaning is kept; nothing is added. Plain, short sentences; no new
  sections, tables or explanations that were not in the original.
- Dated historical documents keep their dates and names; their file names are
  translated where they are Russian, with every link updated.
- Links between documents still work.

## Out of scope

Rewriting or updating content. Wrong or outdated statements go into the `Log`
as notes for Claude instead.

## Log

### 2026-10-19 glm
Done: README.md, docs/game-setup.md, docs/p0-gl-presentation.md,
docs/c1-camera-prototype.md, docs/stage-a-report.md translated; the
Cyrillic glyph names in tasks/026 replaced with words.
Next: docs/p1-devtools.md, docs/2026-10-17-fallout2-ce-3d-env-p0-p1.md,
docs/2026-10-17-fallout2-sprite-lighting-design.md; then the acceptance
grep and the PR.
Check: git grep -I -n -P '[\x{0400}-\x{04FF}]' -- ':!runs' (must be
empty; currently only the three docs above remain).

### 2026-10-19 glm
Done: docs/p1-devtools.md translated (200 lines, full history kept).
Total translated: README.md, docs/game-setup.md, docs/p0-gl-presentation.md,
docs/c1-camera-prototype.md, docs/stage-a-report.md, docs/p1-devtools.md,
tasks/026 glyph names.
Next: translate docs/2026-10-17-fallout2-ce-3d-env-p0-p1.md (336 lines) and
docs/2026-10-17-fallout2-sprite-lighting-design.md (223 lines), then run the
acceptance grep and open the PR.
Check: git grep -I -n -P '[\x{0400}-\x{04FF}]' -- ':!runs' (only those two
docs remain).

### 2026-10-19 glm
Done: docs/2026-10-17-fallout2-ce-3d-env-p0-p1.md and
docs/2026-10-17-fallout2-sprite-lighting-design.md translated. All tracked
files read in English; dates, names, structure and cross-links kept
unchanged.
Notes for Claude (outdated statements kept as written, per Out of scope):
both dated docs still reference the old project root
E:\Projects\fallout2-sprite-lighting in pins/commands; the design doc's
§2 keeps the pre-GitHub handoff text that §8 later supersedes; README's
"light perspective" wording predates the game-like view of docs/2026-10-17
section 12.
Check: git grep -I -n -P '[\x{0400}-\x{04FF}]' -- ':!runs' finds nothing.

### 2026-10-19 claude (review)
Accepted. The acceptance grep finds nothing outside `runs/`; every relative
link in the Markdown files resolves; headings and list items match the
originals one for one, and spot checks of both dated documents keep the
meaning without additions.
