---
id: 141
title: The 3D options checkboxes line up: an empty box stands where a ticked one does
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

- 2026-11-03 — claimed on fresh `main` (`cd05720f`) as `gpt/doing`;
  branch `task/141-checkbox-alignment`, claim commit `11b5f552`.
- The toggle renderer used right-aligned text, so `[ ]` and `[X]` had
  different left positions. It now reserves the `[X]` advance, draws both
  states from that fixed left edge, and left-aligns localized On/Off to a
  shared edge. The localized value edge moves right when needed to preserve a
  gap after the checkbox.
- Captures in `runs/141-checkbox-alignment/` use Klamath `kladwtwn`,
  1920x1080, scale 2. Before and after sheets cover EN/RU with pixel/sharp
  text; sidecars report `scene_view=full`. Sharp sidecars report active text
  rendering. Captures show several enabled and disabled toggles.
- `tools/configure-build.ps1 Release` passed. CTest passed 20/20, including
  `options_panel_model_tests`; the scene-export and regression Python suites
  passed 104 and 12 tests. `tools/regression.ps1` passed all 16 spots with
  zero changed pixels.
- The dev-snapshot input script completed and wrote all five snapshots for
  each capture. Its wrapper then reported its task-154-only guard that the
  run did not change resolution and scale; capture sidecars and input logs are
  retained. No owner game process or owner cache was used.
- Four before/after JPEG sheets are `evidence-en-pixel.jpg`,
  `evidence-en-sharp.jpg`, `evidence-ru-pixel.jpg`, and
  `evidence-ru-sharp.jpg`; each is below 1 MB. The paired sidecars and scripted
  input logs are retained in their baseline/after folders.
- Task remains `doing`, `reviewed: no` until the pushed PR CI is green.
