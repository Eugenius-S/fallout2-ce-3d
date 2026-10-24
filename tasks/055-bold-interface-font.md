---
id: 055
title: Bold small interface font
milestone: M1
status: done
owner: gpt
suggested: gpt
claude-only: no
user-check: yes
depends-on: [054]
reviewed: yes
---

## Goal

Use the existing embedded Bold 700 face for modern sharp rendering of font 101
instead of SemiBold 600. Preserve the existing font size, fitting algorithm,
placement, palette colors, and fades.

## Context

- Task 054 extended sharp text to every real font-101 draw while preserving
  original palette colors.
- `ce/src/scene_renderer/text_draw.cc` already has the embedded Bold face for
  font IDs 103 and 104. Use that same face for font 101 only.
- The captured font-101 line height is 9 logical pixels. Russian diacritics
  must still fit its visibility mask at physical scales 2 and 3.
- Font IDs 102, 103, and 104 keep their existing eligibility and face behavior.

## Acceptance

- Every sharp font-101 run selects the existing embedded Bold 700 face.
- Font 102 remains on the existing SemiBold path in renderer tests; font 103
  and 104 continue to share the existing Bold path.
- The existing 9-pixel line clipping tests for CP1251 `И`, `Й`, `Е`, and `Ё`
  still pass at scales 2 and 3.
- Original palette colors, line-height fit algorithm, glyph placement, and
  eligibility are unchanged.
- Release build, CTest, scene exporter tests, regression unit tests, and the
  official 16-point off regression pass. Capture genuine Russian inventory,
  dialogue, and character screens at scale 3 with sharp text off (original
  pixels) and on (Bold 700). Each before/after pair uses the same source,
  executable, private test setup, and 1920x1440 framebuffer.

## Out of scope

- Font size, point size, line height, advance layout, or brightness changes.
- Changes to other font IDs, original bitmap text, palette states, or capture
  eligibility.

## Log

### 2026-10-24 gpt
Done: claimed task 055 from `origin/main` at `d29e8cfb`.
Next: implement only the font-101 face selection and its focused face tests.
Check: task branch and claim commit pushed before code changes.

### 2026-10-24 gpt (visual and regression validation)
Done: font 101 uses the embedded Bold 700 face; font 102 remains on the
existing SemiBold path, and font 103/104 continue sharing Bold. Captured three
Russian scale-3 screens with sharp text off and on using the same binary.
Next: parent handles PR 82 CI, review, and merge.
Check: Release build at `1ddaa05f`; CTest 14/14, exporter tests 44/44,
regression checker tests 12/12, and official off regression 16/16 with zero
changed pixels. Evidence is in
`runs/055-bold-interface-font/accepted-20261024/final-validation.md`.
