---
id: 055
title: Bold small interface font
milestone: M1
status: doing
owner: gpt
suggested: gpt
claude-only: no
user-check: yes
depends-on: [054]
reviewed: no
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
