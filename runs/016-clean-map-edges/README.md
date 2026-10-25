# Task 016: Clean map edges

This evidence checks the intended boundary against the current installation's
Classic 2D renderer and the authored 3D clip. The owner's marked references are
kept byte-for-byte in `boundary-reference/owner-input/`.

- [Owner annotation 1](boundary-reference/owner-input/owner-annotation-1.jpg)
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

The active RPU EDG files are version 1. The engine reads each tile rectangle
followed by its next-elevation indicator; the indicator is not a zone count.
Parsing with the engine's `map_edge.cc` layout finds one rectangle at each of
e0, e1, and e2 for all seven maps. Rectangles below are source tile rectangles
in `(left, top, right, bottom)` order. The KLAGRAZ e0 source rectangle has
reversed X endpoints and needs normalization before use.

| Map | e0 EDG tile rectangle | e1 EDG tile rectangle | e2 EDG tile rectangle |
|---|---|---|---|
| KLACANYN | `(12111, 11684, 19447, 23294)` | `(199, 0, 39800, 39999)` | `(199, 0, 39800, 39999)` |
| KLADWTWN | `(13331, 6901, 23253, 25771)` | `(199, 0, 39800, 39999)` | `(199, 0, 39800, 39999)` |
| KLAGRAZ | `(21760, 13682, 21278, 26560)` | `(199, 0, 39800, 39999)` | `(199, 0, 39800, 39999)` |
| KLAMALL | `(24776, 6704, 29075, 33306)` | `(199, 0, 39800, 39999)` | `(199, 0, 39800, 39999)` |
| KLARATCV | `(15561, 13262, 20451, 24149)` | `(12922, 11266, 23452, 30104)` | `(17334, 14863, 27665, 29939)` |
| KLATOXCV | `(18350, 14276, 22465, 27320)` | `(13954, 10451, 20037, 28332)` | `(12506, 10867, 23063, 24897)` |
| KLATRAP | `(17939, 9281, 21063, 27334)` | `(199, 0, 39800, 39999)` | `(199, 0, 39800, 39999)` |

This is an archive-data audit. The matched runtime evidence below covers all
seven e0 maps and both cave maps at e1. The cave e2 source rectangles are
confirmed in the current e2 runtime snapshots; the remaining e1/e2 rectangles
are broad full-map bounds.

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

### Remaining Klamath edge views on the rebased build

These matched views use the same Release EXE SHA-256
`ce48a2392c0eeea9e5a818ff88fa2f2c9c7375e0e20c4d1d5586af4639e52ee9`,
1920x1080 output, and 2x scale. Each authored JSON records the EDG-derived
four-plane clip and its world bounds. The Classic and authored views share the
same map, elevation, and view tile.

| Map/elevation and view tile | Classic 2D | Authored 3D | Authored world bounds |
|---|---|---|---|
| KLADWTWN e0, 24768 | [PNG](boundary-reference/kladwtwn/matched-edge-rebased/classic/scr00129.png), [JSON](boundary-reference/kladwtwn/matched-edge-rebased/classic/devshot-20261020-141954.json) | [PNG](boundary-reference/kladwtwn/matched-edge-rebased/authored/devhero-20261020-142006-000.png), [JSON](boundary-reference/kladwtwn/matched-edge-rebased/authored/devshot-20261020-142006-000.json) | `[-38.041667,-34.562500,35.958333,34.437500]` |
| KLAMALL e0, 13237 | [PNG](boundary-reference/klamall/matched-edge-rebased/classic/scr00130.png), [JSON](boundary-reference/klamall/matched-edge-rebased/classic/devshot-20261020-142018.json) | [PNG](boundary-reference/klamall/matched-edge-rebased/authored/devhero-20261020-142030-000.png), [JSON](boundary-reference/klamall/matched-edge-rebased/authored/devshot-20261020-142030-000.json) | `[-37.041667,-35.812500,39.958333,36.187500]` |
| KLACANYN e0, 15909 | [PNG](boundary-reference/klacanyn/matched-edge/classic/scr00118.png), [JSON](boundary-reference/klacanyn/matched-edge/classic/devshot-20261020-141218.json) | [PNG](boundary-reference/klacanyn/matched-edge/authored/devhero-20261020-141228-000.png), [JSON](boundary-reference/klacanyn/matched-edge/authored/devshot-20261020-141228-000.json) | `[-31.541667,-25.562500,17.458333,14.437500]` |
| KLAGRAZ e0, 24373 | [PNG](boundary-reference/klagraz/matched-edge/classic/scr00119.png), [JSON](boundary-reference/klagraz/matched-edge/classic/devshot-20261020-141249.json) | [PNG](boundary-reference/klagraz/matched-edge/authored/devhero-20261020-141300-000.png), [JSON](boundary-reference/klagraz/matched-edge/authored/devshot-20261020-141300-000.json) | `[-20.041667,-25.562500,35.958333,28.437500]` |
| KLARATCV e0, 13471 | [PNG](boundary-reference/klaratcv/matched-edge/classic/scr00120.png), [JSON](boundary-reference/klaratcv/matched-edge/classic/devshot-20261020-141311.json) | [PNG](boundary-reference/klaratcv/matched-edge/authored/devhero-20261020-141324-000.png), [JSON](boundary-reference/klaratcv/matched-edge/authored/devshot-20261020-141324-000.json) | `[-32.041667,-34.312500,39.958333,26.187500]` |
| KLATOXCV e0, 15919 | [PNG](boundary-reference/klatoxcv/matched-edge/classic/scr00121.png), [JSON](boundary-reference/klatoxcv/matched-edge/classic/devshot-20261020-141335.json) | [PNG](boundary-reference/klatoxcv/matched-edge/authored/devhero-20261020-141346-000.jpg), [JSON](boundary-reference/klatoxcv/matched-edge/authored/devshot-20261020-141346-000.json) | `[-26.541667,-25.812500,32.458333,25.687500]` |
| KLATRAP e0, 20733 | [PNG](boundary-reference/klatrap/matched-edge-20733/classic/scr00124.png), [JSON](boundary-reference/klatrap/matched-edge-20733/classic/devshot-20261020-141547.json) | [PNG](boundary-reference/klatrap/matched-edge-20733/authored/devhero-20261020-141557-000.jpg), [JSON](boundary-reference/klatrap/matched-edge-20733/authored/devshot-20261020-141557-000.json) | `[-31.041667,-32.312500,30.458333,27.437500]` |
| KLARATCV e1, 14621 | [PNG](boundary-reference/klaratcv/matched-edge-e1/classic/scr00127.png), [JSON](boundary-reference/klaratcv/matched-edge-e1/classic/devshot-20261020-141739.json) | [PNG](boundary-reference/klaratcv/matched-edge-e1/authored/devhero-20261020-141749-000.png), [JSON](boundary-reference/klaratcv/matched-edge-e1/authored/devshot-20261020-141749-000.json) | `[-37.541667,-31.562500,30.458333,30.937500]` |
| KLATOXCV e1, 19091 | [PNG](boundary-reference/klatoxcv/matched-edge-e1/classic/scr00128.png), [JSON](boundary-reference/klatoxcv/matched-edge-e1/classic/devshot-20261020-141800.json) | [PNG](boundary-reference/klatoxcv/matched-edge-e1/authored/devhero-20261020-141810-000.jpg), [JSON](boundary-reference/klatoxcv/matched-edge-e1/authored/devshot-20261020-141810-000.json) | `[-42.041667,-41.312500,41.958333,33.187500]` |

