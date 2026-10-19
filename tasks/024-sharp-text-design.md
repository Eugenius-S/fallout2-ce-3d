---
id: 024
title: Sharp text layer: design
milestone: M1
status: doing
owner: claude
suggested: claude
claude-only: yes
user-check: yes
depends-on: []
reviewed: no
---

## Goal

Decide how interface text is drawn sharp at the window's real resolution in a
modern font while the game is scaled x2 or x3, and write task 025 from it.

## Context

- Text is drawn into the 8-bit game frame by `fontDrawText` and friends
  (`ce/src/text_font.h`); font 101 is `FONT1.AAF` (Pip-Boy, dialogs,
  inventory), fonts 0-9 are `.FON`. The frame is then scaled to the window.
- Text must keep its color, place, clipping by windows and the Cyrillic of the
  Russian localization (CP1251).
- The owner wants a more modern font; the font choice goes to the owner with
  sample snapshots.

## Acceptance

- Task 025 rewritten with the chosen approach, the files it touches and its
  acceptance; the font chosen by the owner and its license fits a public
  repository.

## Out of scope

Implementation.

## Log
