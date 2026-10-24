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
