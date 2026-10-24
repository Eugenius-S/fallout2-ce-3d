---
id: 046
title: Characters darken in building shadows; painted shadows give way
milestone: M1
status: done
owner: glm
suggested: gpt (bailed 2026-10-23, the owner passed it to glm)
claude-only: no
user-check: yes
depends-on: [045]
reviewed: no
---

## Goal

A character or a sprite standing in a building's shadow darkens as a whole,
and the shadows painted into the art do not double the real ones.

## Context

- One shadow lookup at the sprite's anchor slightly above the ground,
  smoothed over about 150 ms, so crossing a shadow border is a short fade.
  Ambient light and the lamps (020) still light it.
- The exporter already gives the shadow-free sprite of each such board
  (`shadows`: board, sprite without the painted shadow, shadow sprite) and the
  floor shade (`casts`). With shadows on, use the shadow-free sprites and drop
  `casts`; with them off, the originals.

## Acceptance

- Snapshots in `runs/046-characters-in-shadow/`: the hero inside and outside
  a shadow, and a board with a painted shadow, shadows on and off.
- With shadows off the regression passes as it is.

## Out of scope

Cloud shadows (023).

## Log

### 2026-10-23 glm
Done: characters and cards darken in the sun's shadow; painted board shadows
give way while the shadows are on. The exporter writes shadows.bin
(shadow_sidecar.py): the shadow-free board sprite keyed by the map object's
(tile, fid) and its frame and rotation, raw RGBA; scene.json's shadows rows
grew the frame and rotation. The engine loads it next to walls.bin
(board_shadows.cc) and the card pass draws those pixels for the board instead
of its art when [3d] shadows is on, caching them as art variant 1
(scene_objects.cc, live_objects.cc); an animated board keeps its art when its
frame moves. The card's own shadow: the shadow pass grew the hex pass, 200x200
texels over the ground, one per hex, drawn with the scene program in light
mode 5 (the pure shadow test at the ground point, 0.05 above it) and read
back once per rendered frame; each card chases its hex's 0 or 1 over 150 ms
(live_objects.cc, keyed by the pick id, the hero has its own state) and the
shader applies it at the sprite's light mode (uSpriteShadow, modes 3 and 5 in
shadowDarken, the translucent path included). `casts` are not drawn anywhere
today, so with the shadows off nothing dropped and the off view is the old
one. Build Release, ctest 12/12, the exporter suite 48/48.
Check: runs/046-characters-in-shadow/, four shots in the chat: the corrals at
17:00 with shadows on (the fence casts, the posts' painted blobs gone), the
hero on the sunlit sidewalk and by the wall corner in the shade (his card
darkens), the fence posts on and off close up, the entrance wide. The
regression with the pinned config (shadows off): the same picture as the 054
baseline, klacanyn-e0, klaratcv-e0 and klatoxcv-e2 at 0 changed pixels; the
off frames are byte-identical across 12:00 and 17:00, and the entrance right
strip's black area matches the reference (it is old, not ours).
Next: the owner judges the look (user-check: yes). The fade is by
construction (~150 ms, exponential-free linear chase); stills cannot show it,
so walking the hero across a border is the owner's check. A devperf.log did
not reproduce in the snapshot runs this time, so the hex pass's cost is not
measured; it is a 200x200 draw plus a 160 KB readback a frame.
Note for 045's follow-ups (the owner's three notes are in 045's Log): the
strange ground shapes at the corrals are still there (they are the 045
matter), and the pre-existing black strip at the entrance view's right edge
with the shadows off is in the regression reference itself, so it predates
the shadows.

### 2026-10-23 glm
Done: the owner passed 045's three follow-ups to glm, and the shadow border
now fades to full transparency. sunShadow() grew four taps a uShadowStep
across: the shadow's core stays sharp, its border ramps to nothing over about
two steps, and the snap's stairs soften with it; the floor and the walls take
the faded share. The hex pass renders the graded factor, so the cards' fade
targets are graded too (shadowGroundShadowFactor replaces the boolean, and a
hex half in shadow fades its sprites half way).
The pen's big soft shape from the owner's screenshot is the tree crown's own
long shadow: the tree NE of the pen (off frame in the corrals view) casts as
a big baked object, and the low 17:00 sun stretches the crown shadow across
the pen. With the fade it melts into the ground instead of cutting out as a
stain; whether plants should stop casting is the owner's call.
Check: build Release, ctest 12/12; the regression with the pinned config is
byte-identical to the morning run (the same 13 of 16, the same three at 0).
The corrals at 17:00: the fence shadow's far edge melts, the crown shadow's
edge no longer cuts; the shots are in the chat and under
runs/046-characters-in-shadow/ (corrals-sunset-faded, _fade-before-after,
_pen-old-new).

### 2026-10-24 glm
Done: the owner's two notes on the softened shadows. 1) The big soft shape in
the pen had to go entirely. It is not the tree crown: tracing it across hours
and from the fly camera showed it anchored at the building SW of the pen, the
roof's own shadow laid across the yard at full strength however far it
reached. The fix is general: the shadow now fades with the distance the sun's
ray travels past its blocker (the fragment's map depth minus the depth the
blocker wrote, uShadowRange scales it; smoothstep over ~5 GL units), so a
wall keeps its dark contact line, a fence's shadow melts along its length and
a roof's far stain is gone. The PCF four-tap border stays. The plants' no-cast
mark (baked.bin v6, scene.plants, BakedObject.noCast in walls.cc) went in on
the way and stays: live plant cards never were casters, but a plant baked as
a solid no longer casts either; the blob's cause turned out to be the roof,
not the tree. 2) The blob gone with it.
Check: build Release, ctest 12/12, the exporter suite 49/49. The corrals at
17:00 and noon and the entrance at 17:00 with the distance fade:
runs/046-characters-in-shadow/ (corrals-sunset-final, corrals-noon-final,
entrance-sunset-final; corrals-sunset-shadows is the before). The regression
with the pinned config (shadows off) is untouched by the shader path; the
morning byte-identical run covers it.
Next: the owner judges the look (user-check: yes); the fade lengths (border
~2 steps, distance ~5 units) are one constant each, tuned for the pen view.

### 2026-10-24 owner (live game review, round 2)
He is playing the build from this branch and is not done reviewing; recorded
only, nothing started. Eight screenshots in the order he sent them:
runs/046-characters-in-shadow/review-2026-10-24/shot-01..08.png.

Shadows (the 023/045/046 line):
- Shadows grab objects in odd ways: bands and patches land on objects that
  should not be shaded like that.
- No antialiasing on the shadow edges; the border still stairs.
- The T hotkey (the sun hour in the [3d] panel) barely moves or stalls; one
  press must skip a whole hour.
- The "shadow pixel size" setting draws stripes on house walls.
- The look he wants, his words: a fairly crisp shadow that fades to nothing
  with distance from the blocker (his cube example). What the build shows now
  does not read that way yet.
- Characters standing in shade dim too much; ease the sprite dimming down
  a bit.

Not shadows, split into their own tasks once he finishes:
- Holes in walls in the 3D view; close them.
- The computer stands must get real 3D too.
- Defects at house corners; he wants one universal fix for all corners, not
  spot patches.

Next: wait for the rest of his notes before planning any fix.
