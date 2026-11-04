---
id: 154
title: Resolution, window mode and picture scale in the 3D settings
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: high
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
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

### 2026-11-02 gpt
Done: `showDropdownForSnapshot` scrolls its row near the upper third of the viewport before opening the list. Added `-ConfiguredDisplay` to `tools/dev-snapshot.ps1` to capture from an isolated existing config without `--dev-screen`; added SDL window flags, window size and drawable size to snapshot sidecars. Background test windows now request no activation for each display mode. English resolution and Russian window-mode dropdown images are captured under `runs/154-display-options/final-20261102/`.
Next: a physical mouse test at 1.4x with before/after images and saved-config values; valid SDL-flag captures for windowed and borderless; pixel comparisons for scale 1.0 against main and legacy scale 2 against scale factor 2.0. Keep the PR draft and the task `doing` until these checks and controller review are complete.
Check: Release build in `build/task154` and ctest pass (18/18); scene-export Python tests pass (89) and regression Python tests pass (12); PowerShell parser accepts `tools/dev-snapshot.ps1`. Two display-mode captures were attempted before the sidecar exposed SDL flags on preview frames. Both have `sdl_window.flags=0` and cannot prove either mode; the controller counted these as two failed captures and directed no further captures. No physical mouse clicks or pixel comparisons were made. The earlier UI captures are valid; the mode evidence is not. PR #217 remains a draft.

### 2026-11-02 gpt
Done: merged current `origin/main` at `201179a0` (merge commit `4728e121`). Replaced the two accepted UI PNG files with JPEG evidence under `runs/154-display-options/final-20261102/` and updated their JSON image paths; both JPEGs are under 300 KB. Rebuilt and checked after the merge.
Next: the physical mouse interaction and config readback at 1.4x, valid SDL-confirmed windowed and borderless snapshots, and the scale pixel comparisons remain. Keep PR #217 draft and task status `doing`.
Check: Release build `cmake --build build/task154` passed; ctest 18/18, scene-export Python tests 93 and regression Python tests 12 passed; `tools/dev-snapshot.ps1` parses and `git diff --check` is clean. No captures were run after the controller stop.

### 2026-11-02 gpt
Done: added SDL window position and classify `SDL_WINDOW_FULLSCREEN_DESKTOP` as borderless in sidecars. Captured configured modes without `--dev-screen`, using the isolated config and `build/scene-cache`. Windowed at 1920x1080 / factor 1.4 reports flags 73734, position -20000,-20000, window and drawable 1920x1080, fullscreen=false and borderless=false. Borderless at 3440x1440 / factor 2.0 reports flags 12295, position 0,0, window and drawable 3440x1440, fullscreen=true, fullscreen_desktop=true and borderless=true. JPEGs and JSON sidecars are in `runs/154-display-options/resumed-20261102/`; both are under 1 MB and identify build `c75e2a92` and executable SHA-256 `260789534b7595e25cccabd3cb51c68bb1912548a5b5377d7a0e2e8120a30ca8`.
Check: two directed `PostMessage` input attempts targeted the isolated, non-foreground test HWND at (1204,438), each followed by an F9 snapshot request (first with zero lParam, second with key/scancode lParam). Neither produced a new sidecar. The foreground remained PowerShell PID 7960 and the system cursor stayed at (1098,173). These messages only confirm queueing and do not establish a click or SDL mouse-state change. Source inspection shows the game reads absolute position from `SDL_GetMouseState` in `ce/src/dinput.cc`; the nonfocused messages did not update that state. The usual physical-input helper focuses the window, moves the system cursor and uses `SendInput`, all prohibited by the controller. No config readback or before/after click frames were obtained. The temporary `--dev-preferences-hold` test scaffold was removed: it only kept a panel open while attempting the unsupported nonfocused F9 path and had no validated use in the final implementation.
Build: incremental Release build reports no work; ctest 18/18, scene-export tests 93, regression tests 12, and `git diff --check` pass.
Still unverified: physical dropdown and slider clicks at 1.4x with before/after frames and saved config values; pixel match for scale 1.0 against main and legacy `scale=2` against scale factor 2.0. No further captures will be attempted after the two failed input-driven capture attempts. Keep task `doing` and PR #217 draft pending controller review.

