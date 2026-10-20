---
id: 030
title: No long dashes in our texts
milestone: M1
status: done
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
  and some of our own code comments. Find them with git grep for the long dash character.
- Files that came from upstream fallout2-ce unchanged (most of `ce/src`, the
  mapper, platform code) stay as they are: change only text we wrote.

## Acceptance

- A git grep for the long dash character finds none in text we wrote; each remaining hit is in an
  upstream file, listed in the `Log`.
- Each sentence is rephrased with a colon, a comma, a full stop or
  parentheses; the meaning stays the same.

## Out of scope

Rewriting texts beyond the dashes.

## Log

### 2026-10-20 glm
Done: every long dash in our texts is rephrased with a colon, a comma or a
joining word - README.md, ce/CMakeLists.txt (one comment), ce/src/
window_manager.h (one comment), all six docs, tasks/030 (its own grep quote),
runs/p1-camera25-review-20261018/README.md. Meaning kept; no content added.
Remaining hits are upstream-only: ce/README.md lines 16 and 199 (upstream CE
text, unmodified by us); binary matches in runs/*.png and game assets are
byte coincidences.
Check: git grep -n for the long dash character over tracked text finds only
ce/README.md.
