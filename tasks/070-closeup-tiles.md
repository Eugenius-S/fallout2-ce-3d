---
id: 070
title: Tiles up close in the fly camera stop looking like pixel noise
milestone: M1
status: done
owner: glm
suggested: glm
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

Up close in the fly camera the floor tiles, the walls and the things read as
clean pictures again, not raw pixel noise.

## Context

- The owner's find of 2026-10-22: `runs/inbox/2026-10-22-fly-closeup-tiles.png`
  (a round tank with its cap, the carpet floor, the wall beside it). They
  remember these were made nearly clean earlier.
- First find out whether an earlier fix (texture filtering, mips, upscaled
  tiles or shapes) was lost, is off in the owner's config, or never covered
  this case; `git log -S` on the filtering calls is the quick way.

## Acceptance

- The same close-up before and after under `runs/070-closeup-tiles/`.
- The game's own view stays pixel for pixel (the regression).

## Out of scope

New art.

## Log

## Log

- 2026-10-27, glm: done, PR awaits review.
  - **Diagnosis.** No earlier fix was lost: `git log -S` over the filtering
    calls shows NEAREST everywhere since the first scene draw (the deliberate
    pixel look).  The tile bake is lossless at native density (TILE_RES 88),
    so at the game's own distance a texel lands about on every pixel; the
    fly camera can put a surface across the screen many times over, and the
    magnified NEAREST texels read as raw pixel noise (the inbox tank, the
    carpet and the wall included).
  - **Fix.** One sampler object (MIN NEAREST, MAG LINEAR, clamp), created
    lazily and bound to texture unit 0 only while the fly camera draws
    (ce/src/scene_renderer/scene_draw.cc; glGenSamplers/glSamplerParameteri/
    glBindSampler added to the loader in gl_procs.h).  Unit 0 is the only
    unit that carries art (floor, walls, things, hero, roofs); the shadow
    map (unit 5), the egg (1), the see-through mask (2) and the lighting
    textures (3, 4) keep their own sampling.  The game's own view never
    binds the sampler and stays pixel for pixel.
  - **Evidence** (runs/070-closeup-tiles/): the same fly camera before and
    after at kladwtwn tile 25076 (turn 25, pitch 14, distance 8), plus a
    closer after shot at distance 2.5; the sheet holds the pair.
  - Checks: ctest 18/18; regression corrals 0 changed pixels.
