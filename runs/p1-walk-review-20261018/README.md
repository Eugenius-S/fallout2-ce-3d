# Perspective walking review, 2026-10-18

The reviewer and user confirmed visible perspective walking and the yellow ground-hex marker. Modal presentation is a separate outstanding regression at this checkpoint: a preview can cover options/pause while their nested loops keep rendering. This report does not accept menus, object picking, UI overlay or full P1.

Controlled runtime executable SHA-256: `f0e3863c7add60819edeb3f24b7af255d90b65b39885345a80751559af40a726`. The exact running sandbox executable and JSON metadata were checked. An earlier implementer message named a different build hash; that hash is not used as evidence here.

Release build and independent incremental rebuild succeeded. The native CTest passed (1/1); its checks remain active under NDEBUG. It tests the actual production ground picker with independent camera goldens, donor hex boundaries, logical/drawable scaling, letterboxing, invalid projection and map bounds.

## Controlled mouse checks

The user granted a short interval without manual input. Computer Use then clicked the observed game window at `(213,292)` in its logical screenshot coordinates. This selected CE tile `25077`, consistent with the separately calculated perspective projection of that hex near `(265.56,327.99)` in the 640x480 game drawable. Pointer coordinates are subject to integer input scaling and need not hit the exact hex center.

| Check | Result | Evidence |
| --- | --- | --- |
| Ground click selects expected CE hex | PASS | Request `25077` in `walk-fresh-stdout.log` |
| Existing CE movement reaches that hex | PASS | From `24688` to `25077`, status `arrived`, pixel offset `(0,0)` |
| Marker is visible at the selected hex | PASS | GL PNG `devwalk-20261018-160949-000.png`, actual window inspected, user confirmation |
| Repeated click on current hex | PASS | `same-tile`, no additional move; `devshot-20261018-161008-001.json` |
| Accurate executable and player metadata | PASS | Paired GL PNG/JSONs included here |
| Actual UI test of unreachable target | OUTSTANDING | No controlled negative click accepted yet |
| Runtime letterbox behavior | OUTSTANDING | Native math tests passed; live resize check not yet accepted |

Movement uses the engine's normal animation/pathfinder registration with an explicit target tile, not direct position mutation. The former `_check_move` mouse conversion is bypassed only in the dedicated dev walking path. Other walk-preview mouse events are consumed rather than sent to the invisible vanilla interface.

Before controlled testing, manual input repeatedly interrupted automated window control. Those early logs are preserved but are not substituted for controlled checks. One old run timed out at a partial movement offset; no mode/state trace was captured at that time, so its specific cause remains unproven. The separately traced hidden-menu presentation defect is being fixed and tested.

Camera, map and geometry remain the accepted Klamath e0 baseline. This ground-ray picker is not object selection; NPC/door/item selection remains a later GPU-ID task.
