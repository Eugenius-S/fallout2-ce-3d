---
id: 108
title: The 3D bottom stop follows the map's drawn ground, with no black strip under it
milestone: M1
status: done
owner: gpt
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

At a map's bottom edge the 3D camera goes down as far as the map has drawn
ground and no farther: the exit zone is in view and nothing black shows
between the ground and the interface bar.

## Context

- 104 (PR #145) moved the 3D bottom stop down by a fixed 24 map pixels in
  `ce/src/map_edge.cc`. The exit zone is better in view, but at kladwtwn a
  black strip now shows above the interface bar
  (`runs/104-camera-bottom-edge-farther/kladwtwn-before-after.jpg`, top
  right); 48 pixels gave a wider strip. A fixed amount fits one map, not
  all: where the drawn ground ends differs per map and along the edge.
- The project rule (AGENTS.md): a 3D fix is a rule for every map, never a
  number tuned on one.

## Acceptance

- The 3D bottom stop is computed per map from where its drawn ground (the
  lowest row of floor the 3D view draws along the bottom edge) ends, so the
  screen's bottom above the interface bar reaches that ground and never
  past it. The fixed 24 goes away. Name the rule in the PR.
- Bottom-edge shots in the 3D view on kladwtwn, klagraz and one map of
  another town, before|after, in `runs/108-bottom-stop-follows-ground/`: no
  black row above the bar, the exit zone in view at least as much as with
  104.
- The other three edges and the original view stay as they are: the
  original view regression pixel for pixel; ctest passes.

## Out of scope

The other edges, the fly camera, filling the void past the map with art.

## Log

- Rule: **The last visible row stays on drawn ground.** For a candidate camera
  center past EDG's bottom stop, project every horizontal pixel on the actual
  map viewport row immediately above the interface bar through the current
  zoom and turn. Allow the scroll only when every sample lands inside an
  occupied `floor.bin` cell and the active map clip. No scene-color threshold
  is used. When the center passes EDG's bottom stop, extend only authored clip
  plane 2 to the matching floor-hull offset; planes 0, 1 and 3 stay authored.
  If plane 2 has no same-direction floor-hull normal, leave the authored clip
  unchanged and fail closed. The configured game resources contain 176 EDG
  files, all version 1; no shipped squareRect (version 2) file was found.
  Future squareRect clips without an aligned floor edge retain their EDG stop;
  support for those files is not claimed.
  The clip extension switches off again when the camera returns inside EDG.
  The fixed 24 map-pixel extension is removed.
- Canonical HUD captures, each 1920x1080 at scale 2, use the same exported
  scene cache before and after. The three before|after sheets are
  [`kladwtwn`](../runs/108-bottom-stop-follows-ground/accepted/kladwtwn-before-after.jpg),
  [`klagraz`](../runs/108-bottom-stop-follows-ground/accepted/klagraz-before-after.jpg),
  and [`denbus2`](../runs/108-bottom-stop-follows-ground/accepted/denbus2-before-after.jpg).
  Endpoints are Klad 28509→29111, Klagraz 24947→25549, and Den Bus 2
  27920→28923. Rows 876–879 in all three 104 captures are fully dark
  (1920/1920 pixels with max RGB < 16); in all three final captures each row
  has 0 such pixels. Den Bus 2's visible orange hatching is the map's exit
  grid: the exported scene has 126 exit-grid decals, named `Exit Grid`.
  The red X near the screen center is not used as exit-grid evidence.
  Capture metadata: 104 CE `68375f13`, EXE SHA256
  `28f38476ba5b722a044d5ced038f61fd45f7b1d822d9ed2658a99eb3eb4fabde`;
  final CE `2d185988`, EXE SHA256
  `16749698f7724deb2e950c63b46525f5f9f56328a848419e7f099aecd909a9c7`.
- Exit-grid evidence comes from Den Bus 2's HUD screenshot and its exported
  `Exit Grid` decal data; the center red X is not used. No sidecar camera
  projection or pixel-color component is used to identify the grid.
- Original 2D: re-captured all 16 `tools/regression/spots.json` Classic views
  on the final EXE and exact base `ff2d06df`, with the same Klamath-start
  `-Classic -Still -Tour` route. Pixel comparison: 0 changed pixels across all
  16 at 1920x1080. Sidecars confirm matching map/elevation/tile metadata.
- 3D default: re-captured the 16 fixed spots on the final EXE and exact base
  `ff2d06df`; direct RGBA comparison: 0 changed pixels in all 16. Frozen
  reference differences are the same on both builds: Klagraz 183 px,
  Klaratcv e0/e1/e2 3837/5185/6299 px, Klatoxcv e2 550 px. References were
  not changed. The Classic captures used the same exact base and final EXE
  SHA above; sidecars confirmed matching map/elevation/tile metadata.
- Other stops: left/right/up endpoints match exact base at tiles
  26723/29494/21486; all four clip planes match. Non-still right/up PNGs have
  578/1256 changed pixels between runs, so no pixel-identical claim is made.
  Bottom→up from tile 29111 returns to tile 21486 and restores authored plane
  2. With zoom 1.15, Klad reaches tile 29312 at +45° and 29111 at −45°.
- Timed real floor-row checks: Klad max 139 µs, Klagraz max 123 µs, +45° max
  157 µs, −45° max 230 µs; each accepted scroll used 3–4 candidate checks.
- Release build passed; `ctest --test-dir build -C Release
  --output-on-failure`: 18/18 passed. Python exporter tests passed 66/66 and
  regression tool tests passed 12/12. The fly tour against exact base `ff2d06df`
  and identical scene cache covered 43 spots on all three turns: 0 changed
  spots. The required
  [`changed.jpg`](../runs/108-bottom-stop-follows-ground/fly-tour-ff2d/changed.jpg)
  records this result; the intended change is confined to the three bottom
  edges above. Final EXE SHA256:
  `16749698f7724deb2e950c63b46525f5f9f56328a848419e7f099aecd909a9c7`.
