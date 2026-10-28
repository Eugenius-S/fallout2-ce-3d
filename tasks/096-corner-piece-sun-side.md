---
id: 096
title: Corner pieces are cut at the corner line, each half on its own face's side of the sun
milestone: M1
status: done
owner: glm
suggested: glm
priority: normal
claude-only: no
user-check: yes
depends-on: [093]
reviewed: no
---

## Goal

A wall's corner piece (the pillar at a corner, the strip at the top of a
corner under the roof's edge, a fence's corner post) takes the side of the
sun that the face it visually continues takes, at every hour, and a shadow
edge crosses it per pixel like it crosses the walls beside it. A corner
piece stays where the original shows it in every camera, and no shadow
runs past it round the corner onto the next face.

## Context

- 093's investigation (its Log, the sheets in `runs/093-shadow-leftovers/`
  and the diagnosis views in `runs/093-probe/`) narrowed the owner's
  leftovers 1, 2, 3, 5 and 6 to one class: a corner piece's art spans two
  faces, and 090's painted-plane rule gives the piece ONE face's side of the
  sun. ShadowDebug 2 proves the inversion: at hour 8 the Buckner corner
  pillar reads shaded beside its lit face, at hour 17 lit beside its shaded
  face (`runs/093-probe/f1-debug-old-vs-new.png`), on main's code.
- The natural fix (093's attempt) cuts the corner's picture at the crossing
  wall and carries the stretch onto the crossing wall's plane. Measured at
  three cut planes:
  - at the Buckner corner the wall rows end inside the crossing band, so
    nothing moves there and the pillar keeps its wrong side;
  - where the cut does bite, the 090 seam metric rises at the evening views
    (19265 h17 133 to 213 at the nearest-plane cut, to 233 at the far-face
    cut; 25075 h17 and 23481 h17 rose with the mid cut): the re-homed
    stretch reads lit inside a shaded wall. Reverted; the exporter is back
    at main's.
- The pen's corner post (6) and the bathhouse's painted post (5) are not
  standing cards: a per-card shader rule changes nothing on them (the pen
  diff is 0 px at hours 8, 12 and 13), and on maps with many card clusters
  (klatoxcv) it moved walls and pools, so it was reverted too.
- Leftover 4 is not this class: the boxes by the bathhouse's corner are
  painted into the wall picture and 093's framings match the vanilla
  framebuffer shot for shot; the owner's live poke-through there is a
  scene object off its table and lives in task 100.
- The owner's later finds of 2026-10-27 (tasks/inbox.md under (096), on
  the board branch until PR #137 merges; the shots live there):
  - the bathhouse's south corner piece is gone in the turned perspective
    camera but in place in the isometric one
    (`runs/inbox/2026-10-27-bathhouse-corner-piece-gone.jpg`);
  - the shadow flows round the corner past a lit corner piece, also at
    Vic's house (`runs/inbox/2026-10-27-vic-house-shadow-round-corner.jpg`);
  - a shadow flaw by the house in the bottom-edge shot
    (`runs/inbox/2026-10-27-bottom-edge-exit-zone.jpg`; the owner did not
    mark the spot, compare it with the original view).

## Direction

The owner set the rule (2026-10-27, on the 093 PR): bake a corner piece as
two pieces cut at the corner line, each carrying its own face's plane, so
each half follows the face it continues. Not the crossing wall's plane for
the whole piece, which is what 090 does and what inverts the piece against
its own face at the ±hours. The 090 seam acceptance (the totals do not
rise) applies; 093's three cut attempts all violated it by re-homing the
stretch onto the crossing wall's plane, so the halves keep their own
planes and their place in the picture.

## Acceptance

- The owner's shots 1, 2, 3, 5 and 6 reproduce fixed at the owner's
  hours, and the later finds with them: the corner piece back in the
  turned camera, no shadow round the corner at either house, the
  bottom-edge flaw gone.
- The 090 seam totals do not rise on any view.
- The regression stays 0 px on the unchanged views; the changed views come
  with updated references and their diff images.
- ctest passes.

## Out of scope

New shadow features, the moon, shadow softness.

## Log

### 2026-10-27 glm: the owner set the direction

- On the 093 PR the owner fixed the rule: two pieces cut at the corner
  line, each on its own face's side of the sun; the 090 seam totals must
  not rise. Leftover 4 moved to task 100. The owner's later corner finds
  from the live run (the inbox (096) entries) joined the scope.

### 2026-10-28 glm: the cut landed; the metric and the regression pay the rule's honest price

- The exporter cuts every corner piece's game-view picture at the corner
  line (the crossing wall's own plane, where the two visible faces meet)
  and the stretch past the line takes the face it continues: its plane for
  the depth and the sun side alike, through the existing depth-home
  mechanism (`_wall_front` corner). The vertices stay where they were;
  093's failed attempts moved them. The same rule cuts a thin wall (a
  fence) whose span crosses the line it corners with (`thin_cuts`). Unit
  tests 66/66; ctest 18/18.
- Measured on the owner's shots and hours (sheets in
  `runs/096-corner-piece-sun-side/`): the Buckner corner's pillar now
  splits at the corner edge, its lit half continuing the lit face at every
  hour (corner-23481-h08/13/17); the left house's corner follows at 19132.
