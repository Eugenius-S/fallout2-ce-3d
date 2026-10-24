---
id: 057
title: Sun shadow look round 2
milestone: M1
status: doing
owner: glm
suggested: glm
claude-only: no
user-check: yes
depends-on: [045, 046]
reviewed: no
---

## Goal

The owner's live review of the 046 build (2026-10-24; his notes are in
046's Log, his screenshots in
runs/046-characters-in-shadow/review-2026-10-24/) sets the shadow look to
reach:

- A fairly crisp shadow that melts to nothing with distance from its
  blocker (his cube example); what the build shows does not read that way
  yet.
- Edges antialiased: the border still stairs.
- No stripes on house walls from the "shadow pixel size" setting.
- The shadow accounts for the caster's art alpha: a fence casts a solid
  band although its art is full of holes.
- Sprites standing in shade dim less than now.

Also on the way: the T hotkey (the sun hour in the [3d] panel) must skip a
whole hour per press.

## Context

- The look today: sunShadow() takes four taps a uShadowStep apart with xz
  snapped to the step grid, ramps the border with smoothstep over about two
  steps and the depth difference past the blocker over about five units
  (uShadowRange); sprites take the hex pass factor whole (uSpriteShadow).
- The stripes point at the xz snap: the sun is static per hour and the
  lookups are world space, so the snap buys stability only while the hour
  animates.
- The fence casts from baked solid geometry; the caster pass must sample
  the texture alpha and cut the holes out.
- Open questions to check during the runs: where the straight card cutout
  went (his words; which card is unconfirmed), and whether fire glows in
  Klamath with [3d] glow on (022 and 050 are in main; Klamath screens are
  dead art).

## Acceptance

- Before and after dev shots of the corrals and the town at 17:00 and noon
  under runs/057-shadow-look-2/: sharp contact, melt with distance, no
  stairs, no stripes at any shadow_pixel_size, the fence shadow full of
  holes, sprites in shade visibly lighter.
- The regression with shadows off stays at the 054 baseline.
- The owner's eye finishes it (user-check: yes).

## Out of scope

Cloud shadows (023), the lamps (020), wall holes and corners and the
computer stands (the inbox lines of 2026-10-24).

## Log

### 2026-10-24 glm
Claimed straight from the owner's review round; the notes and his
screenshots are in 046's Log and
runs/046-characters-in-shadow/review-2026-10-24/. The non-shadow finds of
the same review went to the inbox.

### 2026-10-24 glm
Done: the shadow look after the owner's live review.
- The lookup no longer snaps to the shadow_pixel_size grid: the snap was the
  stripes on house walls (a whole wall face shared a few lookup cells). The
  setting is now the edge softness (the label is "Shadow edge softness"): an
  antialiasing ring of nine taps (the center plus eight round) spans
  max(setting, one depth map texel), so the edge always antialiases whatever
  the setting says, and the shadow core stays crisp.
- The caster pass draws textured with a cutoff: the art's transparent holes
  (a fence's gaps, a window's glass) cast no shadow. The caster buffer
  carries UVs and one draw range per atlas page (walls.cc, WallCasterRange in
  shadow.h, the pass binds each page's texture).
- Sprites in shade dim to 0.35 of the factor (was 0.5); the ground keeps
  0.5. Characters stay readable in a wall's shadow.
- T steps one game hour (was three); night hours still show no sun.
Check: build Release, ctest 12/12. Shots under runs/057-shadow-look-2/:
the corrals at 17:00 and noon, a fence close-up at zoom 2, and size 1
against the default 4. Vs the 046 shots 3.3% of the sunset frame changed
(the shadow areas); the size change now moves 1.1% of the frame, the edge
softness only, where the snap used to restripe every shadowed wall; shadows
on against off moves 27% (they are live in the runs). The regression with
the pinned config (shadows off) is line for line the morning baseline:
every spot's changed-pixel count identical, klacanyn-e0, klaratcv-e0 and
klatoxcv-e2 at 0.
Next: the owner's eye (user-check: yes). The two feel constants (the edge
softness default 4, the distance fade ~5 units) are one line each.

### 2026-10-24 owner (round 4, shots 20..23)
Overall much better. For the next pass:
- Edges a touch sharper still.
- The pen: the shadow there reads as falling from nothing. An alpha-plane
  wall (the pen fence with holes) must cast exactly its opaque texels: the
  fence's shadow must read as the fence's lattice.
- A figure standing in shade was left out of the dimming there.
- A long building's shadow looks strong at both ends and melted in the
  middle, right where the figure stands; asked whether that is real.

### 2026-10-24 owner (round 5, shot 24)
The wanted shape in his words: along its whole length the shadow stays dense
first, and only then melts to zero. So: a solid core with no weakening right
at the blocker, one melt window at the far end.

### 2026-10-24 glm
Done: round 2, the owner's shape spec (dense along the length, a melt only at
the far end; the density right at the blocker he called good already).
- The fade is a plateau now: full density for the first ~2 GL units past the
  blocker, smoothstep to zero by ~9 (was a melt starting at the blocker over
  5). A wall's contact line stays dark even where the roof overshadows it in
  the depth map, and the figure standing there takes the same hex factor, so
  he darkens with his ground.
- The depth map is 4096 (was 2048): the texel halves, the fence's lattice and
  the edges come out crisper through the same one-texel AA floor.
- The default edge softness drops from 4 to 2 game pixels (sharper borders,
  his round 4).
Check: build Release, ctest 12/12. Round 2 shots under runs/057-shadow-look-2/
(corrals-sunset-r2, corrals-noon-r2, corrals-fence-zoom-r2): 23% of the
sunset frame moved against round 1, the shadows holding density where they
melted before; the shadows-off regression is still line for line the morning
baseline. The game exe in game/ carries the round.
Next: the owner's eye again. If the building's middle still reads light, the
plateau widens by one constant (2 to ~4); the melt end (9) and the default
softness (2) are one line each. The 4096 map costs 32 MB and one caster pass
a frame; the dev perf log can price it if the owner asks.

### 2026-10-24 glm (diagnosis before the fix)
The melted middle is real and it is the distance fade misreading stacked
blockers: for a ground point at the wall base the sun's ray often clears the
wall and hits the ROOF first, so the depth map stores the roof and the fade
measures from the roof, far away, and melts the wall's own contact line; at
the building's ends the rays pass the roof and the wall itself is the
blocker, so the ends stay dark. The figure left out follows the same hex
factor. Plan: keep the shadow whole for the first few units and melt over a
longer reach (smoothstep(3, 10) GL units instead of (0, 5)), halve the depth
map texel (2048 to 4096) and drop the default edge softness from 4 to 2 game
pixels, so the fence's lattice and the edges come out crisp; then re-shoot
the pen.
