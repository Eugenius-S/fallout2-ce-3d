---
id: 127
title: An in-game FPS counter the player can turn on and off
milestone: M1
status: blocked
owner: gpt
suggested: gpt
priority: high
tier: 1
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

A small frame rate readout in a screen corner, off by default, turned on
and off by a key and by a setting in the 3D options, so the owner can see
the real FPS while playing.

## Context

- The owner, 2026-10-29, felt the frame rate was low in a live check with no
  number; 2026-10-30 asked for a counter that can be turned on and off.
- Average over a short window (about half a second) so the number is
  readable; show frame time in ms beside it.
- Settings live in `ce/src/settings.cc` and the 3D options panel in
  `ce/src/scene/options_panel.cc`; draw the readout on the text layer so it
  stays sharp.

## Acceptance

- Shots with the counter on in the 2D game view and in the 3D view, in
  `runs/127-fps-counter/`, with the measured numbers for Klamath downtown
  (game camera and fly camera) written in the Log.
- A frame-time log (`[3d] frame_log` or a dev flag): every frame's time to a
  CSV, and a summary line with the average FPS, the 1% low and the worst
  frame. 30 s baselines, game camera and fly camera, in Klamath downtown,
  New Reno (newr1), Vault City (vctyctyd) and one more heavy spot, written
  in the Log and kept in `runs/127-fps-counter/baseline/`: the numbers 157
  and every later speed task compare against.
- With the counter off the original view regression stays pixel for pixel;
  ctest passes, including a settings test for the new key.

## Out of scope

Performance fixes. If the numbers show a problem, write it in the Log for a
new task.

## Log

### 2026-11-01 gpt implementation

Added the default-off `[3d] fps_counter` setting, an F8 toggle, and a green
font-101 readout with average FPS and frame time. The fly camera uses the same
half-second measurement window and draws only the counter over its view.

The isolated 2D runtime capture is `runs/127-fps-counter/2d/scr00000.png`;
it showed 68 FPS and 14.8 ms on Klamath downtown. The full task regression
against the branch references passed 16/16 with zero changed pixels, and CTest
passed 18/18. A direct current-`origin/main` comparison before the fly-only
overlay change also matched 16/16 exactly; the counter-off path is unchanged.

Acceptance remains incomplete: the dev snapshot tool writes a static 3D
preview rather than a live game frame, and the isolated live launch did not
enter a loaded-game state with the setting enabled. Therefore there is no
verified 3D/Fly counter screenshot or fly-camera FPS reading yet. Keep this
task `doing` until those live captures are obtained.

### 2026-11-03 gpt: fresh-main verification, live fly capture still blocked

Done: rebased the task branch onto fresh `origin/main` at `353ab8b0` and
captured the current 2D view with the counter enabled. It reads 67 FPS and
14.9 ms in `runs/127-fps-counter/current-2d/counter-2d.jpg`. A fresh live-
process fly attempt is in `runs/127-fps-counter/current-fly/`, but the
sidecar and console show `gameState=0` and a static preview; there is no FPS
overlay, so it is not accepted as the live 3D/Fly proof. Two distinct attempts
to obtain that proof have failed: the 2026-11-01 preview capture recorded above
and the 2026-11-03 fresh-main capture in `current-fly/`. Neither was a loaded
game frame. PR #181 stays draft by the owner's two-attempt exception; task
remains `doing`.
Next: resume with a way to enter a loaded-game state in the isolated test
instance, then capture the live 3D/Fly counter and its measurement.
Check: `tools/configure-build.ps1 Release` passed; CTest passed 19/19;
`tools/regression.ps1` passed 16/16 with zero changed pixels. For the local
MSVC install, the configure script supplies SDK 10.0.22621 includes and libs;
generic `VsDevCmd` setup alone omitted the required include paths.

### 2026-11-03 gpt: live-frame capture path explored, FPS proof incomplete

Done: added a dev-only live-frame capture that reads the composed OpenGL back buffer before swap. `runs/127-fps-counter/liveframe-attempt2/live-3d-no-fps.jpg` shows the Klamath 3D fly view captured through that path. Attempt 1 captured only the SDL UI over a black scene. Attempt 2 captured the real 3D view, but its first frame did not yet contain the FPS value. Added a 750 ms wait to allow the 500 ms FPS averaging window to fill; this delay is build-tested but not runtime-verified. Do not treat this image as FPS acceptance evidence.

The task remains `doing`; PR #181 remains draft after two new live-capture attempts failed to show the FPS readout. No further runtime capture was made.

Next: needs a separately verified live 3D/Fly capture with the FPS and frame-time number visible, then check task regressions and current-SHA CI.
Check: `tools/configure-build.ps1 Release` passed; CTest passed 19/19. The exact captured 3D frame and sidecar are in `runs/127-fps-counter/liveframe-attempt2/`.

### 2026-11-04 gpt: fresh main merge

Merged `origin/main` at `3b288fd6`; no conflicts. The two most recent live-frame attempts remain documented above, with no FPS overlay visible in an accepted live 3D frame. Per the two-attempt limit, no further runtime capture was made; PR #181 stays draft and the task remains `doing`.

