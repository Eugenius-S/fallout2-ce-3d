---
id: 126
title: Russian main menu labels fit their buttons
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

Every main menu label stays inside its button row, in Russian and English,
at 16:9 and 21:9, and no label runs over the red round button.

## Context

- The owner, 2026-10-29: `runs/inbox/2026-10-29-main-menu-russian-labels.jpg`.
  «ВСТУПЛЕНИЕ» and «НОВАЯ ИГРА» extend left over the red button in their rows.
- First check whether stock fallout2-ce with the Russian data does the same.
  If it does, the fix is a general rule (shrink or fit the label into the
  space between the button and the row's right edge), not a per-label
  offset. If it is ours (the wide menu art of task 118, the text layer, the
  scaling), fix ours.
- The menu loader and layout live in `ce/src/mainmenu.cc`.

## Acceptance

- before | after of the main menu in Russian and English at 16:9 and 21:9,
  in `runs/126-menu-labels-fit/`.
- Every menu button still works by a mouse click (say how it was checked).
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

The menu art, its framing (task 118), the font choice and the translation.

## Log

- 2026-11-01: Checked the stock Fallout 2 executable with the Russian data; the long labels also reach into the red-button area, so this is a shared label-fit issue.
- Added one shared text column based on the high-resolution button panel bounds. Labels are centered and horizontally fitted between the red-button area and the row's right inset. The classic menu path remains unchanged.
- Captured origin/main before and task/126 after views for Russian and English at 16:9 and 21:9. Labeled side-by-side JPEGs are `runs/126-menu-labels-fit/russian-16x9.jpg`, `russian-21x9.jpg`, `english-16x9.jpg`, and `english-21x9.jpg`.
- Clicked all six rows in the isolated `fallout2-ce-test.exe` copy using native mouse input. New Game, Load Game, Options, Credits, and Exit showed their expected transitions or exit. Intro reached `MAIN_MENU_INTRO`; with `DEBUGACTIVE=log`, `debug.log` recorded `Playing movie: intro.mve` and `Playing movie: credits.mve`. The same log then reports `Movie error` after one frame for each movie. The test copy does not show successful movie playback, so Intro is not accepted as fully verified.
- `ctest --test-dir build --output-on-failure`: 18/18 passed.
- Full scene regression against current `origin/main`, using `build/scene-cache`: 16 spots passed with 0 changed pixels.
- Status remains `doing` until Intro playback can be verified with a working movie asset/runtime.

- 2026-11-01 Closed at merge (#183); the owner checks it live.
