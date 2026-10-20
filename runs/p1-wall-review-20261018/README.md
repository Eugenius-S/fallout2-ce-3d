# Static wall preview review, 2026-10-18

Accepted scope: fixed-camera Klamath e0 floor, static walls and doors, using original textures. This is not completed P1: live objects, interactive doors, picking, UI overlay and new lighting remain outstanding.

The reviewer inspected the actual game window and the GL PNG. The user explicitly approved this camera from the displayed result. Preserve its angle, perspective and scale when adding live objects:

- eye `(-3.95610, 11.00000, 21.11265)`
- target `(-11.95610, 1.00000, 9.11265)`
- vertical FOV `50`, near `0.1`, far `400`

Executable SHA-256: `251f7e919be107737a49ff63de0b6cfe4efc66ff0dd98af28470736558578b6d`.
CE base `f6c1969`, outer base `b647b1e`; implementation remains uncommitted. Metadata identifies the exact executable, not a fictitious committed build.

Tests ran in `game/runtime-review-20261018`, with copied config/saves/cache and read-only-by-use DAT hardlinks. No Steam installation changes.

| Check | Result | Evidence |
| --- | --- | --- |
| Floor and wall/door geometry in real GL window | PASS | `devwall-20261018-151520-000.png`, matching JSON; 7,758 triangles, one atlas page |
| GL PNG matches observed window | PASS | Independent Computer Use inspection and PNG inspection |
| Camera and EXE metadata | PASS | Both wall snapshot JSONs |
| Return to vanilla after 300 updates | PASS | `wall-return-stdout.log`, `scr00001.png`, `devshot-20261018-151646.json`; actual vanilla window observed |
| Three-byte walls.bin | PASS | `wall-corrupt-stdout.log`: invalid header, snapshot skipped, vanilla window observed |
| Missing sprites0.png | PASS | `wall-noatlas-stdout.log`: missing atlas, snapshot skipped, vanilla window observed |
| Failed runs preserve previous evidence | PASS | Wall PNG count stayed two through both negative runs |
| Physical Ctrl+F10/F9 | UNVERIFIED | Automated key delivery was unreliable in earlier floor tests; frame return uses the same disable helper |

Both sandbox cache files were restored and hashes checked after negative runs. `walls.bin` SHA-256: `e7fc068330b990f3385d2a0ccabc480e2c01cf3df7c384617c332f291d30ae0f`. Full exporter provenance is retained alongside this report. Exporter suite: 21 tests passed; Release CE and mapper build succeeded; CE diff check was clean.

The initial 1,800-update run was interrupted with physical Escape and did not establish automatic return. The fresh 300-update run above establishes it independently. In preview the ordinary software UI is hidden; menus/picking and gameplay in this view are not accepted.
