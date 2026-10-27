# Verification: task 095

Date: 2026-10-27. Branch: `task/095-camera-stops-at-map-edge`, base `cdb7b34e`.

## Change

Full 3D view no longer enables free scroll, so normal map-edge and object-blocker checks run. The separate expanded hero-distance limit was removed too: it let a wide 3D view scroll 240 px farther than Classic and produced different center tiles. Normal 3D and Classic scrolling now use the same original 480×400 limit. The fly camera remains on its independent `StaticFlyCamera` path.

## Verification

- Capture executable SHA-256: `3f5e0256285bd5023290f809c9a26325a9587dd79d64db735cb79c6baa339cf0`.
- CTest: 18/18 passed.
- KLADWTWN screen-edge probes: 3D and Classic matched on all sides after 300 stable frames: Left 17526, Right 22872, Up 12876, Down 26722. All eight markers report `edg_enabled=1` and `edg_zone=1`; see `captures.json` and paired panels.
- KLADWTWN arrow-key probes at those endpoints: all eight (four directions × two views) passed with `moved=0`.
- KLAMALL left-edge probe on a second map: both views stopped at 18715. EDG was enabled and its zone selected. Center was (464,212), hero was (944,212), delta (−480,0); this stops at the shared hero-distance guard, not at the map boundary.
- The normal mouse-driven `-Probe` clicked a walk target: logs show the hero moving 25075→25875→25889. Its two snapshots both report center 24678. A separate normal walk animation traversed a 27-step path, hero 20100→16100, with center unchanged at 20100. Camera follow during walking was therefore not demonstrated.
- A `-FlyDistance 120` scene snapshot was rendered, but interactive WASD movement was not tested. The isolated test window is placed offscreen; CUA did not expose it for input.
- All 16 regression PNGs were RGBA-identical to `E:/Projects/fallout2-ce-3d-gpt-094/build/094-direct-base` (0 differing pixels; stable metadata differences 0). Baseline capture executable SHA-256: `aad846b198bf96d588d0157c7bd2fc535dd41558e41a3a2cbed35330d9352354`. Final capture executable SHA-256 is listed above. See `regression-rgba.tsv`; raw captures are in `build/095-final-regression-3f5e0256/`.

## Review disposition

The owner accepted the change. The object-blocker branch is unreachable on the installed EDG maps: all 155 have a zone at each elevation. The original game has no camera follow during walking, so adding it is out of scope. The independent fly path is untouched by this change.

The screenshots above were captured before removal of the one-use blocker finder. The post-cleanup executable SHA-256 is `1becf6414fa63eaad52e413fcfb8f25e9aa65cc722de4af50b935c4ad17ea041`; CTest passed 18/18 after cleanup. The screenshots were not recaptured on that executable.
