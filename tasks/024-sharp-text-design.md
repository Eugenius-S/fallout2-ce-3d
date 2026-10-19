---
id: 024
title: Sharp text layer: design
milestone: M1
status: done
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

### 2026-10-19 claude
Done: the design is written into `tasks/025-sharp-text.md` (record and replay
of the game's text draws, erase of the pixel text in the presented frame, a GL
text pass at window resolution). Font sample sheets for the owner:
`runs/024-sharp-text-design/fonts-ru.png`, `fonts-en.png`. Heavier,
regular-width fonts match the game's look; condensed and light ones read too
small next to it.
Next: the owner picks the font; write the choice here and into task 025's
`Context`, set this task done.
Check: open the two sheets.

### 2026-10-19 claude
Done: the owner chose IBM Plex Mono SemiBold (closest to the game's text
width); titles use IBM Plex Mono Bold. SIL Open Font License. Written into
task 025.
Next: task 025.
Check: `runs/024-sharp-text-design/fonts-ru.png`.
