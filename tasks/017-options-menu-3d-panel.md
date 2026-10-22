---
id: 017
title: Options menu with a 3D panel
milestone: M1
status: doing
owner: gpt
suggested: gpt
claude-only: no
user-check: yes
depends-on: [014]
reviewed: no
---

## Goal

Every new setting is in the game's own options menu.

## Context

- The game's preferences window is implemented in `ce/src/preferences.cc`.
- The setting descriptors and staged `OptionsSettingsModel` were completed in
  the first part of this task. Keep that model layer and its registry-ordered
  rows; new `[3d]` settings must appear without adding menu-specific keys.
- The owner reference images and full brief are saved in
  `runs/017-options-menu-3d-panel/owner-brief.md` and next to it as
  `owner-reference-*`. Image 54 shows the original preferences block, which
  stays intact. Image 55 shows the desired scrollbar. Images 56 and 57 show
  possible FRM screen frames. Image 58 is the rejected flat green panel over
  the map and must not be used as the target.
- `runs/017-options-menu-3d-panel/owner-reference-scrollbar-full-height.png`
  shows the owner's latest requirement: the original-style scrollbar spans
  nearly the full window height with consistent small end margins.
- `runs/017-options-menu-3d-panel/owner-reference-background-repeat.png`
  records the rejected mirrored-tile result. The new metal texture must not
  create visible repeated spots, symmetric rosettes, or obvious tile seams.
- The latest references are `owner-reference-crt-corner-stair.png`,
  `owner-reference-legacy-background-edge.png`, and
  `owner-reference-small-tile-repeat.png`. They reject the stepped monitor
  corner, the visible edge of the original rectangular background, and the
  still-obvious repeat from the small grain patch.
- The latest UI references are `owner-reference-refinement-a55f0ffd.png`,
  `owner-reference-refinement-49f41cbe.png`,
  `owner-reference-refinement-20619958.png`, and
  `owner-reference-refinement-2724fe26.png`. They guide the native bezel,
  aligned control grid, and grouped 3D settings.
- Replace the earlier fixed-footer and flat-panel appearance. Cover the whole
  game view at any resolution with an opaque, seamless metal texture tiled
  from the preferences art. The map must not show through.
- Use a larger, low-contrast source tile or a runtime-assembled tile from the
  same game art. Do not use mirrored repetition, a tiny repeated grain, or
  visible seams. Use the same texture in page gaps, around original controls,
  and across the rest of the screen.
- Keep the original preferences block at the top, including its controls and
  three buttons. Scroll the complete page as one unit, including that block
  and the new panel; do not pin the footer.
- Preserve original control art and text, but crop away its large rectangular
  background between individual control blocks. Blend the cutouts into the
  same full-screen metal texture without changing their hit regions.
- Below the original block, build a resizable screen using a 9-slice from the
  game's FRMs. Prefer the large Pip-Boy screen; if its frame cannot be cut
  cleanly, use the message window or dialog screen. Keep the four corners
  fixed and tile the edges and interior so the frame grows with the setting
  count. The corner and edge cuts must meet without a stepped join.
  Draw the dynamic settings inside in green Pip-Boy text.
- Cut frame pieces at runtime using rectangle constants in code. Do not commit
  extracted or modified game art. A plain temporary frame is acceptable only
  as an intermediate step if the preferred FRM proves difficult to cut.
- At the minimum 640x480 logical resolution, preserve the full original
  controls and their hit regions. Place the scrollbar in a verified metal
  margin; when extra width is available, keep it outside the original 640-pixel
  block.

## Acceptance

- The complete screen is covered by a seamless tiled metal backdrop, with no
  map visible through the menu. Its texture has no obvious repeated spots,
  symmetric rosettes, small-grain repetition, or tile seams. The original
  rectangular background has no visible outer edge around the preserved
  control blocks.
- The original preferences block and all its controls remain intact and
  functional at the top. The three original buttons scroll with the page.
- The new panel uses the preferred game FRM frame, or the message/dialog
  fallback, assembled as a tiled 9-slice. Its corners and edges join cleanly,
  and it grows with the number of settings.
- The panel lists every `[3d]` descriptor in stable registration order within
  General, Lighting, Camera, and Other groups. Unknown registered keys appear
  in Other. Rows use a 28-pixel height for toggles/values and 40 pixels for
  sliders, with shared positions for drawing, hit regions, and scrolling.
  Labels share a left edge and controls share a right edge. It uses a switch,
  numeric slider, or choice control by type; unsupported value types remain
  visible. Use descriptor label message IDs when available and the English
  fallback label otherwise. New keys appear without menu code changes.
- Keyboard selection remains internal; the selected control has no extra
  decoration. Toggle state remains visible in its checkbox. Disabled and
  command-line-overridden settings keep a distinct muted yellow color.
- The existing `OptionsSettingsModel` keeps edits staged. Done commits them
  before the classic preferences save; a failed commit stays open and shows an
  error. Cancel or Escape restores legacy preferences and discards staged
  `[3d]` edits. Defaults resets both sets of settings. Setting changes obey
  each descriptor's apply policy, appear at once or after the menu closes as
  specified, and save to `fallout2.cfg`.
