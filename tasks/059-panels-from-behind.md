---
id: 059
title: Panels and some boxes vanish from behind in turned views
milestone: M1
status: done
owner: none
suggested: claude
claude-only: no
user-check: no
depends-on: [038]
reviewed: yes
---

## Goal

A thing drawn on its wall plane (a wall sign, a poster, a locker) shows its
mirrored back from behind instead of nothing; the same for the one box that
vanished (bolcas2).

## Context

- Found on 2026-10-23 in task 039's whole-Klamath pass.
- Evidence: runs/039-shapes-klamath/girlp005.frm.png (the poster shows at 0
  and 90, at 180 the full frame has nothing where it should hang),
  buckas1.frm.png, bilchsig.frm.png, anmste14.frm.png, bolcas2.frm.png.
- No face culling is left on for object cards: scene_draw.cc disables it
  right after the wall batches, so the cause is elsewhere (the plane
  projection in live_objects.cc onFlyPlane, a depth fight with the floor, or
  the exporter's baked mode).
- Panels are 4% of Klamath's static things by count; the fix lifts them all
  at once, better than per-FRM exceptions.

## Acceptance

- A plane-locked thing shows its mirrored back from behind in a turned view.
- The shapes-klamath score's panel failures clear.

## Log

### 2026-10-25 claude
Folded into 075: closed here, the work goes on there.
