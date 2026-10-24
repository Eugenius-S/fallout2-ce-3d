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
