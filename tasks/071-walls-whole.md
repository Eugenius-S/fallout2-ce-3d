---
id: 071
title: Walls: no holes, a back side everywhere, clean corners
milestone: M1
status: doing
owner: claude
suggested: claude
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

One general fix for the walls in 3D: no holes, every wall seen from behind
shows its back, and house and fence corners close cleanly. No spot patches.

## Context

From the owner (2026-10-22 and the live review of 2026-10-24):

- Some walls lost their back side: from behind in the fly camera they
  vanish. They remember every wall had one.
- Holes in walls in the 3D view. The shack's holes by the barrels may be
  fine (it is a shack), the rest are not.
- Defects at house corners; they want one universal fix.
- The fence corner at the pen is broken, and the shadows show it.

Look for a lost back face or a culling change since 034 first; 059 (panels
from behind) may share the cause.

## Acceptance

- Turned and from-behind shots of the long building, the shack and the pen
  fence under `runs/071-walls-whole/`.
- The regression stays where it is; the game's own view is untouched.

## Out of scope

Doors (072).

## Log
