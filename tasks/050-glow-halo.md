---
id: 050
title: Slime, fire and lights get a visible glow halo
milestone: M1
status: doing
owner: glm
suggested: gpt
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

### 2026-10-22 glm
Done: the halo (post.cc): the bleed now has two scales, the quarter keeps a
single tight round for the near glow, and a new eighth-size chain carries the
wide spill (threshold + two blur rounds); the composite adds both, 0.55 and
1.5 times glow_strength, so the default 1.0 clearly shows. The wide chain
feeds only from the marked texels: the scene shader writes the mark into the
emissive buffer's alpha (scene_shader.cc), and a new marks pass multiplies by
it, so the daylit town cannot haze (checked: the town pair reads equal, mean
lift 0.06/255). Fire and lights glow through the marks they already carry:
the mall's fire barrel, the town campfire, the vault torches; lamps light
their pools as engine lights (020) and bleed through the near chain. New
export group vault15 (export.py): its screens carry live cycling colours
(monitr7 316 cycling px, monitr1/3/8, comp9, bvs44/45) unlike Klamath's dead
ones; screens on/off in runs/050-glow-halo/vault-screens/. Pairs (off/on)
in runs/050-glow-halo/: cave, vault-screens, torch (the mall fire barrel at
night), night-fire (the town campfire at night), town (daylit, clean);
README there.
Next: the owner judges the look (user-check).
Check: build + ctest 12/12 + the exporter and regression unit tests; the
glow-off path is byte-neutral against the reverted build (0 px on the
klatoxcv-e1 spot); see tasks/054 for the pre-existing reference mismatch
that hits the suite with 050's code stashed the same way.