KLATRAP tile 15823 was fully outside the active rectangle and produced an
all-black map view, so the accepted comparison uses tile 20733, where the
house and fence meet the playable edge. KLARATCV e0/e1 are naturally very dark;
their matched coordinates and EDG planes are verified in metadata, but those
frames provide limited visual detail. An ambient-override attempt did not
brighten the cave renderer and its diagnostic captures are excluded.

The two cave e2 regression snapshots also report `source=authored`; their
world bounds and four planes are in the regression run metadata. The RPU EDG
source rectangles are KLARATCV `(17334, 14863, 27665, 29939)` and KLATOXCV
`(12506, 10867, 23063, 24897)`.

The Classic snapshots verify the engine's 2D EDG cut for the current RPU
installation. Authored snapshot metadata reports EDG-derived bounds for all
seven e0 maps and the restricted cave e1/e2 levels. Visual detail varies by
map darkness; the cave evidence should be read with that limitation.
`pickStaticGroundTile` applies that clip and `sceneTileAt` returns `-1` when the
ground pick is outside it. The runtime probe below verifies that move-mode
mouse-up cannot use the stale cursor tile after an outside pick, and that a
reachable exit still transitions through the engine's normal exit-grid path.

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

### Final regression references

The final Release run used source commit `5bfbddcc` plus the effective-clip
gate fix recorded in the final task log. Its executable SHA-256 is
`a933abe956a57ed29d393d860aff3b83422df57ec77076d2a1eeda2aad8f4e0e`. The
authored clip was enabled with 1920x1080 output, 2x scale, and the private
scene cache. All 16 regression
spots passed. Pixel comparison against the pre-update references found zero
pixels above the configured channel tolerance of 2 in every spot. The updated
PNG/JSON files are in `tools/regression/reference/`; the prior PNGs remain
available at commit `5bfbddcca59a8a248495dc4b833e4d44edf8713a`.

Five pristine `origin/main` controls at `839673b912bf7e4c6495670f57a4de6e5dc88c2b`
were compared with matching clean-edges-off captures. All five pairs were
pixel-identical (zero pixels above tolerance 2). Their differences from the
pre-update regression references are therefore inherited baseline drift:

| Spot | Main vs off changed pixels / max channel delta | Main vs previous reference changed pixels / max delta |
|---|---:|---:|
| central-doors | 0 / 0 | 133 / 136 |
| KLARATCV e1 | 0 / 0 | 821 / 74 |
| KLATOXCV e1 | 0 / 0 | 22601 / 207 |
| KLARATCV e2 | 0 / 0 | 3847 / 66 |
| KLAMALL e0 | 0 / 0 | 3120 / 164 |

The main/off control captures and original JSON metadata are preserved in
[`regression-attribution/`](regression-attribution/). The final reference update
does not attribute these five old-reference deltas to clean-edge clipping.

## Earlier comparison captures

The original KLADWTWN and KLACANYN authored/floor/off comparison captures remain
in their existing folders. The floor-derived cut is not selected by this
evidence. The acceptance target is the current installation's original 2D
playable boundary, including transition corners; portions of buildings beyond
that line may be clipped.
