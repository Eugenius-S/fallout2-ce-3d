---
id: 141
title: The 3D options checkboxes line up: an empty box stands where a ticked one does
milestone: M1
status: todo
owner: none
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

In the 3D options panel the brackets of every checkbox sit in one column,
ticked or not: "[ ]" takes the same place as "[X]", the space where the X
would be keeps its width.

## Context

- The owner, 2026-11-01, a screenshot in chat: rows "[X] On", "[ ] Off",
  "[X] On"; the empty box sits narrower and shifted right against the
  ticked ones, because the space is narrower than X.
- `ce/src/scene/options_panel.cc` around line 758 draws the box as one
  string, `value ? "[X]" : "[ ]"`, right aligned by
  `drawRightAlignedControlText`, so the narrower string moves. Draw the
  brackets at fixed positions computed from the "[X]" width and the X
  centered between them (or nothing there when off), so both states share
  the same left and right bracket. Check the On / Off column the same way.
- Both languages and the sharp and pixel text paths.

## Acceptance

- before | after crops of several toggle rows on and off, English and
  Russian, at x2, in `runs/141-checkbox-alignment/`: the brackets line up.
- ctest passes (the options panel model tests); the original view
  regression stays pixel for pixel.

## Out of scope

Other rows, the panel layout (130).

## Log
