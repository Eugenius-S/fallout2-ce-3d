# Task 054 final validation

The accepted PNG, JSON sidecar, and fixture console log are in each listed
directory. Each sidecar records the exact source commit, executable SHA-256,
screen dimensions, scale, and sharp-text metadata for that capture.

| Screen | Mode | Source | Executable SHA-256 | Evidence |
| --- | --- | --- | --- | --- |
| Inventory | EN, 1920x1080, scale 2, sharp on | `f41156e4` | `3eacbce84eeb2ffe7d168fc76387f2f6d773e55a1590a830bc960a9372a4d251` | `en-x2-inventory-on/` |
| Inventory | EN, 1920x1080, scale 2, sharp off | `f41156e4` | `3eacbce84eeb2ffe7d168fc76387f2f6d773e55a1590a830bc960a9372a4d251` | `en-x2-inventory-off/` |
| Character | EN, 1920x1080, scale 2, sharp on | `f0d61572` | `04fe67d23bad30f2b385fa1d57b85f399cdcf6d2a8dd4490734d1e0924f6dda3` | `en-x2-character-on/` |
| Pip-Boy Status | EN, 1920x1080, scale 2, sharp on | `f0d61572` | `04fe67d23bad30f2b385fa1d57b85f399cdcf6d2a8dd4490734d1e0924f6dda3` | `en-x2-pipboy-on/` |
| Barter | EN, 1920x1080, scale 2, sharp on | `f0d61572` | `04fe67d23bad30f2b385fa1d57b85f399cdcf6d2a8dd4490734d1e0924f6dda3` | `en-x2-barter-on/` |
| Message log | EN, 1920x1080, scale 2, sharp on | `d38f5f35` | `a8aaace0a003bb5dab61064ee900a22c3e8f91c33508e2de31bdddc1ad41a376` | `en-x2-log-on/` |
| Pip-Boy Status | RU, 1920x1440, scale 3, sharp on | `d38f5f35` | `a8aaace0a003bb5dab61064ee900a22c3e8f91c33508e2de31bdddc1ad41a376` | `ru-x3-pipboy-on/` |
| Character | RU, 1920x1440, scale 3, sharp on | `d38f5f35` | `a8aaace0a003bb5dab61064ee900a22c3e8f91c33508e2de31bdddc1ad41a376` | `ru-x3-character-on/` |
| NPC dialogue | EN, 1920x1080, scale 2, sharp on | `d38f5f35` | `a8aaace0a003bb5dab61064ee900a22c3e8f91c33508e2de31bdddc1ad41a376` | `en-x2-dialog-on/` |

The fixtures use real engine screens and input. Pip-Boy is enabled with the
existing `start.pipboy=2` option in process memory only, then opened with P and
its Status page selected with event 500. The barter capture uses Sulik's real
dialogue and B key; it verifies exactly two white `$0` totals. The message log
capture uses the stock slash key on the Classic 2D map path so the vanilla
frame includes the HUD; the dated line came from `editor.msg`.

The inventory sharp-off capture retained 34 visible font-101 screen runs and
reported no active sharp pass or overlay. The sharp-on capture reported 34
overlay runs. Character, dialogue, barter, Pip-Boy, and log captures all
reported nonempty visible font-101 screen records and overlay pixels. Barter
reported two distinct white `$0` totals. The message log reported the in-game
date and time from `editor.msg` (July 25, 2241 at 8:24).

## Regression and tests

- Release build used `tools/configure-build.ps1`; final sidecar-bound binary
  embeds source `d38f5f35` and has SHA-256
  `a8aaace0a003bb5dab61064ee900a22c3e8f91c33508e2de31bdddc1ad41a376`.
- CTest passed 14/14.
- Scene exporter unit tests passed 44/44.
- Regression checker unit tests passed 12/12.
- Official off regression passed 16/16 with zero changed pixels. The copied
  summary is in `off-regression-16/summary.json` and `summary.txt`; the full run
  is `runs/regression/20261024-170942-005-1104/`.

Brightness changes remain a separate owner follow-up after review of coverage.
No brightness, font-size, or font-weight change was made in this task.
