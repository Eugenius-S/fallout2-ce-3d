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

Final 3D exe SHA256: `4f2ce48a9aa7dedfd0f12e0dce07bd226f8ece6b3e56f7f5e4b7edf415c3ca9f`.
Clean 098 baseline exe SHA256: `23a382f0d445f924bba10acdd4e58e6839f112b637803d0a79398d532ad1984c`.

Compared the final 16 3D regression frames directly against `098-regression-baseline` as RGBA with matching metadata. Ten are exactly 0 px. Six cave frames changed near point lights: KLARATCV e0/e1/e2 = 204/1,175/676 px; KLATOXCV e0/e1/e2 = 764/300/861 px. All other ten are 0 px. This is the intended 3D lighting scope, so the full 3D cave set is not pixel-identical.

Classic control used `-Classic -Still` at the same points, 1920x1080 scale 2. It is a vanilla framebuffer with `scene_view=off`; 12 available pairs are exact 0 px and their map/elevation/tile/camera/resolution/scale metadata matches. Classic exe SHA256: baseline `23a382f0d445f924bba10acdd4e58e6839f112b637803d0a79398d532ad1984c`; fixed `4f2ce48a9aa7dedfd0f12e0dce07bd226f8ece6b3e56f7f5e4b7edf415c3ca9f`. Four Classic points did not produce a screenshot/sidecar within 25s: KLACANYN e0 and KLATOXCV e0/e1/e2. Their 3D captures completed. Thus Classic 0 px is proven on 12/16 points, not all 16.

The standard reference checker reports eight cave/Klamath reference mismatches; direct comparison to the 098 baseline above is the relevant attribution check because the stored references have different counts. No reference images were updated.

## Automated tests

- CTest: 18/18 passed.
- `py -3 -m unittest discover -s tools/regression -p 'test*.py' -v`: 12/12 passed.
- Full `tools/regression.ps1`: completed all 16 captures; compare results are recorded above.