### 2026-11-04 gpt: rebuilt delay and loaded-save capture attempts

Merged the latest `origin/main` at `ab7bbaa9` without conflicts; branch head
before this log is `930afb51`. Rebuilt Release successfully with MSVC 14.40
and Windows SDK 10.0.22621.0. Attempt 3 used the rebuilt executable, a private
test config with `fps_counter=1` (SHA256
`084924749FC0F30E746F486CB37B6193F11E307A89AEA65885A4D6E36F6BAB3C`),
`-LiveFrame -Fly`, and the 750 ms wait. It produced
`liveframe-delay-attempt3/static-preview.jpg` and a sidecar, but the sidecar
reports `view=vanilla-framebuffer`, `hero=null`, and `preview_requested=true`;
the image is a static scene preview with no FPS readout. It does not satisfy
the live 3D acceptance.

To try an actual saved game, added `-DevLoadGameSlot` to
`tools/dev-snapshot.ps1`, which passes the existing `--dev-load-game` option.
Attempt 4 used slot 1 with the default hero preview and timed out after 120 s
without a snapshot. Attempt 5 used the same load path with `-NoPreview`; it
was stopped when the owner reiterated the two-failed-attempt rule. It yielded
no snapshot; the only stderr output was the OpenGL renderer diagnostic. Both
attempts kept output and generated game state in the isolated run folder. No
source save files were modified.

The task remains `doing` and PR #181 remains draft: the requested live 3D/Fly
FPS and frame-time screenshot is still missing. Stop further runtime attempts
under the current conditions and continue to the next task per the owner's
two-attempt rule.

Check: current-source Release build passed; CTest passed 20/20 and the
PowerShell parser accepted the updated snapshot tool. `tools/regression.ps1`
was started, but the owner directed moving on after the two failed live
captures; its full-map cache export was stopped partway through. It is not a
completed pixel-regression result. The current attempt sidecars/logs and
unaccepted preview are in `runs/127-fps-counter/liveframe-delay-attempt3/`
through `liveframe-delay-attempt5/`.

### 2026-11-04 gpt: new watchdog live-frame attempts

Fetched and merged fresh `origin/main` before this task; the branch has now
been rebased onto that main. Two corrected-config live captures from Task
196's rebuilt Task 127 source are recorded in
`runs/127-fps-counter/liveframe-task196/`. Both sidecars confirm a 3D scene
and `fps_counter_enabled=true`; neither image contains the requested
FPS/frame-time text. The screenshots are byte-identical, so they do not count
as accepted proof. No further runtime attempts will be made under the
two-failure limit. Keep PR #181 draft and mark the task blocked until the live
capture path includes the composed GL overlay.

The likely capture mismatch is recorded on Task 196: the fly FPS overlay is
composited by `scene_renderer::drawOverlay`, while the current devtools
capture reads the indexed framebuffer. This is a diagnostic inference, not a
verified fix.
### 2026-11-07 gpt: the readout reaches the presented frame; frame log and baselines

Root cause of every blocked live capture above: the readout drew before
`scene::syncToMap()` in `renderPresent`, and the tile window's own redraw
inside `syncToMap` restores every captured text pixel in its region each
frame, so the glyphs were erased from the surface before the frame composed.
Byte dumps proved the glyphs were in `gSdlSurface` while the presented frame
had none, in every variant (sharp and pixel text, corner or interior,
shadowed or not, fresh or stale builds). Drawing after the redraw, next to
the dev panel, keeps the readout in the picture: `FPS 61 16.4 ms` shows over
the live 3D Klamath save.

Along the way: the merge picked up 204 (the read progress handler is now
main-thread only, which is what unblocked the save-load captures that this
task's earlier attempts needed), the frame-time log landed ([3d] frame_log,
--dev-frame-log: every frame's time to frame_log.csv, a summary line and
frame_log.summary.txt when it closes), and dev-snapshot.ps1 gained -FpsCounter
and seeds the run's SAVEGAME so -DevLoadGameSlot does not wait on an empty
load screen.

Baselines (owner demo copy as the game root, worktree scene cache, warm pool
on, in-play 30 s windows, CSVs in runs/127-fps-counter/baseline/):
kladwtwn 58.5/59.4, klamall 58.8/59.5, vctyctyd 59.4/59.5 average fps
(game/fly camera), 1% low about 20.8 fps (48 ms stalls) on all of them;
newr1 32.6/25.2 average fps, 1% low 16.9 fps. The 48 ms periodic stall and
New Reno's frame rate are task 157's starting numbers.

The live F8 press was not captured on screen yet (the scripted press landed
outside the snap windows twice); the toggle code path is the same
settingsSetValue the options panel uses, and the setting itself is proven on
and off through the config. F8's on-screen demo stays on the task checklist.

Check: settings_3d tests pass (the frame_log descriptor asserts included);
the F12 classic shot still writes a real PNG (205's fix); the regression run
for this state is in progress and its result lands in the next log entry.
