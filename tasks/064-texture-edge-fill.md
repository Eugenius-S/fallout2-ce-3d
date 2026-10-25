---
id: 064
title: Faces with no real art fill from the sprite's edge texel, not holes
milestone: M1
status: doing
owner: glm
suggested: glm (the owner's proposal, 2026-10-24)
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

A solid's face that has no real art of its own never shows a hole or a wrong
stripe: it fills by repeating the sprite's nearest edge texel, the way the
atlas already extrudes edge pixels for seams.

## Context

- The owner's proposal: when an object's geometry is whole but the texture
  does not cover it, take the texture's border pixel and repeat it over the
  gap. Agreed with two bounds (the owner's word settles them):
  1. It is a safety net for FACES WITHOUT ART, not a fix for wrong geometry.
     The by-name box slivers of 040 (a 0.4-unit crate box sampling a
     transparent UV sliver) have broken geometry first; a border fill there
     would paint a tiny block, not a crate. The fitter fix in 040 comes
     first; this task then guarantees that whatever geometry stands, its
     artless faces still show surface.
  2. It applies to the faces the game's camera never sees (fly-only backs,
     ends, undersides: the drum's far halves, box far sides). A prominent
     face smeared with streaks would read worse than the careful mapping
     there today; those keep their real art.
- Where it bites today (the eval's failing looks): barrel4's "striped ghost
  backs", sstove2 from behind at 12% of the game view (040's and 044's
  notes), the far halves of drums 033 added. The exporter picks some region
  of the sprite for such faces; where that region is transparent or wrong
  the fragments discard (holes) or stripe.
- Likely shape: in the exporter (tools/scene_export/solids.py /
  wall_sidecar.py), a face with no mapped art takes the sprite's cell border
  as its UV (clamp): nearest sampling then repeats the edge texel across the
  face; the cell's extruded border in the atlas (EXTRUDE = 4) already
  carries the edge colour outward, so a clamp beyond the cell is already
  half-built. The engine may need nothing at all; verify that first.

## Acceptance

- barrel4 from behind and sstove2 from behind show surface, not stripes or
  holes, in runs/056-texture-edge-fill/ before | after (the eval's turned
  views are the fastest check: eval.py check --only).
- The 16-view regression unchanged (the game camera never sees these faces).
- The exporter tests still pass; a new one covers the fill choice.

## Out of scope

- The fitter fix for the collapsed by-name boxes (040's current step; this
  task's bound 1).
- Any change to what the game's camera sees (backs stay fly-only).

## Log

### 2026-10-24 glm
Done: the task from the owner's proposal, claimed after saying the two
bounds above (his word settles them; the message thread of 2026-10-24).
Next: read the far-half UV choice in solids.py (drums 033, box far sides),
pick where the clamp belongs, verify the atlas extrusion makes it a
UV-window change only.
Check: none yet.
