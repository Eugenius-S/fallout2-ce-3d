---
id: 214
title: Military base walls show light stripes in the glow
milestone: M1
status: open
owner: gpt
suggested: owner
priority: normal
tier: 3
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

The owner, 2026-11-07 (a screenshot of the military base cave interior):
"the light plays up, there are little stripes on the walls" — vertical
light stripes run along the cave walls near the vats in the 3D view with
the glow on. Fix the wall lighting so the glow reads smooth, as part of the
next 3D pass over maps.

## Context

- The owner marked the spot with a red ellipse on the wall to the right of
  the vats; stripes are vertical and follow the wall panels.
- Suspects from earlier wall work: per-panel lighting seams on wall cards
  (the wall pass lights each card separately), or the glow pass sampling
  the wall strip edges. Compare a matching capture with glow off to split
  wall lighting from the glow pass.

## Acceptance

- The same wall view shows no stripe pattern; the fix is a general wall
  lighting rule, not a per-map case.

## Log

- 2026-11-07: owner report with a screenshot; queued for the next 3D pass.
- 2026-11-07, the pass: diagnosed the same day. The vat-area walls captured
  with glow on at several view tiles, brightened and compared with the
  classic 2D of the same spot: the vertical streaks exist in the classic
  render — they are the wall art's own striations, which the 3D view
  reproduces faithfully under the green glow. A wall small-light
  bilinear-sampling hypothesis produced zero pixel change here (the vat
  glow does not sit in the per-hex small lights) and was dropped. Verdict:
  no 3D defect; task closed as by-art unless the owner frames an exact
  spot with the new Ctrl+F12 mark that shows flat brightness steps the
  classic view does not have.
