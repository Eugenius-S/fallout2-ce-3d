# Klamath gallery: 2D | 3D

62 views over the 11 reachable Klamath map elevations, each a picture with the
game's own 2D on the left and the 3D scene on the right, from the same view
hex, still mode. Views: every building (by its roof piece) and a grid over the
floor, at most six per elevation (`tools/gallery_spots.py`); two snapshot
tours, with and without `-Classic`; pairs by `tools/gallery_pairs.py`.

No draw-order problem shows in any view: buildings, furniture, people,
animals, fences, trees and roofs stand and overlap as in 2D.

## Problems

1. **Empty ground is gray, not black.** Squares without floor and everything
   beyond the 100x100 squares show the gray backing plane under the floor and
   the near-black clear colour; 2D shows black. All maps; clearest in
   `klacanyn-e0-15829`, `klacanyn-e0-23909`, `klaratcv-e1-6701`,
   `klaratcv-e1-14621`, `klatrap-e0-15823`.
2. **No map edge.** 2D cuts the view at the map's edge rectangle (Restoration
   Project's edge data) and shows black beyond it; 3D goes on with floor and
   roofs ending in diamond teeth. `klacanyn-e0-15909`, `klacanyn-e0-23869`,
   `kladwtwn-e0-8711`, `kladwtwn-e0-15043`, `klagraz-e0-24373`,
   `klamall-e0-14643`, `klatrap-e0-15823`, `klatrap-e0-20733`. Task 016.
3. **No game light.** Caves are dark in 2D (low ambient, lamps); 3D draws
   them at full brightness. All KLARATCV and KLATOXCV views, e.g.
   `klaratcv-e0-13471`, `klaratcv-e0-13511`. Task 020, first step.
4. **Building corners** (found by the owner in play, KLADWTWN): a strip of
   wall art standing off a corner, and a roof corner not meeting the wall top
   (`runs/012-klamath-scene-correctness/`). Task 012.
5. **Perspective near the screen's edges**: things there sit a few game
   pixels from their 2D place (the vertibird in `klacanyn-e0-15869`). By
   design of the mild perspective, not a fault.
