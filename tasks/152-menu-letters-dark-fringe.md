---
id: 152
title: The main menu's yellow letters have no dark pixel fringe
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

The yellow button labels of the main menu ("Вступление", "Новая игра",
"Загрузить" and the rest) read clean: no black pixels around the letters
that the original menu does not have.

## Context

- The owner, 2026-11-02, live on main `a4d47b26`, Russian:
  `runs/inbox/2026-11-02-menu-letters-dark-fringe.png`, "round the yellow
  letters there seems to be a black pixel".
- 126 fitted these labels to the buttons; the menu text goes through
  `ce/src/scene/menu_text.cc`. Suspects: a dark outline or shadow pass, an
  edge of the scaled glyphs blending with black instead of the plate, the
  palette's colour 0 left around the glyphs. Compare with the game's own 2D
  menu at the same size.

## Acceptance

- before | after crops of the menu labels, English and Russian, next to the
  original 2D menu, in `runs/152-menu-letters-dark-fringe/`.
- ctest passes.

## Log

- 2026-11-02 Cut from the owner's live find.
- 2026-11-03 gpt
  - Done: fitted high-resolution labels now blend directly into the main-menu framebuffer, so antialiased edge pixels sample the actual panel instead of black in a temporary zero-filled buffer.
  - Visual: same-build and same-config live baseline/fix captures in English and Russian; each pair uses 1920x1080, logical 960x540, scale 2, and the main menu with the 3D scene off. JPEG sheets: `runs/152-menu-letters-dark-fringe/evidence-en.jpg` and `runs/152-menu-letters-dark-fringe/evidence-ru.jpg`. Matching capture metadata records language, resolution, scale, and draw path. The capture-only hook was removed from submitted source.
  - Check: Release build succeeded; `ctest --test-dir build-task152 -C Release --output-on-failure` passed 20/20 tests.
  - Next: none.