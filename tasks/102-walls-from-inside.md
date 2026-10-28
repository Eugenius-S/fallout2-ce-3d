---
id: 102
title: Walls seen from inside are whole and dark, with no holes and no lit window strips
milestone: M1
status: doing
owner: gpt
suggested: gpt
priority: normal
claude-only: no
user-check: yes
depends-on: []
reviewed: no
---

## Goal

From inside a building in flight, its walls are whole, with no gaps to the
outside, and their inner face does not show the outside picture's windows
lit up.

## Context

The owner, 2026-10-27, live with the 095 build, shots in `runs/inbox/`:

- `2026-10-27-wall-holes-still.jpg`: inside a house at eye level the walls
  between the doors have gaps; the owner: the task about holes in walls
  changed nothing here. 071 ("Walls: no holes, a back side everywhere") is
  done, so this is what it left.
- `2026-10-27-bathhouse-inside-light-strips.jpg`: inside the bathhouse,
  roof off, the far wall's inner face shows vertical strips of bright specks
  where the outside picture has windows; the owner reads them as stray strips
  of light.

## Acceptance

- before | after of both views and two more interiors of Klamath at eye
  level, in `runs/102-walls-from-inside/`, with what the inner face of a wall
  shows now and why.
- The 090 seam totals do not rise; the original view regression stays pixel
  for pixel; ctest passes.

## Out of scope

Roofs (101), corner pieces (096), the table's junk (100).

## Log

### 2026-10-28 gpt
Investigation before implementation: exported only `kladwtwn/0` to
`build/scene-cache`. The owner JPEGs have no camera/tile metadata, so their
exact camera tiles cannot be recovered. I matched their wall patterns and
nearby map objects to the source wall families, then reproduced both patterns
in the runtime snapshots below.

The door-wall view matches the `doorb06.frm` room and the neighboring
`nec` wall family: wall rows 225, 253, 264, and 292 are `nec107.frm`,
`nec108.frm`, `nec102.frm`, and `nec079.frm`; rows 253 and 260 have about
51% transparent texels in the sampled back strip (atlas sprites 1843 and
1801). The scene's door object at tile 15272 is `doorb06.frm`. The runtime
view at tile 15272 shows outside-colored pinholes in those dark inner wall
faces. These alpha holes are a plausible cause of the openings in the owner's first view, but the exact source pieces remain unconfirmed.

The bathhouse view matches the two narrow windows on its far wall: rows 200
and 210 are `nec151.frm` and `nec150.frm` windows at tiles 14714 and 14722;
both sample back sprite 2216, whose dark texture has 7.3% transparent texels.
The neighboring wall panels are `nec096.frm`, `nec097.frm`, and `nec098.frm`.
The runtime view near tile 14900 shows the same light leaking through this
inner window pattern. It is the back texture's transparent pixels, not bright
opaque specks in the source, that reveal the outside.

Correction: the earlier rows 553/556/557 and row 308 were unrelated nearby
walls and are not the source pieces for these views. Do not use them as
implementation targets. Next: one general rule that makes wall-back texture
holes opaque and subdued for every wall, then retake the owner views and the
other Klamath interiors.

### 2026-10-28 gpt (safety check) before implementation

Checked the exporter’s source geometry classification, independent of English
prototype names. `boxes.Cut.kind` is derived from the un-sheared front image;
`Cut.solid` records its actual solid runs. Bathhouse rows 200/210 are `window`
with a full-width run `(0, 32)`, so making only their back-texture transparent
texels opaque and dark is consistent with a closed wall panel. Row 225 is
`plain`, also with a full-width run. The first-view candidates rows 253 and 260
are `other`, with only run `(10, 16)` across 16 columns. Their front silhouettes
therefore have a partial solid run. This is evidence of an opening in the
source silhouette, not proof that the space is an intentional passage. The
back sprites are about 51% transparent, but filling their transparent texels
could close that opening.

Reproduced the source scene with `py -3 tools/scene_export/export.py
--game-root E:\Projects\fallout2-ce-3d\game --map kladwtwn --elevation 0
--cache-root .\build\scene-cache`; source classification is in
`tools/scene_export/vendor/fp/boxes.py` (`Cut.sort` and `Cut.body`). The scene
rows above are from `build/scene-cache/kladwtwn/0/scene.json`; inspection of
`SceneBuilder.cuts` confirmed their `kind` and `solid` values. No owner EXE was
started or focused.

