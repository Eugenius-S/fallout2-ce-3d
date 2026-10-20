# Task 016: Clean map edges

This evidence checks the intended boundary against the current installation's
Classic 2D renderer and the authored 3D clip. The owner's marked references are
kept byte-for-byte in `boundary-reference/owner-input/`.

- [Owner annotation 1](boundary-reference/owner-input/owner-annotation-1.png)
- [Owner annotation 2](boundary-reference/owner-input/owner-annotation-2.png)

## Boundary source

For these maps, the active Restoration Project archive is
`mods/rpu.dat` (SHA-256
`27db830e0fb67d02b60e166e2acb772a92292f5f4dd88f733b740b406a0864ee`). It
contains the version 1 EDG rectangles below. These are current-installation
values, not a claim about an unmodified vanilla installation.

| Map | Active EDG e0 tile rectangle | Classic pixel rectangle | Authored 3D world polygon, in order |
|---|---|---|---|
| KLADWTWN, Klamath Downtown | `(13331, 6901, 23253, 25771)` | `(2624, 1008, 5312, 2544)` | `(-38.0417,-13.5625)`, `(3.9583,-34.5625)`, `(35.9583,13.4375)`, `(-6.0417,34.4375)` |
| KLAMALL, Klamath Mall / Trapper Town | `(24776, 6704, 29075, 33306)` | `(2688, 1008, 5472, 2616)` | `(-37.0417,-14.0625)`, `(6.4583,-35.8125)`, `(39.9583,14.4375)`, `(-3.5417,36.1875)` |

The pixel rectangle is the EDG v1 visible rectangle. It is distinct from the
contracted scroll border and from the discrete exit-grid tiles. The exit grids
remain the map's transition triggers; they are not used as map-wide bounds.

### Active EDG coverage

The same active RPU archive contains one EDG zone at e0 and one at e1 for each
of the seven Klamath maps below. No e2 EDG record was found. That only describes
the clipping data: KLARATCV and KLATOXCV still have e2 map fixtures, so a
missing e2 EDG record must not be read as a missing map elevation. Rectangles
below are source tile rectangles in `(left, top, right, bottom)` order; the
KLAGRAZ e0 source rectangle has reversed X endpoints and needs normalization
before use.

| Map | e0 EDG tile rectangle | e1 EDG tile rectangle | e2 EDG |
|---|---|---|---|
| KLACANYN | `(12111, 11684, 19447, 23294)` | `(199, 0, 39800, 39999)` | absent |
| KLADWTWN | `(13331, 6901, 23253, 25771)` | `(199, 0, 39800, 39999)` | absent |
| KLAGRAZ | `(21760, 13682, 21278, 26560)` | `(199, 0, 39800, 39999)` | absent |
| KLAMALL | `(24776, 6704, 29075, 33306)` | `(199, 0, 39800, 39999)` | absent |
| KLARATCV | `(15561, 13262, 20451, 24149)` | `(12922, 11266, 23452, 30104)` | absent |
| KLATOXCV | `(18350, 14276, 22465, 27320)` | `(13954, 10451, 20037, 28332)` | absent |
| KLATRAP | `(17939, 9281, 21063, 27334)` | `(199, 0, 39800, 39999)` | absent |

This is an archive-data audit, not a visual reference for every map. The
Classic corner captures below currently verify KLADWTWN and KLAMALL only; the
remaining maps still need runtime boundary review.

## Runtime references

All frames use the same Release executable, SHA-256
`ecdd1f0bec75e62ad1ba6e7ca57c8011bc55be68397fc76e8cce46b313b1ca38`, at
1920x1080 and 2x scale. Each original snapshot JSON is beside its PNG.

KLADWTWN Classic 2D corner views are centered at tiles 14424, 6108, 33688,
and 25372. The matched edge view at tile 24768 compares the 2D cut with the
authored 3D cut:

