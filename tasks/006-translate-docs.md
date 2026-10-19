---
id: 006
title: Translate the repository to English
milestone: M0
status: todo
owner: none
suggested: glm
claude-only: no
user-check: no
depends-on: []
reviewed: no
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
