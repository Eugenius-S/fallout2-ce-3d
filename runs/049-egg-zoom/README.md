# 049: the egg's circle follows the zoom

The owner's find: zoomed in, the circle cut out of walls around the hero
stayed the game's size in screen pixels (`owner-egg-not-scaled.png`).

## The fix

The egg's mask (129x98) lives in the game's own pixels. The zoom shows that
picture scaled around the screen center, so the scene now hands the shader a
rect scaled by the shown zoom around the mask's sprite anchor (half across,
one pixel up from the bottom, the point `updateRoofsFromGame` projects). At
zoom 1 the rect is bit-identical to the old one, so the regression passes
untouched; at other zooms the circle keeps the hero's on-screen size.

## Evidence

Buckner house, the hero inside at the south wall
(`KLADWTWN.MAP`, view/hero tile 24285):

- `egg-zoom-0.5.jpg`, `egg-zoom-1.jpg`, `egg-zoom-2.jpg`: full frames.
- `zoom1-cut-crop.png` vs `zoom2-cut-crop.png` (side by side in
  `zoom1-vs-zoom2.png`): the wall's see-through oval around the hero is
  twice as wide and tall at zoom 2, as is the hero.
- `zoom1-cut-crop-egg-off.png`, `zoom2-cut-crop-egg-off.png`: the same views
  with `F2CE_NO_EGG=1`, the circle gone.
- Measured on/off pixel boxes: zoom 1 (832,296)-(1090,480), zoom 2
  (703,150)-(1219,518): exactly twice the size.

## Tooling added

- `--dev-zoom=D` (and `-Zoom D` on `tools/dev-snapshot.ps1`): the zoom of a
  background run (it is pinned to 1 without it).
- The snapshot JSON reports the egg: its rect, the shown zoom, how many
  walls.bin rows the game marked see-through.
- `F2CE_NO_EGG=1` hides the circle for an on/off capture pair.
- `--dev-hero-tile` now zeroes the egg's leftover scroll shift
  (`gEgg->x/y`): a teleport left it stale and displaced the circle by up to
  160 px in the snapshots.

## Notes for the follow-ups

- The engine itself excludes `OBJECT_TRANS_NONE` walls and fences (the
  corral fences, the Buckner house walls) from the egg; the translucent
  circle the 2D game shows on them is the wall-translucency preference, a
  separate feature the 3D scene does not render yet.
- With the circle working, the cave views (klatoxcv-e0, klaratcv-e2) are the
  regression spots where the egg's cut is clearly visible.