- The scrollbar uses the original slider groove and red knob style. It scrolls
  the whole page by wheel, knob drag, groove click, and Up/Down, Page Up/Down,
  Home, and End. Its groove, knob travel, drag mapping, and hit region share a
  track spanning the window height with small equal end margins. The original
  control art and hit regions are never clipped by
  the bar. At the 640-pixel logical minimum, the current fallback track covers
  the rightmost 12 pixels of the outer frame bezel because there is no outside
  margin; it stays clear of the row hitboxes. Resolve this medium issue before
  final task acceptance.
- Works in 2D and 3D, at x2 and x3 scale, in Russian and English. Capture the
  window at the top and bottom for each combination of 2D/3D, x2/x3, and
  Russian/English: 16 snapshots in the PR, with the game frame at the requested
  scale.

## Out of scope

The sharp text layer (task 025); the panel uses the game's text drawing now.

## Log

### 2026-10-20 gpt
Done: Claimed task 017 after task 016 merged to main at `0c123c9e`. Reviewed the existing preferences screen and `OptionsSettingsModel`; the accepted implementation plan keeps the legacy page in a 640-pixel offscreen canvas, scrolls the content through a clipped viewport, and pins the existing footer.
Next: Add optional numeric range metadata and complete the descriptor/model layer with focused tests before integrating the scrollable UI.
Check: Fresh worktree is based on `origin/main` at `0c123c9e`; ignored build and private scene cache were preserved across checkout recreation. No implementation changes yet.

### 2026-10-20 gpt
Done: Added optional numeric range metadata to setting descriptors and schema parsing, projected staged settings into ordered panel rows by section and value type, and synchronized the registered apply policy for all three zoom settings. Added focused schema, row projection, and zoom apply-policy tests.
Next: Wait for owner review of this model-layer patch before integrating scroll, rendering, and input into the preferences window.
Check: Release build and CTest pass, 11/11 tests.

### 2026-10-22 gpt
Done: Integrated a modular, scrollable 3D options panel with dynamic registry rows, typed controls, staged edits, keyboard and mouse input, legacy control hitbox scrolling, and a fixed footer. Added an isolated preferences snapshot route and supplied numeric range metadata for glow strength.
Next: Get owner review of the first visible panel frame and integration before capturing the bottom rows and running the full 2D/3D language and scale matrix.
Check: Clean Release build completed, CTest passes 12/12, and the isolated 1920x1080 x2 preferences panel snapshot is saved at `runs/017-options-menu-3d-panel/scr00146.png`.

### 2026-10-22 gpt
Done: Superseded the previous fixed-footer and flat green panel appearance with the owner's full-screen metal backdrop, scrolling original block, FRM-based 9-slice, and original-style scrollbar brief. Preserved the completed descriptor and model layer, copied the owner brief and images 54 through 58 into the task run folder, and marked image 58 as rejected.
Next: Inspect the candidate runtime FRMs and report the planned cuts, full-page geometry, and affected preference hooks before changing implementation.
Check: Read `runs/017-options-menu-3d-panel/owner-brief.md` and inspect `owner-reference-*`; task context and acceptance now capture the new brief.

### 2026-10-22 gpt
Done: Replaced the fixed footer and flat panel with a full-screen scrolling preferences page, a 9-sliced Pip-Boy monitor, and the original FRM scrollbar. Added top and bottom Russian x2 captures for root review.
Next: Root review, then verify input, save and cancel behavior, and the full display matrix.
Check: Release build and CTest pass (12/12). Captures: `scr00154.png` and `scr00155.png` in `runs/017-options-menu-3d-panel/`.
### 2026-10-22 gpt
Done: Added bottom clearance so the last slider stays inside the 9-slice frame. Added an isolated language override to the official snapshot helper and captured the full 16-frame 2D/3D, x2/x3, Russian/English matrix.
Next: Add a narrowly scoped background interaction and persistence probe, then verify wheel, scrollbar, keyboard, slider, legacy controls, Done, Cancel, and Defaults.
Check: Release build and CTest pass 12/12. The 16 PNG and JSON pairs are indexed in `runs/017-options-menu-3d-panel/matrix.md`; x3 captures use 1920x1440.

### 2026-10-22 gpt
Done: Moved the minimum-width scrollbar to the unused right metal margin and reused the same x geometry for the visible groove and hit target. Retook all eight x3 captures and updated the matrix index; prior captures remain as history.
Next: Add a narrow isolated interaction and persistence probe through the preferences event loop.
Check: Full clean Release build and CTest pass 12/12. New x3 captures `scr00174.png` through `scr00181.png` use the 1920x1440 drawable.

### 2026-10-22 gpt
Done: Replaced the mirrored 152x38 backdrop sample with a non-mirrored 32x16 game-metal grain patch from the original preferences art, shared by the gap and full-screen fill. Added the owner's repeated-pattern reference and the controlled main-branch comparison evidence.
Next: Continue the isolated interaction and persistence probe.
Check: Official Release build passes. Root approved the fresh Russian x3 top and bottom frames at logical 960x600 in `runs/017-options-menu-3d-panel/background-fix-v2/`. The root's 16 gameplay captures match clean origin/main pixel-for-pixel; see `runs/017-options-menu-3d-panel/regression-attribution/README.md`.

