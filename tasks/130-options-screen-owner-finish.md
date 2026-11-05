---
id: 130
title: The 3D options screen finished the way the owner says, ready for the showcase video
milestone: M1
status: doing
owner: none
suggested: any
priority: normal
tier: 3
claude-only: no
user-check: yes
depends-on: [112]
reviewed: no
---

## Goal

The options screen with the 3D panel looks finished to the owner, so it can
be shot for the showcase video (097, segment 11: every 3D feature switched
off one by one, a slider moved).

## Context

- The owner, 2026-11-01: the screen needs more work before it goes into the
  video, done to the owner's notes. The notes come in chat; the agent asks
  the owner for them when it takes the task and copies them here, with the
  pictures in `runs/inbox/`.
- Builds on 112 (buttons under the panel, straight scrollbar, side frame,
  shadow density 0.8; PR #158) and the layout of 017
  (`runs/017-options-menu-3d-panel-owner-brief.md` on the owner's machine).
  Code: `ce/src/scene/options_panel.cc`.
- Owner note, 2026-11-04 (folded in, the UI batch): a cursor movement speed
  setting: the settings also need the cursor's movement speed. Check first whether the existing `[preferences]
  mouse_sensitivity` in `fallout2.cfg` can carry it: CE ignores it for the
  absolute cursor in a window, so the speed may need its own scaling of the
  cursor's motion there.

## Acceptance

- Every note of the owner's copied into Context holds; before | after of
  the whole options screen at 16:9 and 4:3 in
  `runs/130-options-screen-owner-finish/`.
- A cursor speed slider in the settings visibly changes how fast the
  in-game cursor moves, in windowed and in fullscreen, and is saved in
  `fallout2.cfg`.
- ctest passes; the original view regression stays pixel for pixel.

## Out of scope

The game's own preferences page art, the video itself.

## Log

### 2026-11-05 gpt
Done: Added the cursor speed setting and windowed absolute-cursor scaling; the English and Russian 16:9 and 4:3 panel comparisons are in `runs/130-options-screen-owner-finish/`. Release build and CTest passed earlier (20/20). The initial production probe failed to stage a slider value. A second attempt, adjusted to start at the track midpoint as the existing zoom probe does, failed identically. Both isolated probe logs are in `runs/130-options-screen-owner-finish/cursor-speed-probe/` and `cursor-speed-probe-2/` in the worktree; they are not presented as runtime proof.
Next: Diagnose why the production probe's simulated mouse position does not reach the cursor speed slider, then verify saved config and windowed/fullscreen movement before claiming acceptance.
Check: `tools/dev-snapshot.ps1 -PreferencesProbe cursor-speed`; both attempts reported `PREFERENCES_PROBE FAIL: cursor speed slider did not stage a new value`.
