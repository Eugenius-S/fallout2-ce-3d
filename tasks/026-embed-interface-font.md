---
id: 026
title: Embed the interface font
milestone: M1
status: doing
owner: gpt
suggested: glm
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

The game executable carries the interface font and a TrueType rasterizer, so
the sharp text layer (task 025) only has to draw.

## Context

- The owner chose IBM Plex Mono: SemiBold for text, Bold for titles (SIL Open
  Font License): <https://github.com/google/fonts/tree/main/ofl/ibmplexmono>.
- Rasterizer: `stb_truetype.h` from <https://github.com/nothings/stb>
  (public domain or MIT), placed like the other single-file libraries in
  `ce/third_party/` (see `lodepng`).
- Unit test suites are standalone CMake projects in `ce/tests/<suite>/`
  (see `sprite_card`), run by `.github/workflows/verify.yml`.

## Acceptance

- `ce/assets/fonts/ibm-plex-mono/` holds `IBMPlexMono-SemiBold.ttf`,
  `IBMPlexMono-Bold.ttf` and `OFL.txt`, downloaded from the link above.
- `ce/third_party/stb/` holds `stb_truetype.h` from a tagged or dated commit
  (record which in the file's directory) and its license.
- The build embeds both TTF files into the executable (CMake generates a
  source with byte arrays at build time; the TTF files stay the only source).
- `ce/src/scene_renderer/embedded_fonts.{h,cc}`: returns the bytes and size of
  the SemiBold or Bold font.
- Unit test suite `ce/tests/embedded_fonts/`: both fonts load in
  stb_truetype; glyphs exist for `W`, `Ж`, `ё`, `Щ`, `?`, digits; the suite is
  in the `verify.yml` matrix.
- The game builds and the other test suites pass.

## Out of scope

Drawing text (task 025).

## Log

### 2026-10-19 gpt
Done: claimed task 026 on the owner's explicit priority after task 004 merged
as PR #3 (`691ba6e`). The existing GPT worktree is reused to retain its build
cache. No font or renderer changes have been made yet.
Next: pin and vendor IBM Plex Mono SemiBold/Bold with OFL and stb_truetype
with its license, add build-time byte embedding and glyph tests. Until task
002 merges, preserve existing scene_renderer/devtools files; the owner has
been asked whether the new embedded_fonts files are an allowed exception.
Task 009 is explicitly deferred until task 002 merges.
Check: inspect this task's header on origin/main and the branch
`task/026-embed-interface-font`.
