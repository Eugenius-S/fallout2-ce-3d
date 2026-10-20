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

The boundary-reference frames use the same Release executable, SHA-256
`ecdd1f0bec75e62ad1ba6e7ca57c8011bc55be68397fc76e8cce46b313b1ca38`, at
1920x1080 and 2x scale. Each original snapshot JSON is beside its PNG.

### Live outside-click movement probe

The probe ran with the authored e0 clip on KLADWTWN through
`tools/dev-snapshot.ps1 -Probe`, using the isolated `task009-clean-edges-on`
configuration and private scene cache. Both snapshots share run ID
`5efe3b7e7ad6435088b0f3f15ec7e9de`, view tile 24678, hero tile 25075, and
Release executable SHA-256
`782ca2da4d04499592f26390097914d86ba2108fbdc9930bd6b967275589ce1e`
(`ce_commit` `a98aa84f`). Original PNG and JSON files are in
[`movement-probe/kladwtwn`](movement-probe/kladwtwn/).

The in-bounds positive control moved the hero from tile 25075 to 25875. The
probe then primed a pathable adjacent cursor target at tile 25674, distinct
from the idle hero at 25875. At logical screen coordinate `(956, 51)`, the
fresh 3D ground pick was `-1` while the cursor still held tile 25674. The
synthetic engine mouse-up left the hero at 25875 and animation state idle
(`busy=0->0`); after the follow-up wait the hero remained at 25875 and idle.
The log records `outside movement rejected`, with no probe failure. The
existing talker-click stage ran only after this assertion.

The raw [`devprobe.log`](movement-probe/kladwtwn/devprobe.log) and both
same-run snapshot pairs are retained byte-for-byte as runtime evidence.

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
`pickStaticGroundTile` applies that clip and `sceneTileAt` returns `-1` when the
ground pick is outside it. The runtime probe below verifies that move-mode
mouse-up cannot use the stale cursor tile after an outside pick, and that a
reachable exit still transitions through the engine's normal exit-grid path.
This does not yet verify visual bounds for the other five Klamath maps.

### Live exit-grid transition probe

The existing dev pick probe ran on KLAMALL e0, with the authored clip enabled,
using the isolated `task009-clean-edges-on` config and private scene cache. It
found the live exit-grid object at tile 28078 and verified its destination
fields: map index 9, tile 25142, elevation 0. The hero moved normally from tile
29889 to that grid; the click changed animation from idle to moving (`busy=0->1`).
The engine then loaded KLADWTWN e0, map index 9, at tile 25142 after 59 probe
frames. The probe did not invoke `mapSetTransition` or move the hero by a test
teleport. The map files and maps table came from the active `mods/rpu.dat`
archive recorded in the scene provenance.

The raw log's `exit probe queued` line records the live exit tile and
destination; `exit transition passed` records the arrival. Its `exit click
dispatched` line has a stale `target=29688` field from the preceding outside
click stage, so that field is not evidence of the exit target. The dev-probe
logger was corrected afterward to record the dispatched screen coordinates.

The original [`devprobe.log`](exit-transition/klamall-to-kladwtwn/devprobe.log),
two same-run PNG/JSON snapshot pairs, and metadata are in
[`exit-transition/klamall-to-kladwtwn`](exit-transition/klamall-to-kladwtwn/).
The run ID is `806e4533ce9645d4983cc4d231048451`. The executable SHA-256 is
`b5742fbd121a3bfbc2896ee256a5a65fe15e9d950059fe48cffef91638d12566`; it was
built from source commit `22baf835` plus the local dev-probe-only transition
check captured in this evidence checkpoint.

## Earlier comparison captures

The original KLADWTWN and KLACANYN authored/floor/off comparison captures remain
in their existing folders. The floor-derived cut is not selected by this
evidence. The acceptance target is the current installation's original 2D
playable boundary, including transition corners; portions of buildings beyond
that line may be clipped.
