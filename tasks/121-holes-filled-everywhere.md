---
id: 121
title: No holes anywhere: the edge-colour fill runs over every map and thing
milestone: M1
status: doing
owner: glm
suggested: owner
priority: normal
claude-only: no
user-check: yes
depends-on: [115]
reviewed: no
---

## Goal

The owner, 2026-10-31: work the hole filling through completely while the
reviewer is away. 115 taught props.fill_holes (an enclosed transparent
texel takes the colour of the nearest filled edge pixel) for the exporter's
things; the pass must cover everything: solids and their rests, boards,
wall sheets, forms, every map of the game.

## Context

- 115's fill ran on the solids' textures and the wall cuts; anything the
  pipeline draws by another path (boards' live frames, shadow sprites, rock,
  tiles) was not audited.
- The owner's words for the fill rule: the colour of the last pixel (the
  nearest filled edge texel), which is what 115 shipped.
- The owner's rule is about what the PLAYER sees: the solids draw over their
  cards in the game camera too, so a filled texture plugs the see-through
  gaps there as well - the regression deltas are the fill working, not a
  fault.

## Acceptance

- A survey over all maps lists every art that still shows a see-through hole
  after the fill, and the fill covers them (or the survey shows the hole is a
  real gap the game's own view also shows).
- The 16 regression spots change only where a hole was plugged (the
  references update after the owner's visual pass); no spot shows anything
  but a hole turning into edge colour.
- Fresh before | after pairs for the holes this finds.

## Out of scope

Unique objects' arts (they keep their cards), geometry work (task 120).

## Log

### 2026-10-31 glm: the band pass - the gaps a shape straddles fill; the sweep ran the whole game

The first new hole class: a transparent band with opaque art above and below
in the same column - the gap a hovering lid leaves over its body, the air
between a shelf's boards - which the border-reach rule of 115 kept open but
a 3D face shows as a slit straight through the thing. The rule took two
cuts. "Opaque above and below" alone filled a notch cut into the side of one
piece (the 115 border test caught it): the honest discriminator is the art's
connected pieces - the band fills only when the opaque above and the opaque
below belong to DIFFERENT pieces (a lid over a body), while a bite out of
one piece stays open. props.fill_holes labels the art's 4-connected pieces
and fills the straddled bands between them; two new unit tests (the lid gap
fills, the open notch and the open sky between two posts stay).

The survey over the stood arts: 36 art rotations change, and the before |
after sheet (the visualisation in this session's gallery) shows the fills
read right at scale - the shelves' boards close into a solid face, the tyre
stacks and the vents close, the cactus plugs between its arms; the vault
elevator (velev04/09, +1883 texels) smears its wall texture down the shaft
gap, tolerable at view scale, the owner judges. The whole-game build check
(226 elevations of every map file the game ships): every map builds, two
pre-existing failures on the unused third elevations of epamain1/epamain2
(truncated maps, the exporter's own --maps flow skips them per map).

The regression then showed what the rule means in the player's view: 14 of
the 16 spots change (the entrance spot 24k pixels, the corrals 15k), every
delta a hole turning into edge colour where a stood solid covers a straddled
gap - the shrubs' stems, the cactus arms, the shelf boards. This is the
order working, not a fault; the acceptance line above was corrected to say
so (the original "0 changed pixels" line belonged to 115's enclosed-only
fill). The pairs (reference | this branch) are in
runs/121-holes-filled-everywhere/ for six spots; the references update after
the owner's pass.

Branch note: branched from task/120-scenery-3d-everywhere (that PR carries
the stands this fills; both wait on 119).