### 2026-11-02 gpt
Done: compared decoded vanilla-framebuffer pixels on KLADWTWN/e0, view tile 23293, requested hero tile 25075, still, 1920x1080. Scale 1.0 against a separate build of origin/main 201179a0: 0 differing pixels, maximum channel difference 0. Main integer scale 2 versus task scale_factor 2.0: 0 differing pixels, maximum channel difference 0. JPEG sheets and JSON sidecars with executable hashes and capture metadata are in runs/154-display-options/pixel-comparison-20261102/. Both used --dev-scale; this compares rendered output, not a separate readback of legacy fallout2.cfg.
Check: a provisional opt-in --dev-input implementation pushed SDL mouse events and used a dev-only mouse-state override. Two input-driven attempts produced repeated identical screenshots and no configuration change. The second log contains only the first F9 push; the missing F9 key-up and repeated key processing are a suspected cause, not a validated fix. Per the user's stop condition no third attempt was made. The unverified code diff is saved locally at build/task154-input-unverified.patch and was not committed. Pixel comparisons used task executable SHA 448e2002684773c69dacf30beebc1a1b4a123771b486420fc5d8d1b406f811a0, whose embedded ce_commit c75e2a92 is stale; the sidecars record the actual hashes.
Next: implement and validate a complete press/release F9 sequence and scripted SDL mouse handling with strict per-step status, then capture before/after for each click, read the saved configuration and verify CI. Keep status doing and PR #217 draft.

### 2026-11-02 gpt
Done: tried the requested one-shot `snap` mechanism without F9 keyboard events. Each of two scripted-input runs produced five snapshots and logged SDL mouse motion and button events. The screenshot and event-log summaries are in `runs/154-display-options/snap-input-diagnostic-20261102/`.
Check: attempt 1 logged `DEV_INPUT snap`, `DEV_INPUT motion 910 244`, `DEV_INPUT button 1 at 910 244`, then motion and button events at `810 307`: the resolution dropdown opened, but the item selection did not take effect. Attempt 2 followed an experimental change to dropdown event handling: the popup did not remain open through selection, and subsequent slider, wheel and confirmation events did not produce the requested saved changes. Both logged five `DEV_INPUT snap` actions. The isolated config after attempt 2 contained `resolution_x=1920`, `resolution_y=1080`, `scale=2` and no `scale_factor`; this does not prove a successful selection or scale change. These are synthetic logical mouse coordinates, not a physical-pointer test. Per the owner's two-attempt stop condition there was no third run.
Next: investigate why the row event closes the dropdown before the selection and validate the full input path and config readback. The unvalidated source changes and test script remain locally in `build/task154-snap-input-unverified.patch` and `build/task154-input-snap.txt`; they are not part of the PR. Keep task doing and PR #217 draft.

### 2026-11-02 gpt
Done: Scripted SDL logical mouse input opens the resolution list, selects 2560x1440, drags the picture scale to 2.0x and presses Done while the running screen is 1920x1080 at 1.4x. The isolated fallout2.cfg reads resolution_x=2560, resolution_y=1440, scale_factor=2.000000 after applying; the active window remains at 1.4x until restart. Five JPEG frames, SDL metadata, input events and config hash are in runs/154-display-options/scripted-clicks-20261102/. Dropdown handling consumes the opening and selection mouse press until release, restores the popup background on close, and writes the legacy resolution keys alongside the pending choice. The dev-only input path uses SDL events in logical coordinates; it does not move the owner's system cursor or assert physical device behavior.
Check: clean build, ctest 18/18; Python scene-export 95 and regression-tool 12 tests pass. Full visual regression: 16/16, zero changed pixels (runs/regression/20261102-152136-838-72264). Previously accepted original-view scale comparisons and window-mode captures remain in this task's evidence. CI run 41412249631 passed all required checks, including windows-game and settings_3d. The controller checked all five post-merge frames and the isolated config hash. Main ff69d7cb was merged before the final interaction capture.
Next: owner review in PR #217; the changes take effect after the game restarts.