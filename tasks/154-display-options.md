---
id: 154
title: Resolution, window mode and picture scale in the 3D settings
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: high
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The player sets the screen in the game, in the 3D settings' first section
("Основные" / Main), without editing fallout2.cfg:
- a dropdown with every resolution the display offers;
- a dropdown: fullscreen, window, borderless;
- below them a slider for the picture and interface scale, 1.0x to 3.0x in
  steps of 0.2x.

## Context

- The owner, 2026-11-02, on the panel
  (`runs/inbox/2026-11-02-display-options-place.png`): "add a resolution
  choice here (a dropdown with all available resolutions), a dropdown
  fullscreen / window / borderless, below a scale slider from 1x to 3x in
  0.2x steps". High priority.
- Folds in 139 (fractional scale), whose context still holds: today the
  scale is the integer `[screen] scale` (`settings.screen.scale`,
  `ce/src/svga.cc` around line 174), the logical framebuffer is the window
  divided by it and must stay at least 640x480; a fractional factor makes
  the logical size the window divided by it, rounded, and the final picture
  is stretched without blur (nearest or sharp-bilinear). The sharp text and
  the 3D view draw at the window's real resolution. A factor that would
  leave less than 640x480 is not offered (the slider stops there, or the
  step greys out with a note).
- `[screen] resolution_x`, `resolution_y` and `windowed`
  (`WindowMode`, `settings.h`) are the stored keys. The resolution list comes
  from SDL's display modes for the window's display, without duplicates,
  largest first; the current one is selected even if SDL does not list it.
  Borderless is a desktop-sized window without frame on the current display.
- The panel (`ce/src/scene/options_panel.cc`, labels in
  `ce/src/scene/menu_text.cc` in English and Russian) has sliders and
  checkboxes but no dropdown yet: add one that fits the panel's look (the
  Pip-Boy green, the existing row height), opens over the rows below and
  closes on a choice, Escape or a click outside.
- A change applies when the panel closes if the engine allows it without a
  restart; where it needs one, the row says so and the value is saved.
  A resolution or mode that fails falls back to the previous one.
- An old integer `scale` in a player's ini keeps working and reads as that
  factor.

## Acceptance

- Shots of the open dropdowns and the slider, English and Russian, and of
  Klamath at 1.0x, 1.4x, 2.0x and 3.0x on a 16:9 window, plus one
  borderless and one windowed run, in `runs/154-display-options/`: the
  interface fits, clicks land where the cursor points.
- At 2.0x the picture matches today's `scale=2` pixel for pixel; the
  original view regression stays pixel for pixel; ctest passes, with a
  settings test for the new keys and for reading an old integer `scale`.

## Out of scope

The 3D camera zoom, new art for any size, the dialogue font clipping (140).

## Log

- 2026-11-02 Cut from the owner's request; 139 folded in.

### 2026-11-02 gpt
Done: added resolution and window-mode choices, the 1.0x-3.0x scale slider, fractional logical sizing, legacy integer-scale loading, and English/Russian labels. Release build and ctest pass. Python scene-export tests pass (88 tests) and regression unit tests pass (12 tests). Captured Klamath at 1.4x and English/Russian options-panel evidence under runs/154-display-options/.
Next: inspect the no-snapshot timeout from the Russian mode-dropdown capture, then complete the remaining scale, window-mode, click-coordinate, and original-view regression checks before marking done.
Check: `cmake --build build/task154`; `ctest --test-dir build/task154 --output-on-failure` (18/18); `py -3 -m unittest discover -s tools/scene_export -p 'test*.py'` (88); `py -3 -m unittest discover -s tools/regression -p 'test*.py'` (12). One isolated `kladwtwn` 1920x1080 capture at 1.4x passed (logical 1371x771, drawable 1920x1080). The second UI capture attempt timed out without a snapshot; see draft PR #217.

### 2026-11-02 gpt
Done: merged current `origin/main` at `fd8329b4`; added `<cmath>` for `std::round` in `ce/src/settings.cc` and pushed `c75e2a92`. Rebuilt the Windows Release tree from clean outputs with `tools/configure-build.ps1 Release`; `ctest --test-dir build --output-on-failure` passes (18/18). CI run 41387413946 passes `windows-game` and `scene (settings_3d)`. Fresh Klamath evidence is in `runs/154-display-options/current-20261102/`; all sidecars identify `ce_commit=c75e2a92` and executable SHA-256 `04a1e42fd6a9ef0e035816131d5b19d3b3ede01376a7046f8060d633b8f9b322`. Scales 1.0x, 1.4x, and 2.0x use 1920x1080; 3.0x uses 2560x1440 because 1920x1080 would leave less than the required 640x480 logical framebuffer. English resolution choices are visible in the panel at the bottom scroll position.
Next: fix the snapshot helper so it scrolls the target row into view, then add real dropdown/slider click proof. Capture Russian mode choices and test windowed and borderless using the actual SDL window flags. Compare the original view and scale=2 legacy output pixel for pixel before completing the task.
Check: three attempts to capture the requested dropdown evidence did not show the popup: English at top was clipped, Russian mode at top was absent, and Russian mode at bottom was offscreen. In `ce/src/scene/options_panel.cc`, `showPanelForSnapshot` only selects scroll 0 or `maxScroll()` (628-630), then `showDropdownForSnapshot` opens the row without making it visible (633-645); `dropdownVisibleItemCount` computes from its current screen position (922-933). The English resolution popup at bottom is visible, but the Russian mode popup and real clicks remain unverified. Runtime `--dev-screen` sets `settings.screen.windowed = WindowMode::Windowed` (`ce/src/devtools/devtools.cc:1226-1228`), and snapshot `windowed` records that setting rather than SDL window flags; no borderless/windowed mode proof or original-view regression/pixel comparison has been captured. PR #217 remains a draft.
