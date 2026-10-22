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
- Replace the earlier fixed-footer and flat-panel appearance. Cover the whole
  game view at any resolution with an opaque, seamless metal texture tiled
  from the preferences art. The map must not show through.
- Keep the original preferences block at the top, including its controls and
  three buttons. Scroll the complete page as one unit, including that block
  and the new panel; do not pin the footer.
- Below the original block, build a resizable screen using a 9-slice from the
  game's FRMs. Prefer the large Pip-Boy screen; if its frame cannot be cut
  cleanly, use the message window or dialog screen. Keep the four corners
  fixed and tile the edges and interior so the frame grows with the setting
  count.
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
  map visible through the menu.
- The original preferences block and all its controls remain intact and
  functional at the top. The three original buttons scroll with the page.
- The new panel uses the preferred game FRM frame, or the message/dialog
  fallback, assembled as a tiled 9-slice. It grows with the number of settings.
- The panel lists every `[3d]` descriptor in registry order with its label and
  current value. It uses a switch, numeric slider, or choice control by type;
  unsupported value types remain visible. Use descriptor label message IDs
  when available and the English fallback label otherwise. New keys appear
  without menu code changes.
- The existing `OptionsSettingsModel` keeps edits staged. Done commits them
  before the classic preferences save; a failed commit stays open and shows an
  error. Cancel or Escape restores legacy preferences and discards staged
  `[3d]` edits. Defaults resets both sets of settings. Setting changes obey
  each descriptor's apply policy, appear at once or after the menu closes as
  specified, and save to `fallout2.cfg`.
- The scrollbar uses the original slider groove and red knob style. It scrolls
  the whole page by wheel, knob drag, groove click, and Up/Down, Page Up/Down,
  Home, and End. The original control art and hit regions are never clipped by
  the bar.
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