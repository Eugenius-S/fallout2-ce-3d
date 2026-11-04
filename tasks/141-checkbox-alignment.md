---
id: 141
title: The 3D options checkboxes line up: an empty box stands where a ticked one does
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
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

- Live top and bottom captures of toggle rows in English and Russian, pixel and\r?\n  sharp text, at 1920x1080 scale 2, in
  `runs/141-checkbox-alignment/`.
- The `[X]` and `[ ]` boxes occupy identical slots and end on the slider
  track endpoint. Toggle values share the slider-value right edge.
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
  `evidence-ru-sharp.jpg`; each is below 1 MB. The paired capture sidecars
  are retained in their baseline/after folders.
- Draft PR #235 passed every required check on `116b1941`, including the
  Windows build and CTest. Status update commit follows; wait for its checks
  before marking the PR ready for review.
- The 2026-11-03 owner review requested a fixed right edge for all toggle values,
  checkbox right edges at the slider track endpoints, and the same bracket slot
  in checked and unchecked states. `drawCheckboxControl` now anchors both
  brackets to the track endpoint, and On/Off values share the fixed value-column
  right edge.
- Rebuilt the Release binary and captured the actual 3D options panel in EN/RU,
  pixel/sharp text modes, at the top and bottom scroll positions. All eight
  screenshots use 1920x1080 output, 960x540 logical resolution, scale 2, and
  the same executable SHA; sharp captures report sharp text active and drawn.
  The four inspected JPEG sheets and capture metadata are in
  `runs/141-checkbox-alignment/`.
- CTest passed 20/20; pixel regression passed 16/16 with zero changed pixels.
  The project capture wrapper also reports its unrelated task-154 display
  postcondition because this task intentionally leaves resolution unchanged;
  the retained screenshots and sidecars are valid.
- PR #235 remains draft pending current-SHA CI.
- Follow-up review found that the visible X in sharp text was left of the opening's
  center. The checkbox now centers original-font ink inside the measured bracket
  opening and applies the sharp font's horizontal bearing correction.
- Final live top/bottom pairs were recaptured in EN/RU and pixel/sharp modes at
  1920x1080, logical 960x540, scale 2. All eight sidecars use executable SHA
  `3aafe38a3bd065b674470b3c6e7b95f88aeec3e19b491e0012f3ee4f7fcff112`; sharp
  captures report active and drawn text. Evidence sheets and metadata were
  refreshed in this folder.
- Release build passed; CTest passed 20/20. `tools/regression.ps1` passed 16/16
  with zero changed pixels. Sharp pixel inspection shows the X ink centered to
  within one output pixel, the nearest alignment on the integer logical
  framebuffer at scale 2.
- The snapshot wrapper reports its task-154 resolution/scale postcondition
  after writing the requested live captures. No owner game process or owner
  cache was used. PR #235 remains draft pending CI on this follow-up commit.
