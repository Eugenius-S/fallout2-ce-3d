---
id: 217
title: The Ctrl+F12 mark rectangles show on screen, the hint stops overlapping monitor messages, shot metadata speaks UTF-8
milestone: M1
status: done
owner: claude
suggested: owner
priority: high
tier: 2
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The owner, 2026-11-07 ~15:30, after playing with the F12/Ctrl+F12 keys: the
mark rectangles "не работает" — nothing appeared on screen while dragging
(the saved files prove the capture itself worked), the corner hint about
Enter and Esc overlapped another text, and "в игре она не отображалась" —
the mark only exists in the saved PNG.

## Cause

Three defects, all found in the owner's own shot metadata
(shots/dev/2026-11-07/20261107-152218-kladwtwn-e0.json):

- With sharp interface text on, the 3D present path rebuilds the screen from
  captured TEXT runs only (`drawSceneOverlayWithSharpText` → the prepared
  frame); raw pixels written into the indexed surface — the green outlines —
  are dropped. The hint (a real text run) survived, the rectangles did not.
- The hint drew at the bottom-left corner — the game's own monitor message
  line shares that corner, so the two texts stacked.
- The JSON kept game strings as raw cp1251 bytes ("╓хэЄЁ уюЁюфр"), and the
  window mode reported `"windowed":false` for borderless, which hid the
  owner's fullscreen reports during debugging.

## Fix

- `owner_shot::drawSceneOverlay()` re-draws the rectangles as RGBA patches
  straight over the presented scene via `scene_renderer::drawOverlay` (the
  same mechanism the fly-camera FPS readout uses); called after the scene
  overlay in both the full-view and fly-camera present paths.
- The hint moved to the top-left corner (the monitor messages keep the
  bottom-left) and goes through the menu catalog: «РАМКА: выделяй мышью,
  Enter - сохранить, Esc - отмена».
- Every owner-shot monitor message goes through `menuStatusText` with new
  catalog pairs (скриншот сохранён / рамка сохранена / рамка не удалась /
  рамка отменена / переход выполнен).
- The JSON converts game strings cp1251→UTF-8 and reports
  `"window_mode":"windowed|fullscreen|borderless"` instead of the boolean.

## Verification

- ctest 20/20 green.
- Demo copy, kladwtwn loaded, `--dev-owner-mark-once=10`: the automated mark
  saves clean + marked PNG + JSON; the new overlay pass runs inside the
  present path without disturbing the frame.
- The owner's original shots (marked PNG) confirmed the capture side was
  already correct; this fixes the on-screen side and the metadata.