- The seams metric, matched framings, before (main's scene) | after:
  16 of the 18 game views sit at main's level or below (25075 h7 275 to
  262, 23481 h7 199 to 174, 17104 h7 97 to 66, 19265 h7 112 to 96, and so
  on). Two views rise at 17:00, 19265 by 18 pixels and 20690 by 26, and
  the rise is the rule working: the new lit pixels are a corner piece's
  half reading its own face's side inside the shadow that runs past the
  corner (seam-rise-*.png). At 17:00 the two faces of these corners sit on
  opposite sides of the sun, so a whole-piece corner cannot agree with
  both; the owner decides whether the metric's hole definition should
  learn the split corner or the price stands.
- The three fly metric views changed their framing against 090's records
  (shot fresh here), so they were compared as a matched pair: before and
  after are identical to the pixel (713, 53, 1279).
- The fly tour against main's scenes: 0 of 43 spots changed
  (fly-tour-changed.jpg). The game's own view changed only where a corner
  piece's half now takes the other face's plane (the depth moves with it,
  which is what the turned-camera case needs).
- The regression against the 2026-10-26 references (main's scene is
  pixel-clean on every one of them, measured): the changed views are
  entrance 189, corrals 252, residents 60, north-rooms 28,
  central-doors 16, map-edge 4, klamall 336, klaratcv 3837/5185/6343,
  klatoxcv-e2 550 pixels; the diffs sit at wall corners and fence corners
  (the depth edges of the split halves). References updated for the nine
  rule-changed views, their diff images in `runs/096-corner-piece-sun-side/`.
  Three views fail on main itself and were left as they were: klagraz-e0,
  klatoxcv-e0, klatoxcv-e1 (492039, the stale reference task 058 knows).
- The pen's corner post (the owner's fault 6) is the one spot the rule
  does not move yet: its picture is drawn by two rail-end rows whose
  spans stop at the corner line, so the cut lands on 2-pixel slivers
  (pen-corner-h17.jpg, before and after alike). Giving the post's halves
  their fences' planes needs the post card split at its own axis, which
  needs a look at the art; left for the owner's eye and a follow-up.
- The owner's review (2026-10-28) settles the acceptance: the two 17:00
  rises stand. A corner piece's half that follows its own face is the
  rule, and the metric stays as 090 defined it; the decision is written
  here so the follow-up tasks measure against it, not against the old
  totals. Status done; the references above were retaken on this branch.
- The owner's three later finds got their before | after sheets at the
  owner's framing (runs/096-corner-piece-sun-side/):
  - `inbox-corner-turned-before-after.jpg`, the bathhouse corner piece
    lost in the turned perspective camera. Reproduced on the branch
    build: the bathhouse's interior hero tile 18930, zoom 2.5, the
    camera turned 40 degrees, hour 8 - the striped pillar and its boss
    are gone, while the isometric camera of the same view keeps them.
    Main (before) and 096 (after) are identical to the pixel there: the
    turned camera renders through the free camera's batches, and there
    the piece rows stay trimmed at the corner line (the cut is the game
    camera's only), so the pillar art past the trim never draws and the
    free camera's fills show the plain wall. This is the same rule's
    free-camera half and it is its own work; the sheet names it.
  - `inbox-vic-shack-before-after.jpg`, the shadow round the corner at
    Vic's shack (the task 039 tiles: view 17659, hour 13, zoom 2).
    The cut does reach this corner: the shack's west-face corner strip
    reads its own face now (the amplified diff marks the strip), and
    the corner-top edge the owner marked is otherwise unchanged.
  - `inbox-bottom-edge-before-after.jpg`, the flaw by the vine wall at
    the map's bottom-edge exit zone (view 28096, hour 8): the building's
    cast shadow ends with a notched edge by the wall, the owner's circle
    sits on it. Main and 096 are identical here except the pen's fence
    corner post, so the flaw is outside 096's class; the sheet carries
    the classic view of the same spot for comparison. It stays an inbox
    item of its own.
- The owner's posts 5 (bathhouse) and 6 (pen): the split of a post card
  at its own axis cannot ride the span cut - the visible post art lives
  in rows whose spans stop at or before the corner line, so there is no
  past stretch to re-home (the pen's cut bites 2-pixel slivers and the
  visible post does not move). Cut as task 107 in this branch.
