---
id: 103
title: The see-through halo round the hero works at walls again
milestone: M1
status: todo
owner: none
suggested: gpt
priority: high
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

A wall in front of the hero turns see-through round him in the 3D view, as
018 made it, not only a roof.

## Context

- The owner, 2026-10-27, live with the 095 build: inside the bathhouse by
  its south wall the wall in front of the hero is drawn solid, no cut-out
  (`runs/inbox/2026-10-27-hero-see-through-halo-gone.jpg`); on a roof the
  halo still shows (`runs/inbox/2026-10-27-halo-on-roof.jpg`). So it is lost
  for walls only.
- The owner again, 2026-10-28, live on main `9705aca4`
  (`runs/inbox/2026-10-28-halo-roof-only.png`): the hero stands behind the
  brick building's wall (the red hex), seen from the camera through the
  wall; the roof shows the cut-out, the wall under it stays solid. The
  first try (2026-10-28) put the hero above the wall's top edge in frame,
  where no wall stands in front of him: the spot to test is a hex behind a
  wall's line from the camera, the wall between the camera and the hero.
- 018 built it: `objectSeeThroughAroundDude` (`object.cc`) picks the
  objects, walls.bin names each face's map object. Something since then
  stopped walls from taking it.

## Acceptance

- Name the commit that lost it (git bisect with a dev snapshot of that
  bathhouse spot) and fix the cause.
- before | after of the bathhouse spot and of one more wall, in
  `runs/103-see-through-halo-at-walls/`; the roof case unchanged.
- The original view regression stays pixel for pixel; ctest passes.

## Out of scope

The halo's shape and size.

## Log

### 2026-11-01 glm: the mechanism verifies correct everywhere; the find does not reproduce

Done: an exhaustive reproduction sweep, no code change. The wall cut works on
today's main in every configuration I could reach, so the branch holds the
investigation, not a fix.

- The egg mask (art/intrface/egg.frm, interface 2) is a filled ellipse: the
  interior is index 1, the soft edge 6..122, 0 outside. The shader's wall
  passes (uEggPass 1 discards the whole ellipse, the egg pass redraws the
  mask-alpha edge over the hero) and the card passes (the main pass discards
  the ellipse for see-through cards via uEggPass=1 + uSeeThrough=2, the egg
  pass redraws the edge) both net exactly the 2D behaviour.
- The walls.bin row attribution verified clean: a census of all 1270 used
  rows puts every key's hex ground inside its triangles' ground (the
  earlier "row 0 spans the bathhouse" reading was a behind-the-eye
  projection artifact of my probe, not a real misattribution).
- objectSeeThroughAroundDude matches the upstream fallout2-ce and
  fallout2-re decompilations bit for bit, including the "TODO: Probably
  wrong" first branch (PROTO_EXT_FLAG_HIDDEN | WEST_CORNER).
- The cut verified on: the bathhouse room (9 hero tiles, kladwtwn
  e0), the entrance brick building interior (12 tiles, hx 68-88,
  hy 116-120; the owner's 2026-10-28 shot matches this building), the
  camera centred and scrolled, 4:3/16:9/21:9, zoom 1/1.28/1.37/2.048,
  today's main and 9705aca4 (the owner's build), a fresh cache, the
  owner's own mod-cache copy, and the owner's SAVEGAME SLOT01 state
  (hero 25075, camera 24678: the hero stands visible, nothing covers
  him, 0 changed pixels is correct there).
- Evidence: runs/103-see-through-halo-at-walls/ (the egg-on | egg-off
  pair inside the entrance building at 23676, the owner-save state,
  the bathhouse 17878 view).
- Dev-tool gotcha found on the way: --dev-hero-tile onto an
  out-of-bounds or wall tile (e.g. 18375 on kladwtwn) leaves the hero
  invisible, the roofs unhidden and the egg rect off-screen, in the 2D
  classic too. Sweep fixtures must use walkable tiles.
- gpt's draft (PR #151) re-read: at his spot 19075 the cut works; his
  "marked south-wall section stays solid" (tile 19102, fid 030001BB)
  is the section EAST of the hero, which the tile rule correctly does
  not cut; it does not indicate a broken mechanism.

owner: I could not reproduce the missing wall cutout in any reachable
state. To pin it: the save from the moment of the 2026-10-28 find
(copied into game/data/SAVEGAME, read-only), or the hero tile and the
camera tile of that shot, plus the window and zoom used. With that the
pair runs in one command (dev-snapshot.ps1 -Slot N -Still). If the
find was the wall piece directly in front of a hero standing three or
more rows north of a HIDDEN-flagged wall run, the suspect is the
first branch of objectSeeThroughAroundDude (the "TODO: Probably
wrong" decompilation), which cuts nothing there; vanilla's behaviour
in that case needs the original exe's judgement.

Next: the exact-state repro from the owner, then the fix and the
before | after pair.
Check: py -3 -c (the on/off diff) per the commands in the Log above;
the pairs in runs/103-see-through-halo-at-walls/.
