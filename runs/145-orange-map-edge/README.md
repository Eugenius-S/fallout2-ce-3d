# Task 145: orange map edge

## Evidence

- `before.jpg` and `after.jpg` are matched static 3D previews of KLADWTWN.MAP, elevation 0, view tile 24678, at 1920x1080 (logical 960x540, scale 2). Both use the same fixed camera, daylight time 288000, and authored map clip with identical bounds and planes. The orange dithered exit-grid overlay is visible before and absent after; the black exterior and authored cut remain.
- The executable hashes differ as expected: before `7c3c4fb3a04848ded34d9a03745b6a49dc702579d33a4902361a91f922c20dc1`, after `2f9b44503c14ee5edf6a4df7a1cf1c1d93eb4a044c659a6d65550e7615fbe4ed`. Both sidecars report CE commit `79155dfd`.
- `classic-2d.jpg` is the game's vanilla framebuffer at the same map, elevation, tile, and resolution. It is a contextual 2D reference, not a pixel-matched 3D baseline; it reports game time 288005 while the matched 3D pair reports 288000.
- The reference sweep captured 106 fixed spots. 46 showed pixel changes; visual review of all changed highlights places them on translucent exit-grid marker regions. The other 60 had identical pixels and their references were restored. The change keeps exit-grid objects in picking data while suppressing their translucent cards in 3D.

Source captures and sidecars are in the local build tree: `build/task145-3d-24678/`, `build/task145-after/`, and `build/task145-classic-24678/`.

## Fly-tour check

- `tour-klatoxcv/changed.jpg` records a completed control tour: 5 spots across three elevations, no changed views. Those cave views do not contain the visible EDG overlay.
- Earlier full and reduced Kladwtwn tours timed out, as recorded in the task Log. The fresh bounded current-main route documented below completed on both sides and supersedes that route blocker.

## Refreshed-main verification, 2026-11-06

- `tour-current-main-edge.jpg` is a bounded live fly-tour comparison after rebasing on `6beaf53f`: Kladwtwn e0 tile 24678 at -45, 0 and +45 degrees on both current main and task head `839484d8`. The orange marker disappears at -45 and 0; +45 is unchanged. Both sides used the same 13-file scene fingerprint (prefix `56c9acd`) and 1,430 resources. This completes the bounded fly check that earlier full routes could not complete.
- `current-time-8/3d/` and `current-time-8/2d/` contain fresh same-tile 3D and classic 2D captures. Both are 1920x1080, tile 24678 and hour 8; the classic run advanced five ticks to 288005 while the still fixture held at 288000. The 2D frame is contextual, while the matched 3D pair remains the pixel-comparable before/after evidence.
- `regression-review-1.jpg` and `regression-review-2.jpg` show before/after pairs for all 46 updated regression references. I reviewed every pair: the removed pixels belong to the translucent exit-grid wedges along map edges; regular scenery remains in place. The other 60 references have no changed pixels.
