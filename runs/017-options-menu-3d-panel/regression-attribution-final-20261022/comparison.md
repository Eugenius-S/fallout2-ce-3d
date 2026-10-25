# Gameplay regression attribution

## Final result

The owner explicitly approved replacing the outdated references from the pristine control. The official checker updated all 16 references from `build/task017-baseline-source/runs/regression/20261022-093020-641-44416` (source `32f0fe2b`, executable SHA-256 `ddc03416aa937b3bff789b7fb92ea636760523efd0da660f9dbbd9f5880636f4`). The control's relevant source paths match `origin/main` at `5d3056c8`: `git diff --exit-code 32f0fe2b origin/main -- ce tools/regression tools/test-config` returned 0.

Command: `py -3 tools/regression/check.py --run build/task017-baseline-source/runs/regression/20261022-093020-641-44416 --reference tools/regression/reference --update`. All 16 captures validated and were updated. A fresh official candidate run at `runs/regression/20261022-192743-346-44732` then passed 16/16 with 0 changed pixels and exit code 0. Decoded RGBA SHA-256 hashes for every refreshed reference, control capture, and candidate capture match; details are in `comparison.json`. No thresholds were changed.

## Initial attribution (before the approved refresh)

The first candidate run `runs/regression/20261022-191516-761-50648` (source `e29ef058`) exited 1: 13 old-reference failures and 3 passes. At that point, the clean control `build/task017-baseline-source/runs/regression/20261022-093020-641-44416` (source `32f0fe2b`) matched the candidate in RGBA for all 16 spots; the owner had not yet authorized a reference refresh.

| Spot | Legacy reference | Changed pixels | Candidate / control RGBA | Exact match |
| --- | --- | ---: | --- | --- |
| entrance | fail | 501 | [candidate](runs/regression/20261022-191516-761-50648/entrance/devhero-20261022-191526-000.png) / [control](build/task017-baseline-source/runs/regression/20261022-093020-641-44416/entrance/devhero-20261022-093033-000.png) | yes |
| corrals | fail | 361 | [candidate](runs/regression/20261022-191516-761-50648/corrals/devhero-20261022-191537-000.png) / [control](build/task017-baseline-source/runs/regression/20261022-093020-641-44416/corrals/devhero-20261022-093044-000.png) | yes |
| residents | fail | 380 | [candidate](runs/regression/20261022-191516-761-50648/residents/devhero-20261022-191549-000.png) / [control](build/task017-baseline-source/runs/regression/20261022-093020-641-44416/residents/devhero-20261022-093057-000.png) | yes |
| central-doors | fail | 133 | [candidate](runs/regression/20261022-191516-761-50648/central-doors/devhero-20261022-191559-000.png) / [control](build/task017-baseline-source/runs/regression/20261022-093020-641-44416/central-doors/devhero-20261022-093108-000.png) | yes |
| north-rooms | fail | 396 | [candidate](runs/regression/20261022-191516-761-50648/north-rooms/devhero-20261022-191610-000.png) / [control](build/task017-baseline-source/runs/regression/20261022-093020-641-44416/north-rooms/devhero-20261022-093118-000.png) | yes |
| map-edge | fail | 1085 | [candidate](runs/regression/20261022-191516-761-50648/map-edge/devhero-20261022-191621-000.png) / [control](build/task017-baseline-source/runs/regression/20261022-093020-641-44416/map-edge/devhero-20261022-093129-000.png) | yes |
| klacanyn-e0 | pass | 0 | [candidate](runs/regression/20261022-191516-761-50648/klacanyn-e0/devhero-20261022-191631-000.png) / [control](build/task017-baseline-source/runs/regression/20261022-093020-641-44416/klacanyn-e0/devhero-20261022-093138-000.png) | yes |
| klagraz-e0 | fail | 74 | [candidate](runs/regression/20261022-191516-761-50648/klagraz-e0/devhero-20261022-191641-000.png) / [control](build/task017-baseline-source/runs/regression/20261022-093020-641-44416/klagraz-e0/devhero-20261022-093147-000.png) | yes |
| klamall-e0 | fail | 3235 | [candidate](runs/regression/20261022-191516-761-50648/klamall-e0/devhero-20261022-191652-000.png) / [control](build/task017-baseline-source/runs/regression/20261022-093020-641-44416/klamall-e0/devhero-20261022-093157-000.png) | yes |
| klaratcv-e0 | pass | 0 | [candidate](runs/regression/20261022-191516-761-50648/klaratcv-e0/devhero-20261022-191702-000.png) / [control](build/task017-baseline-source/runs/regression/20261022-093020-641-44416/klaratcv-e0/devhero-20261022-093207-000.png) | yes |
| klaratcv-e1 | fail | 821 | [candidate](runs/regression/20261022-191516-761-50648/klaratcv-e1/devhero-20261022-191713-000.png) / [control](build/task017-baseline-source/runs/regression/20261022-093020-641-44416/klaratcv-e1/devhero-20261022-093216-000.png) | yes |
| klaratcv-e2 | fail | 3847 | [candidate](runs/regression/20261022-191516-761-50648/klaratcv-e2/devhero-20261022-191723-000.png) / [control](build/task017-baseline-source/runs/regression/20261022-093020-641-44416/klaratcv-e2/devhero-20261022-093226-000.png) | yes |
| klatoxcv-e0 | fail | 836 | [candidate](runs/regression/20261022-191516-761-50648/klatoxcv-e0/devhero-20261022-191734-000.png) / [control](build/task017-baseline-source/runs/regression/20261022-093020-641-44416/klatoxcv-e0/devhero-20261022-093237-000.png) | yes |
| klatoxcv-e1 | fail | 22777 | [candidate](runs/regression/20261022-191516-761-50648/klatoxcv-e1/devhero-20261022-191744-000.png) / [control](build/task017-baseline-source/runs/regression/20261022-093020-641-44416/klatoxcv-e1/devhero-20261022-093250-000.png) | yes |
| klatoxcv-e2 | pass | 0 | [candidate](runs/regression/20261022-191516-761-50648/klatoxcv-e2/devhero-20261022-191755-000.png) / [control](build/task017-baseline-source/runs/regression/20261022-093020-641-44416/klatoxcv-e2/devhero-20261022-093326-000.png) | yes |
| klatrap-e0 | fail | 2130 | [candidate](runs/regression/20261022-191516-761-50648/klatrap-e0/devhero-20261022-191805-000.png) / [control](build/task017-baseline-source/runs/regression/20261022-093020-641-44416/klatrap-e0/devhero-20261022-093341-000.png) | yes |

Per-spot dimensions, paths, and SHA-256 hashes of decoded RGBA bytes are stored in `comparison.json`. The 13 old-reference differences are pre-existing: the candidate is pixel-identical to the pristine `32f0fe2b` control, and the intervening `origin/main` changes contain no engine, regression-reference, or test-config changes. Keep task 017 in progress until the owner decides whether to replace those baselines.
