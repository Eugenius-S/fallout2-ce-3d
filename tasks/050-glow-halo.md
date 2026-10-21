---
id: 050
title: Slime, fire and lights get a visible glow halo
milestone: M1
status: todo
owner: none
suggested: glm
claude-only: no
user-check: yes
depends-on: [022]
reviewed: no
---

## Goal

Glowing things bloom so it shows: a soft halo that spills over their
surroundings, as in the owner's example
(`runs/050-glow-halo/owner-wanted-look.webp`). Today (022) the slime keeps the
classic look with almost no halo (`owner-slime-now.png`, `owner-cave-now.png`).

## Context

- The glow pass is 022's (`ce/src/scene_renderer/post.cc`, the emission in
  `scene_shader.cc`): the marks (alpha 252 and 254) and the bright pass feed
  an emissive buffer that is blurred at a quarter size and added back.
- The halo needs more spread and strength: a wider Gaussian (more rounds or
  an eighth-size level added to the quarter one), a strength that shows at
  the default `glow_strength` 1.0, and a light falling on the floor and walls
  next to a glowing thing (the halo added over them is enough).
- Fire and the engine's light sources (020) should glow as well: barrels with
  fire, campfires, lamps.
- Screens and monitors: the owner's Vault screenshot is a map that has no 3D
  export yet (only Klamath has one), so it showed the classic 2D. Klamath's
  screens are all in the toxic caves: `klatoxcv` elevation 1, hexes 16449
  (bvs30, monitors) and 19246 (bvs44, a computer); elevation 2, hexes 13696,
  13700, 13702 (glcomp02, computers), 14493 (bvs32, a monitor), 14691 and
  14695 (bvs35, bvs47, computers), 14872 (robcomp2, the robot receptacle).
- Those Klamath screens are dead in the art itself (no animated screen
  colours; `owner-klatoxcv-dead-monitor.png`), so they cannot show a screen's
  glow. Export one more map with working screens close to its entrance (the
  owner's `owner-vault-screens-2d.png` is a Vault: find which map and use it,
  or another one with lit monitors), add it as a group in `export.py`, and
  check the screens' glow there.
- The daylit town must stay clean: no haze over sand and bright walls.

## Acceptance

- Snapshots with the glow on and off in the toxic cave, by a burning barrel
  at night and in the daylit town (`runs/050-glow-halo/`); the owner judges
  the look (user-check).
- Snapshots of working screens with the glow on and off on that extra map.
- With the glow off the regression passes as it is.

## Log
