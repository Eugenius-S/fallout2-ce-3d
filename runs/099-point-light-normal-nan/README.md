# 099 point-light normal evidence

Shader change: `sceneLighting` mode 2 uses the face cross product already computed before alpha discard (`gFaceCross`). No sun, light color, or intensity code changed.

## Targeted before/after captures

All pairs are 1920x1080, scale 2, `scene_view=full`; sidecars match map, elevation, view tile, camera, hero, and render settings. Only run/time and executable SHA differ. Pixel counts compare decoded RGBA; alpha changed in 0 pixels.

| View | Changed pixels | Changed RGB channel values | Max channel delta | Pixel bounds |
|---|---:|---:|---:|---|
| KLARATCV e0, tile 17541 | 1,179 | 3,193 | 55 | 301,31–1843,875 |
| KLATOXCV e0, tile 17941 | 929 | 2,724 | 52 | 603,84–1510,490 |
| KLATRAP e0, tile 19721, test ambient override 16384 | 4 | 12 | 27 | 718,402–852,505 |

The Klamath Trapping Cave has no measurable change with its normal ambient=1.0. The captured pair uses the dev-only `-Ambient 16384` test override to make the barrel point light visible; this does not modify source code or the sun. Treat that pair as supporting evidence, not a normal-settings acceptance result.

Full-frame sheets: [rat cave](rat-cave-e0-before-after.jpg), [toxic cave](toxic-cave-e0-before-after.jpg), [Klamath barrels](klamath-lit-barrels-e0-before-after.jpg). The toxic cave crop [marks changed pixels in magenta at 3x](toxic-cave-changed-edges-3x.jpg). At full resolution, differences follow cutout contours on cave walls/rocks and the acid-pool rim; no isolated new dark/bright speck is visible. Visible warm lamp/light area is on the right side around x=1500,y=320; the changed contours are visible around the nearby rock group around x=1360,y=310 and the acid edge around x=700,y=460.

## Regression

Targeted before/after capture fixed EXE SHA256 (before the 098 rebase): `4f2ce48a9aa7dedfd0f12e0dce07bd226f8ece6b3e56f7f5e4b7edf415c3ca9f`.
Clean 098 baseline exe SHA256: `23a382f0d445f924bba10acdd4e58e6839f112b637803d0a79398d532ad1984c`.

Compared the final 16 3D regression frames directly against `098-regression-baseline` as RGBA with matching metadata. Ten are exactly 0 px. Six cave frames changed near point lights: KLARATCV e0/e1/e2 = 204/1,175/676 px; KLATOXCV e0/e1/e2 = 764/300/861 px. All other ten are 0 px. This is the intended 3D lighting scope, so the full 3D cave set is not pixel-identical.

Classic control used `-Classic -Still` at all 16 regression points, 1920x1080 scale 2. All are vanilla framebuffer (`scene_view=off`) and before/after PNGs compare exactly at 0 px; map/elevation/tile/camera/resolution/scale metadata matches. Twelve points captured with direct startup targets. Four (KLACANYN e0 and KLATOXCV e0/e1/e2) did not emit a screenshot on direct startup: the isolated logs stopped after `startup target ... accepted=1`. The test-only workaround starts on KLADWTWN, then uses the existing `-Tour` path to visit the four targets; all four produce matching captures on baseline and fixed builds. No game or shader source changed for the workaround. Classic exe SHA256: baseline `23a382f0d445f924bba10acdd4e58e6839f112b637803d0a79398d532ad1984c`; fixed `4f2ce48a9aa7dedfd0f12e0dce07bd226f8ece6b3e56f7f5e4b7edf415c3ca9f`.

After rebasing onto merged 098 and rebuilding, the final EXE SHA256 is `e8bb1bbff72f582d9d14f4c407920a06ad4e83c94c7d4d9d06952a49c184c730` (CE commit `6ef11132`). Refreshed exactly six stored references from isolated 1920x1080, scale 2 captures: KLARATCV e0/e1/e2 and KLATOXCV e0/e1/e2. Before-to-new-reference changed pixel counts were 204, 1,175, 720, 8,437, 535,552, and 861 respectively. Their updated JSON sidecars retain the capture map/elevation/tile/camera, renderer, resolution, scale, CE commit, and EXE SHA. Per-capture process logs, JSON, PNGs, previous reference PNGs, and SHA values are in [`reference-refresh-6.txt`](reference-refresh-6.txt) and ignored `build/099-raw/reference-refresh-6/`.

Full final `tools/regression.ps1` captured all 16 points: 15 pass at 0 changed pixels. KLAGRAZ e0 remains 132 px different from its stored reference, the same deviation as on clean 098 baseline; it was not changed and is recorded in `tasks/inbox.md`.

## Automated tests

- CTest: 18/18 passed.
- `py -3 -m unittest discover -s tools/regression -p 'test*.py' -v`: 12/12 passed.
- Full final `tools/regression.ps1`: 15/16 pass at 0 px; KLAGRAZ e0 has the pre-existing 132 px reference difference.
