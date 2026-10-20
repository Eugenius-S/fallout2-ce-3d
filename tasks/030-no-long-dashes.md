---
id: 030
title: No long dashes in our texts
milestone: M1
status: doing
owner: glm
suggested: glm
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

Our texts read like plain spoken English: no long dashes (U+2014).

## Context

- The owner's rule (2026-10-20), now in `AGENTS.md`: never write the long
  dash in any text.
- About 180 of them today, in `README.md`, `ce/README.md`, `docs/`, `runs/*/README.md`
  and some of our own code comments. Find them with `git grep -n "—"`.
- Files that came from upstream fallout2-ce unchanged (most of `ce/src`, the
  mapper, platform code) stay as they are: change only text we wrote.

## Acceptance

- `git grep -n "—"` finds none in text we wrote; each remaining hit is in an
  upstream file, listed in the `Log`.
- Each sentence is rephrased with a colon, a comma, a full stop or
  parentheses; the meaning stays the same.

## Out of scope

Rewriting texts beyond the dashes.

## Log
