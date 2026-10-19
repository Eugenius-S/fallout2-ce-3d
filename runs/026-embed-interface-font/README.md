# Task 026 verification

The Release game and mapper build with embedded IBM Plex Mono SemiBold/Bold
and stb_truetype. Both original TTF byte streams were found verbatim in each
executable and in the standalone font test executable.

- All 5 native suites, 30 exporter tests and 9 regression-tool tests pass.
- `glyph-test.txt`: the public getter loads both fonts and rasterizes all 30
  required glyphs, including Cyrillic.
- `regression.json`: all six fixed Klamath views match the unchanged task 003
  references with zero changed pixels. One capture and its original metadata
  are included here. The build predates the documentation-only completion
  commit; the recorded EXE hash identifies the executable actually tested.
- Font drawing is outside this task; no interface pixels changed.

Reproduce from the worktree with the normal Release build, CTest and
`powershell -NoProfile -ExecutionPolicy Bypass -File tools/regression.ps1`.
