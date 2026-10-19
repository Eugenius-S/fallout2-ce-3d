# Task 010: a scene for any map elevation

`py -3 tools/scene_export/export.py --maps klamath` built all 12 Klamath map
elevations in 67 s. Each tour is one game process (`tools/dev-snapshot.ps1
-Still -Tour ...`) that travels map to map; every JSON names the map and
elevation the snapshot shows.

- `tour/`: KLADWTWN, KLATRAP, KLARATCV e1 in 3D; KLAMALL e1 cannot be reached
  (an empty elevation with no free hex), the snapshot shows KLAMALL e0 in 3D;
  ARCAVES has no scene and stays 2D (`vanilla-framebuffer`); back on
  KLADWTWN (loaded from its saved state, `KLADWTWN.SAV`) the scene is 3D again.
- `tour2/`: KLACANYN, KLAGRAZ, KLAMALL e0, KLARATCV e0 and e2, KLATOXCV e0, e1
  and e2 in 3D; ARCAVES in 2D, with `scene.log` lines naming the reason.

Seen in passing, for task 011: KLATRAP shows a dark empty area where its floor
has no tiles; in KLARATCV e1 the hero is hard to find among the cave walls.
