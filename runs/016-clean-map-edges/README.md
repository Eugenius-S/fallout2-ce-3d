# Task 016: Clean map edges

These stills compare the authored EDG cut, the floor-derived cut, and the
`clean_edges=0` baseline. They use the Release executable built from source
commit `f29410be` (SHA-256
`51201dd056d197899589b6b07417f33df94291cb4d4ad95448829b1d14e01466`). All
captures are 1920x1080 at 2x scale, elevation 0, with a fixed camera and hero
tile per map. The original snapshot JSON is kept beside each PNG.

The implementation branch is rebased on `origin/main` at `10fff6d5`, preserving
the newer camera zoom and lighting changes alongside map clipping. The five
review captures remain pinned to their original source and executable metadata.

## KLADWTWN

View tile and hero tile: 8711.

| Variant | Snapshot |
|---|---|
| Authored EDG | [PNG](kladwtwn/authored/devhero-20261020-090613-000.png), [JSON](kladwtwn/authored/devshot-20261020-090613-000.json) |
| Floor-derived | [PNG](kladwtwn/floor/devhero-20261020-090640-000.png), [JSON](kladwtwn/floor/devshot-20261020-090640-000.json) |
| Edges off | [PNG](kladwtwn/off/devhero-20261020-090658-000.png), [JSON](kladwtwn/off/devshot-20261020-090658-000.json) |

## KLACANYN

View tile and hero tile: 15909.

| Variant | Snapshot |
|---|---|
| Authored EDG | [PNG](klacanyn/authored/devhero-20261020-090714-000.png), [JSON](klacanyn/authored/devshot-20261020-090714-000.json) |
| Floor-derived | [PNG](klacanyn/floor/devhero-20261020-090731-000.png), [JSON](klacanyn/floor/devshot-20261020-090731-000.json) |
| Edges off | Pending: shared test helper queue |

The captured JSON confirms the same camera, view/hero tile, still mode, scale,
source commit, and executable hash for each captured variant on its map. The
parent's visual review found that the floor-derived cut preserves the building
better on KLADWTWN; owner choice is still pending. Regression references were
not updated.

Validation on the rebased Release build: clean Release build passed; CTest
10/10; scene-export unit tests 38/38; regression unit tests 12/12.
`git diff --check` passed.
