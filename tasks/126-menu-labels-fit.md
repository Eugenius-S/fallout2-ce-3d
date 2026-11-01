---
id: 126
title: Russian main menu labels fit their buttons
milestone: M1
status: doing
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