A general filter that only fills `plain`/`window` wall backs can handle the
bathhouse view while preserving open geometry, but it leaves the first-view
candidate unchanged. The owner JPEGs have no camera metadata, and this
investigation cannot distinguish an intentional pass-through from the reported
wall leak for those `other` pieces. The name-based prototype exception is not
safe: the candidate wall pieces share generic `Wall` names, and the geometric
classifier says their front shape is open. No implementation was retained;
the task remains unresolved until a reliable source-level distinction or
camera/source match for the first view is available.

### 2026-10-28 gpt: general mirrored-alpha implementation and verification

Implemented one rule for every thick wall's fly-only back: use the front
sprite's alpha in the same wall-local coordinates (the back geometry already
reverses UVs), preserve openings, and replace only pixels that become opaque
on the back with dark RGB `(24,24,24)`. The front atlas and game-only batches
are unchanged. The exporter packs generated back images into additional
`spritesN.png` pages and references those pages only from fly-only wall
batches. The unit test checks alpha and RGB behavior and maps a point at a wall
endpoint through front and back UVs to verify world-coordinate coverage.

Checks:

- `py -3 -m unittest discover -s tools/scene_export -p test_export.py`: 67/67
  passed.
- `ctest --test-dir build --output-on-failure`: 18/18 passed.
- Exported all seven Klamath maps (12 map/elevation scenes) and ran exporter
  `--check`, using only `build/scene-cache`. Kladwtwn has `sprites0.png` plus
  generated `sprites1.png`; its runtime snapshot loaded in `scene-gl` with
  29,459 wall triangles. The loader cap is 16 pages; each Klamath scene stays
  below it. `wall_pages=4` in the snapshot is the batch count, not atlas-page
  count.
- Pixel comparison, same branch executable and capture settings, current main
  `game/mod-cache/scene` vs this branch's `build/scene-cache`: all 16 original
  game-view regression spots had 0 changed pixels. The repository's older
  pinned-reference script separately reported six mismatches (183, 3,837,
  5,185, 6,299, 550 and 16 pixels across its listed spots); those references
  are not the current main-cache baseline.
- 090 seam metric, same executable/settings for all 21 views: all 18 fixed
  views had identical main/branch counts. For the fly pose at tile 24292,
  yaw +45 degrees and distance 8, totals were 417/159/430 on main and
  415/159/430 on the branch for 7/12/17h. No per-view count rose; aggregate
  2,948 to 2,946. This fly pose differs from the historical 090 capture
  (which recorded 557/8/457), so that historical aggregate of 3,022 is not a
  camera-matched comparison.
- Fly tour against current main completed: 29 of 43 spots changed. Sheets are
  in `runs/102-walls-from-inside/fly-tour/`. Changed spots: Kladwtwn tiles
  24268, 24292, 20268, 20292, 20690, 19265, 17104, 19108, 24678, 22958,
  24481, 14505, 25075, 23481, 22885 and 17699; Klamall 12666, 15450 and
  21700; Klatrap 21913; Klaratcv elevations 0/1/2 at 21511, 14701, 17107
  plus 13511 and 22661; Klatoxcv elevations 0/1/2 at 23879, 19091, 15065
  plus 22924. Most changes are the expected fly-side wall backs.

Matched runtime pairs at 1920×1080, scale 2, with the current executable are
saved under `runs/102-walls-from-inside/{bathhouse-14900,door-room-15272,
north-room-14892,west-room-22885}/validated/{before,after}`. They use
`-Fly -HeroTile=<tile> -ViewTile=<tile>` and identical angle/distance settings
within each pair. The door-room and the two extra room views show interiors;
the bathhouse capture is at tile 14900 and shows the nearby wall area, but the
owner JPEG has no camera metadata and the exact bathhouse interior viewpoint
could not be established. The mapping to rows 200/210 remains a source-based
match, not a pixel-matched reproduction. Thus the requested four before/after
views are present, but the specific owner bathhouse view is not proven to be
the same scene. Keep this task in `doing`; acceptance is not fully verified.
