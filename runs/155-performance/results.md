# Task 155: before and after

Klamath downtown (kladwtwn), the owner's settings (sun shadows, glow, camera
rotation, 3D scale 2), 2310x1298, RTX 5070. Measured with the dev profiler
(`F2CE_PERF`, see docs/p1-devtools.md). "Before" is the same build with
`F2CE_SHADOW_NO_CACHE=1 F2CE_SHADOW_NO_SKIP=1`, which restores the old
per-frame behaviour. The game holds 60 fps, so GPU time per frame is the
number that shows the gained headroom.

| Measure | Before | After |
|---|---|---|
| Average frame time | 20.5 ms (48 fps) | 17.5 ms (56 fps, 60 is the cap) |
| GPU time per frame | 20.6 ms | 11.8 ms |
| CPU time to submit a frame | 12.2 ms | 4.5 ms |
| Sun shadow map pass | 4.2 ms | 0.0 ms |
| Ground with shadows | 9.1 ms | 6.6 ms |
| Flat objects with shadows | 2.5 ms | 1.2 ms |
| Objects with shadows | 1.9 ms | 0.9 ms |
| 1% low frame time (whole run) | 30 to 34 ms | 31 to 40 ms |
| Map load (scene build) | 320 to 410 ms | 300 to 330 ms |
| First switch of sun shadows on, worst frame | 60 to 190 ms | 48 to 51 ms (two 16 ms steps) |

The 1% low is set by short stalls of the game loop outside the renderer
(object updates, 6 to 9 ms per tick), not by the GPU; this task did not change
them.

## Fixed causes

1. The sun shadow map (8192x8192, about 770 MB of textures) was drawn and
   gap-filled again every frame. It is now kept until a caster changes or the
   sun turns by more than 0.02 degrees. Saves 4.2 ms GPU per frame.
2. Every ground, flat object and object pixel walked 25 shadow taps (the
   ground four more reads each) even where no caster is near. A coarse
   occupancy map of the shadow map (one texel per 16x16) lets a pixel skip
   the loop when its tap area is empty. The result is exact: the picture is
   identical to the full loop (checked by a still capture of the same view,
   0 differing pixels). Saves 4.3 ms GPU per frame.
3. The shadow textures were allocated all in one frame on the first switch of
   shadows on. They are now made one per frame, and at scene load when
   shadows are already on, so nothing is built mid-game.

## Checks

The fly tour against main (same scene caches on both sides) changed 0 of 43
spots (`changed.jpg`). The pixel regression changed 0 pixels on every map.