### 2026-10-22 gpt
Done: Rebuilt the seamless metal tile from varied safe source crops and cleaned the Pip-Boy bezel joins. The black inset now follows rounded corners, and the bottom bezel reuses a vertically flipped native top cut for consistent thickness. Extended the original-style scrollbar to the full window height with shared 8 px top and bottom margins for its groove, thumb travel, drag mapping, and hit target.
Next: Continue the separately paused interaction and persistence probe; task 017 remains in progress.
Check: Root accepted the visual review. Release build and CTest pass 12/12. Russian x3 captures: `runs/017-options-menu-3d-panel/visual-fix-20261022/final-top-v4/scr00018.png` and `runs/017-options-menu-3d-panel/visual-fix-20261022/final-bottom-v8/scr00023.png`. The 30 logical top and bottom bezel rows match exactly after vertical flip across x=32..607 (155,520 pixels); the full-height scrollbar spans pixel rows 24..1775 at 3x, leaving 8 logical pixels at each end.

### 2026-10-22 gpt
Done: Replaced noisy vertical bezel sampling with a clean native 5-row strip. Centered the inner label/control grid on the existing 640-pixel legacy/frame axis, grouped every registered 3D row into General, Lighting, Camera, or automatic Other, and used compact 28-pixel toggle/value rows and 40-pixel slider rows. Replaced the left selection bar with a compact control outline and kept disabled text muted yellow. Copied four new owner references into `runs/017-options-menu-3d-panel/`.
Next: Continue the interaction and persistence probe, then resolve the scrollbar overlap with the right bezel at the 640-pixel logical minimum; task 017 remains in progress.
Check: Root approved the layout and bezel treatment. Release build and CTest pass 12/12. Final-code captures: minimum logical 640x480 bottom at `runs/017-options-menu-3d-panel/ui-refinement-20261022/minimum-bottom/scr00028.png` and wide logical 960x540 bottom at `runs/017-options-menu-3d-panel/ui-refinement-20261022/wide-bottom-final/scr00027.png`. Earlier top captures preceded the final color/slider-spacing adjustment and are not final review evidence. Group headings and enabled control ink measure RGB(60,248,0) against RGB(40,40,40) at 10.26:1. All 10 current descriptors are covered once in stable registration order within groups; row spans are disjoint, end at y=1002 inside the frame ending at y=1034, with 32 pixels of bottom clearance. Both side strips repeat every five logical rows with zero pixel mismatches across 25,600 comparisons per side; the 640-pixel viewport center x=320 maps to screen center x=480 at logical width 960. At 640x480 logical, the track spans x=628..639 and y=8..471, overlaying the rightmost 12 pixels of the 32-pixel outer bezel because no outside margin exists; controls and row hitboxes end at x=588 and x=598 respectively, so they remain clear. At wide logical 960, track x=804..815 stays outside the 640-pixel frame. Physical widths below 640 and Tab navigation were not tested; the capture helper enforces a 640x480 logical minimum, and this UI exposes arrow-key editing/scrolling without Tab navigation. The disabled style was not present in these captures; code keeps its muted-yellow text.

### 2026-10-22 gpt
Done: Removed the selected-control outline at the owner's request; kept the internal keyboard selection and checkbox state. Captured the wide Russian bottom view.
Next: Continue the interaction and persistence probe, then resolve the scrollbar overlap with the right bezel at the 640-pixel logical minimum; task 017 remains in progress.
Check: Release build and CTest pass 12/12. Capture: `runs/017-options-menu-3d-panel/outline-removal-20261022/wide-bottom/scr00029.png` with sidecar JSON. At 640 logical pixels, the 12-pixel track cannot fit outside the full-width preferences frame; moving it inside would cover frame art or the row hit area, so the existing overlap remains unresolved.

### 2026-10-22 gpt
Done: Added an isolated probe through the real preferences event loop for keyboard toggling, wheel and scrollbar input, slider dragging, a legacy preference control, Done, Cancel, Escape, Defaults, and read-only Verify. Cancel/Escape now clear the legacy dirty flag after restoring staged values, preventing an unintended preferences save. Defaults checks every editable registered 3D descriptor.
Next: Resolve the 640-pixel logical scrollbar/bezel overlap by insetting only the decorative frame, then verify narrow Save and Cancel; task 017 remains in progress.
Check: Release build and CTest pass 12/12. Seven successful scenario runs, including raw config SHA checks and a fresh-process readback, are in `runs/017-options-menu-3d-panel/preferences-probe-20261022/final-evidence/`. Each run has stdout and a `preferences-result.json` with initial/final values, config SHA-256, executable SHA-256, and marker-plus-harness-stop completion evidence. Save persisted zoom 7.9, enabled=false, combat difficulty=2; Defaults persisted zoom=1, enabled=true, combat difficulty=1 and checked all 10 editable registered 3D descriptors. Fresh Verify read both saved states. Cancel and Escape restored 3D/runtime and staged legacy values with unchanged config SHA.
