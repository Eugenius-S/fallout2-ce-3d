# 070: tiles up close in the fly camera

The owner's find (`runs/inbox/2026-10-22-fly-closeup-tiles.png`, a round tank
beside a wall on carpet) diagnosed: every art texture samples NEAREST (the
deliberate pixel look; `git log -S` shows no earlier filtering fix that could
have been lost).  At the game's own distance the texels land about one per
pixel, but the fly camera can put a surface across the screen many times
over, and the magnified NEAREST texels read as raw pixel noise.

The fix (ce/src/scene_renderer/scene_draw.cc, gl_procs.h): one sampler object
with MIN NEAREST and MAG LINEAR, created lazily and bound to texture unit 0
(the only unit that carries art: floor, walls, things, hero, roofs) just
while the fly camera draws; unbound when the draw ends.  The game's own view
never binds it, so its picture stays pixel for pixel.

| File | What it shows |
| --- | --- |
| before-fly-dist8.png | The fly camera at kladwtwn tile 25076, turn 25, pitch 14, distance 8, on the pre-fix build: fence, ground and brahmin read as blocky noise. |
| after-fly-dist8.png | The same camera on the fixed build: the same scene reads as pictures. |
| after-fly-dist2.5.png | Closer still (distance 2.5): the hero, the brick wall and the cobblestones stay readable. |
| 070-sheet.jpg | The before | after pair. |

Checks: ctest 18/18; regression corrals 0 changed pixels (the game view does
not bind the sampler).
