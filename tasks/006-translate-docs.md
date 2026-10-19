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

`README.md`, most of `docs/`, some code comments and script messages are in
Russian.

## Acceptance

- `git grep -n -P '[\x{0400}-\x{04FF}]'` finds nothing outside `runs/` and
  outside test data that must contain Cyrillic on purpose.
- The meaning is kept; nothing is added. Plain, short sentences; no new
  sections, tables or explanations that were not in the original.
- Dated historical documents keep their dates and names; their file names are
  translated where they are Russian, with every link updated.
- Links between documents still work.

## Out of scope

Rewriting or updating content. Wrong or outdated statements go into the `Log`
as notes for Claude instead.

## Log
