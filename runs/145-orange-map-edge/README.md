# Task 145: orange map edge

## Evidence

- `before.jpg` and `after.jpg` are matched static 3D previews of KLADWTWN.MAP, elevation 0, view tile 24678, at 1920x1080 (logical 960x540, scale 2). Both use the same fixed camera, daylight time 288000, and authored map clip with identical bounds and planes. The orange dithered exit-grid overlay is visible before and absent after; the black exterior and authored cut remain.
- The executable hashes differ as expected: before `7c3c4fb3a04848ded34d9a03745b6a49dc702579d33a4902361a91f922c20dc1`, after `2f9b44503c14ee5edf6a4df7a1cf1c1d93eb4a044c659a6d65550e7615fbe4ed`. Both sidecars report CE commit `79155dfd`.
- `classic-2d.jpg` is the game's vanilla framebuffer at the same map, elevation, tile, and resolution. It is a contextual 2D reference, not a pixel-matched 3D baseline; it reports game time 288005 while the matched 3D pair reports 288000.
- The reference sweep captured 106 fixed spots. 46 showed pixel changes; visual review of all changed highlights places them on translucent exit-grid marker regions. The other 60 had identical pixels and their references were restored. The change keeps exit-grid objects in picking data while suppressing their translucent cards in 3D.

Source captures and sidecars are in the local build tree: `build/task145-3d-24678/`, `build/task145-after/`, and `build/task145-classic-24678/`.

## Fly-tour check

- `tour-klatoxcv/changed.jpg` records a completed control tour: 5 spots across three elevations, no changed views. Those cave views do not contain the visible EDG overlay.
- The complete Kladwtwn tour timed out on the baseline side after 32/58 images at `tour_step=31`. A reduced two-spot route completed its 6 baseline images but the changed branch side timed out after only 2/7 images at 144 seconds. The direct before/after pair and full 106-spot regression are complete; the missing Kladwtwn fly-tour run is logged as a blocker, so PR #293 stays draft.