| View | Classic 2D | Authored 3D |
|---|---|---|
| Matched edge, tile 24768 | [PNG](boundary-reference/kladwtwn/matched-edge/scr00095.png), [JSON](boundary-reference/kladwtwn/matched-edge/devshot-20261020-093830.json) | [PNG](boundary-reference/kladwtwn/matched-edge/devhero-20261020-092926-000.png), [JSON](boundary-reference/kladwtwn/matched-edge/devshot-20261020-092926-000.json) |
| Corner 1, tile 14424 | [PNG](boundary-reference/kladwtwn/classic-corners/corner-1/scr00100.png), [JSON](boundary-reference/kladwtwn/classic-corners/corner-1/devshot-20261020-094143.json) | - |
| Corner 2, tile 6108 | [PNG](boundary-reference/kladwtwn/classic-corners/corner-2/scr00101.png), [JSON](boundary-reference/kladwtwn/classic-corners/corner-2/devshot-20261020-094155.json) | - |
| Corner 3, tile 33688 | [PNG](boundary-reference/kladwtwn/classic-corners/corner-3/scr00102.png), [JSON](boundary-reference/kladwtwn/classic-corners/corner-3/devshot-20261020-094208.json) | - |
| Corner 4, tile 25372 | [PNG](boundary-reference/kladwtwn/classic-corners/corner-4/scr00103.png), [JSON](boundary-reference/kladwtwn/classic-corners/corner-4/devshot-20261020-094221.json) | - |

KLAMALL Classic 2D corner views are centered at tiles 14226, 5713, 34493,
and 25780. At tile 13237, both render paths show the boundary beside a
building, so the 3D clip may cut through building and fence parts beyond the
playable edge as marked by the owner:

| View | Classic 2D | Authored 3D |
|---|---|---|
| Matched building edge, tile 13237 | [PNG](boundary-reference/klamall/matched-edge/scr00104.png), [JSON](boundary-reference/klamall/matched-edge/devshot-20261020-094601.json) | [PNG](boundary-reference/klamall/matched-edge/devhero-20261020-094545-000.png), [JSON](boundary-reference/klamall/matched-edge/devshot-20261020-094545-000.json) |
| Corner 1, tile 14226 | [PNG](boundary-reference/klamall/classic-corners/corner-1/scr00105.png), [JSON](boundary-reference/klamall/classic-corners/corner-1/devshot-20261020-094627.json) | - |
| Corner 2, tile 5713 | [PNG](boundary-reference/klamall/classic-corners/corner-2/scr00106.png), [JSON](boundary-reference/klamall/classic-corners/corner-2/devshot-20261020-094641.json) | - |
| Corner 3, tile 34493 | [PNG](boundary-reference/klamall/classic-corners/corner-3/scr00107.png), [JSON](boundary-reference/klamall/classic-corners/corner-3/devshot-20261020-094654.json) | - |
| Corner 4, tile 25780 | [PNG](boundary-reference/klamall/classic-corners/corner-4/scr00108.png), [JSON](boundary-reference/klamall/classic-corners/corner-4/devshot-20261020-094708.json) | - |

The Classic snapshots verify the engine's 2D EDG cut for the current RPU
installation. Authored snapshot metadata reports matching transformed bounds
for both town maps. These are the first two verified Klamath maps; the other
Klamath maps with EDG rectangles remain to be checked before task completion.
`pickStaticGroundTile` applies that clip and
`sceneTileAt` returns `-1` when the ground pick is outside it. In move mode,
`game_mouse.cc` leaves the previous cursor tile in place when `tileUnderMouse`
returns `-1`. The full mouse command path and a real exit-grid transition have
not yet been verified, so this evidence does not claim that movement outside
the edge is blocked or that every exit remains reachable.

## Earlier comparison captures

The original KLADWTWN and KLACANYN authored/floor/off comparison captures remain
in their existing folders. The floor-derived cut is not selected by this
evidence. The acceptance target is the current installation's original 2D
playable boundary, including transition corners; portions of buildings beyond
that line may be clipped.
