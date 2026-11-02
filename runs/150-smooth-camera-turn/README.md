# 150: the camera turn step eases in and out

One press of `[` or `]` turns the camera 10 degrees over about 56 ms as a
smooth zero-to-zero ride: the speed ramps in at kCameraTurnAccel, peaks at
the old constant speed (360 deg/s), and the braking arc lands exactly on the
target. A press mid-turn retargets while the speed carries over, so two quick
presses are one 20-degree arc. Held keys keep working: the keyboard repeat
(80 ms) re-targets a settled turn.

The math lives in `ce/src/scene/camera_turn.h` (`cameraTurnEaseStep`), driven
by `updateCameraTurn` in `scene_view.cc`; the old constant-speed move is gone.
The unit test `camera_turn_tests` pins the curve: zero speed at both ends,
exact landing, no backward move or overshoot, continuous speed across a
retarget, the limit and the held-key extension.

## Evidence

- `turn-frames-one-step.txt` and `turn-frames-double-press.txt`: the real
  game's per-frame angle and speed while turning (a diagnostic build of this
  branch printed them; the print is not in the merged code).
- `anchor-0deg.jpg`, `one-step-settled-10deg.jpg`, `double-press-settled-20deg.jpg`:
  the background test view at the start, after one step and after a double
  press. The sidecar JSON now also records `"camera_turn"` for every capture.

Captured on the test instance: `tools/dev-snapshot.ps1 -SceneInput <script>
-TurnKeys -OutputDirectory <dir> -CaptureConsole`, the scripts under
`build/dev-input-150-*.txt` (the `key` and `waitms` actions of the scripted
input, pumped by the main loop with `--dev-scene-input=`).
