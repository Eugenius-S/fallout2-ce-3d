# Task 017 acceptance checkpoint

## Checklist

- [x] Keep the staged settings model and render every registered 3D option in
  its group with shared row geometry.
- [x] Use the approved native frame, seamless metal background, and full-height
  scrollbar. At logical 640x480, the frame is x=16..623 and the scrollbar is
  x=628..639 with a 4-pixel gap; legacy controls and hit regions stay centered
  and unchanged.
- [x] Verify Save, Cancel, Escape, Defaults, and read-only Verify through the
  isolated production event loop with config SHA and fresh-process assertions.
- [x] Capture and validate all 16 Classic/3D, English/Russian, scale 2/3,
  Top/Bottom combinations. PNG dimensions, sidecar resolutions, scale,
  `preview_requested`, `scene_view`, and executable SHA match the matrix.
- [x] Post-rebase Release build, CTest (12/12), exporter tests (44/44), and
  regression unit tests (12/12) pass.
- [x] Run and attribute gameplay regression: it exits 1 with 13 failures and
  3 passes against the existing references. The candidate is RGBA-identical
  to the clean `32f0fe2b` control in all 16 spots; no references were changed.
- [ ] Owner decision on whether to replace the 13 stale references remains a
  gate for final task acceptance.

The metadata sidecars do not contain a language field. The matrix records the
official launcher `-Language` value from each run folder; the launcher wrote
that value to the isolated test config. `renderer=scene-gl` identifies the UI
compositor in both modes. `preview_requested` and `scene_view` identify
Classic (`false`, `off`) and 3D (`true`, `full`). Per-spot regression paths and
decoded RGBA hashes are in
[`regression-attribution-final-20261022/comparison.json`](../regression-attribution-final-20261022/comparison.json)
with a readable summary beside it.

## Draft pull request

**Title:** Build the 3D options panel into the scrolling preferences menu

**Body:**

The preferences menu now keeps the original controls and scrolls them together
with a native-framed 3D settings panel. Registered options appear in stable
groups and remain staged until Done; Cancel and Escape restore the original
values. The original-style scrollbar spans the window. At the 640-pixel
logical minimum, only the decorative frame is inset to leave a 4-pixel gap for
the bar while preserving the centered content axis and legacy hit regions.

Validation: Release build; CTest 12/12; exporter tests 44/44; regression unit
tests 12/12; isolated Save, Cancel, Escape, Defaults, and Verify probes; and
the 16-frame 2D/3D, English/Russian, x2/x3 Top/Bottom matrix in
`runs/017-options-menu-3d-panel/matrix.md`. The gameplay regression exits 1
against old references (13 fail, 3 pass), while all 16 candidate frames match
the pristine `32f0fe2b` control exactly in decoded RGBA. No references were
refreshed. Final task acceptance waits on the owner's baseline decision.
