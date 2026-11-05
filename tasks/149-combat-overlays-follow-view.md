---
id: 149
title: Combat outlines, the hex cursor and floating text follow the 3D turn and zoom
milestone: M1
status: blocked
owner: gpt
suggested: any
priority: normal
tier: 3
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

On a turned or zoomed 3D view, what the game draws over the map in combat
sits on the thing it belongs to: the friend and foe outline follows the
critter's shown sprite, the hex cursor and its to-hit number sit on the
hex, floating text above the head.

## Context

- 135 step 1 (#211) draws the combat outlines from the scene module, the 2D
  outline moved to the hex the scene shows (`scene_overlay.cc`
  `drawOutline`). Codex on #211: `scene_objects.cc::shownArt()` picks a
  camera-relative sprite side while the outline uses `object->fid` and
  `object->rotation` at the original size, so a turned view outlines
  another silhouette and a zoomed view an unscaled one.
- Build the outline from the shown art, with the card's projection and
  scale (in the renderer, as an edge pass on the sprite card, or by
  drawing the shown frame's outline into the overlay at the card's size).
  Check the hex cursor and floating text the same way at zoom 0.5 and 2 and
  turn ±30.

## Acceptance

- Captures at zoom 0.5, 1, 2 and turn 0, +30, -30 in combat (146's fixture
  when it lands, or a still with highlighting on) in
  `runs/149-combat-overlays-follow-view/`, outline on the sprite in each.
- The 2D picture and the unturned zoom 1 view look as before.
- ctest passes.

## Log

- 2026-11-01 Cut from Codex's review of #211.
- 2026-11-02 The owner, live: the outlines keep their size on a zoomed view (`runs/inbox/2026-11-02-combat-outlines-unscaled.png`).

### 2026-11-05 gpt

Done: Turned/zoomed outlines now use `shownObjectArt()` and the projected
sprite card. Floating text anchors project from the same game-space tile and
height; cursor/tile anchors and ground/object picks use the active camera
projection. The old path is kept at zoom 1 with no turn. Preliminary live
captures and sidecars are in `runs/149-combat-overlays-follow-view/`.

Next: Capture the combat outline, hex cursor/to-hit number and damage text at
zoom 0.5/1/2 and turns 0/+30/-30, plus the unchanged 2D and unturned zoom 1
views. The available background save is not in combat. PR #220's combat
fixture remains draft after its two startup failures. A Klamath bark-fixture
run at zoom 2/turn 30 stopped after one scene capture and did not complete the
fixture. Its diagnostic image and metadata are in
`runs/149-combat-overlays-follow-view/`. No combat-overlay acceptance is
claimed; keep this PR draft.

Check: Release build passed; CTest 20/20, scene-export tests 125/125,
regression tests 12/12, `git diff --check`, `check_runs_size.py origin/main`,
and `check_public_content.py origin/main` passed. A live scene snapshot showed
30 degrees at zoom 0.5. The unturned probe snapshot showed the zoom 0.5 cursor.
The first probe used the wrong `-SceneCache` directory and correctly failed
with a missing `floor.bin`; the run passed after using `build/scene-cache`.
### 2026-11-05 gpt

Done: Added two inspected runtime JPEGs showing the live Klamath bark text at zoom 2 with no camera turn: `devhero-20261105-153127-000-zoom2-unturned-bark.jpg` and `devhero-20261105-153132-000-zoom2-unturned-bark.jpg`. Both have JSON sidecars; images are below 1 MB. The source PNGs and harness logs were preserved under ignored `build/149-runtime-text-zoom-source/`.

Next: Combat outline, cursor/to-hit and damage-text acceptance is still missing. This new text capture is unturned and does not establish combat behavior or camera-turn projection. Keep PR #300 draft.

Check: `dev-snapshot.ps1 -UiFixture bark -Zoom 2` completed with `UI_FIXTURE PASS`, first/replacement text visible, and clear of old runs. Inspected the captured game frames. The earlier passing build and test results remain applicable because this documentation/evidence-only update does not change code.

### 2026-11-05 gpt blocked
Done: Added a live unturned zoom-2 Klamath bark capture; the floating text fixture passes. It does not establish combat overlays or turned projection.
Next: Resume the combat acceptance when task 146's fixture works, or a separate safe combat save becomes available. Keep PR #300 draft.
Blocked: The combat fixture PR #220 is draft after two startup failures, and the available background save is not in combat. The one turned zoom-2 bark fixture attempt stopped after its initial scene capture; it was not repeated. No combat overlay acceptance is claimed.
Check: `runs/149-combat-overlays-follow-view/devhero-20261105-153127-000-zoom2-unturned-bark.jpg` and sidecar; #220 is still draft at 3751e138, #300 remains draft.
