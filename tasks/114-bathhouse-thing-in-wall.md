---
id: 114
title: The thing in the bathhouse wall: find what it is and put it where the game has it
milestone: M1
status: doing
owner: gpt
suggested: glm
priority: high
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The grey thing sticking out of the bathhouse's alley-side wall is gone from
the wall in every camera, and whatever it is stands where the original game
puts it.

## Context

- 100 (merged, #147) decided the thing was two `Junk` scenery piles in the
  alley (hexes 16478, 16479) and turned the mapper's loose clutter back
  into cards. The owner, 2026-10-28, live on main `ff5f4ed0` (with 100):
  the thing is still there, and it is not alley junk.
  - `runs/inbox/2026-10-28-bathhouse-wall-thing-iso.png`: the 3D view at
    the game's angle, the grey thing on the wall left of the hero, by the
    corner near the door (also `2026-10-28-bathhouse-corner-piece.png`).
  - `runs/inbox/2026-10-28-bathhouse-wall-thing-top.webp`: flight, roof
    off. Outside the alley wall lies a grey pipe; inside against the same
    wall stand a box, a plate and a can; the table with its things is on
    the right. The owner: the things on the table moved off it and stuck
    in the wall, physically moved.
- So 100 named the wrong objects. Start from the objects, not a theory:
  list every object (pid, hex, elevation, FRM, flags, the art's own x/y
  offset) whose picture lands on that stretch of wall, inside and outside,
  and the table's things; compare where each stands in the original view
  at the same scroll and in the 3D scene.
- Suspects, none proven: the frame offset of the art (FRM shift) taken or
  dropped twice; a lift (things standing on tables) that resolves to the
  wrong table or none; a card anchored at the wrong hex.

## Acceptance

- The Log names each object and the line that put it in the wall.
- before | after with the original at the same scroll, the 3D at the
  game's angle and in flight with the roof off, inside and outside, in
  `runs/114-bathhouse-thing-in-wall/`; the other tables of Klamath with
  things on them checked the same way.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

The shapes of the things themselves (115, 116).

## Log

### 2026-10-31 gpt
Done: Rebuilt the branch after exporting `kladwtwn/0` only to `build/scene-cache` (fingerprint `029201f5f48fb5405639eb68b3f35335117997578136edd979de05362e326b03`). Read-only map inspection identifies the target as object 328, PID `Stuff`, FID 93 `tbltop11.frm`, tile 18282, map serial 1985. Its support is the merged `Table` solid formed by objects on tiles 17882 and 18282 (`ltable4.frm` on the latter). The lift solver selects solid 71, whose footprint contains the target anchor and whose top is z=0.54828. The current diagnostic build is SHA-256 `86c2f1f93a8d3206c01c0adc9cb4f36e2f174a379f4cc35b0160a2e43bd63b`; it is based on project commit `b136e33f` and was built with `tools/configure-build.ps1 Release`. A roof-off `-Solo 2` capture omits the merged support and therefore cannot prove the item is on the table; its compact diagnostic image and sidecar are in this folder.
Next: confirm a roof-off fly-camera angle that visibly includes both the merged table and target, then compare it with the ordinary camera and other Klamath tables before accepting any placement rule.
Check: explicit exporter command `py -3 tools/scene_export/export.py --game-root E:\Projects\fallout2-ce-3d\game --map kladwtwn --elevation 0 --cache-root build/scene-cache` succeeded. Build succeeded. No ctest or pixel regression was run after this branch build. The `-Fly` sidecar reports the static preview camera, so the image is not accepted as fly-angle proof; no placement fix is claimed.
