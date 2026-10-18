# One-map scene export

This adapter exports only `kladwtwn`, elevation `0`, to `game/mod-cache/scene/kladwtwn/0/`. It reads `[system]` paths from `game/fallout2.cfg`, resolving relative paths from the supplied game root. Source order follows CE `gameDbInit` (highest priority first): `master_patches`, `critter_patches`, active mods, the contiguous `PatchFile` chain, `ce.dat`, configured `f2_res_dat`, `critter_dat`, then `master_dat`. The current project config points both patch directories to `data`, so loose `game/data` files override DAT members. DAT files are opened read-only and are never extracted. If active `mods_order.txt` entries are present, the exporter refuses with a diagnostic because this one-map adapter does not model them; it also refuses to guess mod order when `mods/*.dat` exists without an order file.

From the repository root:

```powershell
python tools/scene_export/export.py --project-root E:\Projects\fallout2-sprite-lighting
```

Or provide the concrete game directory:

```powershell
python tools/scene_export/export.py --game-root E:\Projects\fallout2-sprite-lighting\game
```

Requires Python 3, NumPy 2 or newer, and Pillow 10.1 or newer. Run the source-priority regression with `python -m unittest discover -s tools/scene_export -p test_export.py`.

`floor.bin` is a versioned binary sidecar for CE: magic `F2FL`, version 1, 100×100 floor indices, tile-atlas dimensions, 88-pixel drawable cell size, 6-pixel margin, then the outer cell origins from `tiles.cells`. `walls.bin` is a separate versioned sidecar (`F2WL`, version 1) containing atlas-page batches of ready-to-draw triangles. Each vertex stores GL position `x,y,z` and normalized top-down atlas UV `u,v` as five little-endian floats. Its header records byte size, atlas page count, batch count, and total triangle count; each batch records the atlas page index, page width/height, and triangle count before its vertices. Exporter geometry ports the donor's `upright`, `wallFront`, `wallRest`, trim, thin-sheet and static-door surfaces, including projected top caps and back thickness. It includes all 1,259 source wall rows, including the five `hiddenWalls`, because this preview does not yet draw their donor replacement forms. Collider `boxes` are not exported as geometry. `projection.bin` (`F2PJ`, version 1) stores ten little-endian float64 values: `squ[2]`, `sqv[2]`, `origin[2]`, `hpx`, `bbPx`, `rowTilt`, and `stack`. CE uses these exact exported projection values to transform the live hero tile and pixel movement into the same scene coordinates as the floor and walls. `provenance.json` records all sidecar versions and hashes, wall triangle/page counts, archive hashes, resources actually read with their winning source, hashes of copied donor Python modules and adapter files, output hashes, and a deterministic input fingerprint. It contains no timestamp.

With CE built using `--dev-tools`, the static wall preview can include the current player sprite using `--dev-hero-preview`; it requires `projection.bin`, reads the current `gDude` frame and palette, and writes `devhero-...png` snapshots with tile/FID/frame/rotation and world-anchor metadata. Hero mode keeps the accepted wall-preview camera fixed. The scene is view-only: it does not provide picking or game UI, and it does not yet reproduce full row-plane ordering. If projection or frame data is invalid, CE does not mark the snapshot as a successful hero capture.

## Scope and limits

The exporter is an adapter around the donor's offline `SceneBuilder`; it does not invoke the donor's game discovery or extraction flow. The result confirms that the selected game data could be parsed into a scene JSON and its PNG dependencies. It does not confirm visual correctness, CE rendering, runtime compatibility in the app, collision or lighting behavior, or performance. No game executable is launched.

## Donor code and license

The `vendor/f2lib` and `vendor/fp` files are a modified subset of `fallout2-first-person`, revision `0da1cd449f1dc7f2f6986ff7e8728a4b6ae20584`. The donor is licensed under Sustainable Use License 1.0; it is not described here as OSI-approved open source. Original notices and license texts are retained in this directory. This package adds an adapter around the copied donor modules: it checks configured CE sources in priority order, fixes the export to one map/elevation, and writes provenance. The copied donor module files remain verbatim, and the donor repository is untouched.
