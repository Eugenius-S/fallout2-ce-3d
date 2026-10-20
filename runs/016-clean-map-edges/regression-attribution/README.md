# Task 016 regression attribution

The final authored-clip run passed all 16 regression spots. After updating the
references, a second check found zero pixels above channel tolerance 2 in all
16 spots. The final references are in `tools/regression/reference/`; previous
references are available at commit
[`5bfbddcca59a8a248495dc4b833e4d44edf8713a`](https://github.com/Eugenius-S/fallout2-ce-3d/commit/5bfbddcca59a8a248495dc4b833e4d44edf8713a).

Before/after examples from the entrance and map edge show the visual changes
against those earlier references:

| Spot | Before | Final authored clip | Difference image |
|---|---|---|---|
| entrance | [PNG](edge-examples/entrance/before.png) | [PNG](edge-examples/entrance/after.png), [JSON](edge-examples/entrance/after.json) | [PNG](edge-examples/entrance/diff/difference.png) |
| map edge | [PNG](edge-examples/map-edge/before.png) | [PNG](edge-examples/map-edge/after.png), [JSON](edge-examples/map-edge/after.json) | [PNG](edge-examples/map-edge/diff/difference.png) |

The entrance comparison changed 191249 pixels (max channel delta 216); the
map-edge comparison changed 454938 pixels (max channel delta 216), using the
regression checker threshold of 2.

Five pristine main-branch captures from
`839673b912bf7e4c6495670f57a4de6e5dc88c2b` were compared with matching
clean-edges-off captures. All capture pairs use the same map, elevation,
view tile, camera, 1920x1080 drawable, and 2x scale. Main and off images were
pixel-identical at tolerance 2. The table also shows each main image's
difference from the previous reference. These matching main/off differences
are inherited baseline drift, not a clipping-toggle effect.

| Spot | Main | Off | Previous reference | Updated reference | Difference image | Main vs off | Main vs previous reference |
|---|---|---|---|---|---|---:|---:|
| central-doors | [PNG](central-doors/main.png), [JSON](central-doors/main.json) | [PNG](central-doors/off.png), [JSON](central-doors/off.json) | [Before](https://raw.githubusercontent.com/Eugenius-S/fallout2-ce-3d/5bfbddcca59a8a248495dc4b833e4d44edf8713a/tools/regression/reference/central-doors.png) | [After](../../../tools/regression/reference/central-doors.png) | [PNG](central-doors/difference.png) | 0 px / max 0 | 133 px / max 136 |
| KLARATCV e1 | [PNG](klaratcv-e1/main.png), [JSON](klaratcv-e1/main.json) | [PNG](klaratcv-e1/off.png), [JSON](klaratcv-e1/off.json) | [Before](https://raw.githubusercontent.com/Eugenius-S/fallout2-ce-3d/5bfbddcca59a8a248495dc4b833e4d44edf8713a/tools/regression/reference/klaratcv-e1.png) | [After](../../../tools/regression/reference/klaratcv-e1.png) | [PNG](klaratcv-e1/difference.png) | 0 px / max 0 | 821 px / max 74 |
| KLATOXCV e1 | [PNG](klatoxcv-e1/main.png), [JSON](klatoxcv-e1/main.json) | [PNG](klatoxcv-e1/off.png), [JSON](klatoxcv-e1/off.json) | [Before](https://raw.githubusercontent.com/Eugenius-S/fallout2-ce-3d/5bfbddcca59a8a248495dc4b833e4d44edf8713a/tools/regression/reference/klatoxcv-e1.png) | [After](../../../tools/regression/reference/klatoxcv-e1.png) | [PNG](klatoxcv-e1/difference.png) | 0 px / max 0 | 22601 px / max 207 |
| KLARATCV e2 | [PNG](klaratcv-e2/main.png), [JSON](klaratcv-e2/main.json) | [PNG](klaratcv-e2/off.png), [JSON](klaratcv-e2/off.json) | [Before](https://raw.githubusercontent.com/Eugenius-S/fallout2-ce-3d/5bfbddcca59a8a248495dc4b833e4d44edf8713a/tools/regression/reference/klaratcv-e2.png) | [After](../../../tools/regression/reference/klaratcv-e2.png) | [PNG](klaratcv-e2/difference.png) | 0 px / max 0 | 3847 px / max 66 |
| KLAMALL e0 | [PNG](klamall-e0/main.png), [JSON](klamall-e0/main.json) | [PNG](klamall-e0/off.png), [JSON](klamall-e0/off.json) | [Before](https://raw.githubusercontent.com/Eugenius-S/fallout2-ce-3d/5bfbddcca59a8a248495dc4b833e4d44edf8713a/tools/regression/reference/klamall-e0.png) | [After](../../../tools/regression/reference/klamall-e0.png) | [PNG](klamall-e0/difference.png) | 0 px / max 0 | 3120 px / max 164 |

The main controls were built from the pristine main commit above. Their
executable SHA-256 is recorded in each original JSON file. The off captures
were built from task 016 before the final commit and used the isolated off
configuration. The final run used source `5bfbddcc`, the isolated on
configuration, and the private scene cache; its full capture set is preserved
in the regression reference update and its run summary is under the ignored
`build/task016-regression/final-on-20261020/` directory.
