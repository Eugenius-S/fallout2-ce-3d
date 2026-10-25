# Accepted runtime evidence for task 074

The original PNG captures remain local in `E:\Projects\fallout2-ce-3d\game\shots\raw-task074\`. Their original JSON sidecars, fixture stdout/stderr, EXE identity, and raw PNG hashes are committed here. The four JPEGs under `crops/` are direct crops or sheets made from those originals. JPEG encoding changes pixel values, so these are presentation evidence, not pixel-identity artifacts. Source crops were not retouched or scaled.

## JPEG presentation evidence

- `crops/character-injury.jpg`: 1:1 crop of the injury/status plaque from `scr00199.png`, source rectangle `(840,220)-(1200,470)`, 360x250, 51,229 bytes. JPEG SHA-256: `92C443B2278CA5B063538A3AC46AEB142CCDF43B45D1AA93402C3942D9DE6206`.
- `crops/pipboy-underline.jpg`: 1:1 crop of the PipBoy header from `scr00200.png`, source rectangle `(990,220)-(1800,600)`, 810x380, 92,162 bytes. JPEG SHA-256: `E518F94D52EB9F6811061B24D18891D2C40CD2E5D7C3FC2D9A4153CE1EC73B6B`.
- `crops/bark-lifecycle.jpg`: three same-size 1:1 crops from initial, replacement, and post-expiry frames in that order. Each uses source rectangle `(1400,310)-(1840,500)`; 1336x190 with 8-pixel gutters, 197,201 bytes. JPEG SHA-256: `FE1C7E8CBD93B0030C2D1E9A9324CB5797CBF95BF4F6E36A0F1E83715259E4C5`.
- `crops/inventory-dialog.jpg`: 1:1 inventory and dialogue crops shown side-by-side in that order. Source rectangles are `(1200,220)-(1920,620)` and `(760,720)-(1480,1120)`; 1448x400 with an 8-pixel gutter, 361,976 bytes. JPEG SHA-256: `5F1751A6CB69721234FEE97BBD2AB7474557219710C53E7E822732BF25CEE0F4`.

All four JPEGs are below the repository's 1 MiB limit. No screenshot has been synthesized; the sheet only places direct crops side-by-side.

## Inventory and dialogue

- Inventory `scr00202.png`: 34 visible font-101 runs use compact size role `D1.22`. Raw PNG SHA-256: `7C52160BC846D07589E9C4C48D896AB6629B7B52A62AD51B9DD6DBC63439BE0D`.
- Dialogue `scr00203.png`: Russian Klamath conversation with 11 eligible font-101 runs at global Bold default `E1.29`. Raw PNG SHA-256: `0C30864B4E2501B47D48115C72564ADD204317C2F8683F86C9EA6823AC3DD27C`.

Both fixtures completed and emitted PASS. The snapshot wrapper looked for the PNG beside the `shots/dev` JSON, while the images were written to `shots/game`. Their raw copies were recovered from those exact sidecar paths; this post-capture wrapper copy error did not affect either game run.

## Character and PipBoy

- Character `scr00199.png`: injury text uses the approved inactive dark green. Raw PNG SHA-256: `BF269D316F5D8A53156A1C199444F0F4A6786B0C879950509BD96C1DA3EC7C32`.
- PipBoy `scr00200.png`: underline follows glyph ink, with the approved one-pixel gap. Raw PNG SHA-256: `60987C39333853CD184B6A0E35C430EC841C2E8ED9B3CBD1386EEF55A1EE571A`.

## Floating bark lifecycle

- Initial `devhero-20261025-143840-000.png`: raw SHA-256 `AA7F86E2392845433E8B0B6685C72FF9F347EDC52D777D814B6396596AB004A5`.
- Replacement `devhero-20261025-143841-000.png`: raw SHA-256 `EC6B6CD0184CC6DE6ACB083CB481A6DAF7DA78FF58B5A627E16E6E3AC79AD0B6`.
- After expiry `devhero-20261025-143844-000.png`: raw SHA-256 `0E2903F11FDC084F2EF0B7B70D602AFB22B7EA25969CE1676250651B71386092`.

The fixture found 1955 visible pixels in the initial line and 595 in the replacement, with no visible old run. After expiry both fixture lines were absent and the clear-run count was zero.

## Capture identity

All sidecars identify source `b2b11252`, EXE SHA-256 `b42652371149fd364a573aba284cf1497cdf267118d88a18056148fabd3ce704`, resolution 2310x1298, and scale 2.

The owner's running GLM build was commit `e08785c8`, before this font implementation; its config was 2310x1298 at scale 2. Earlier reviewed captures used 1920x1440 at scale 3. The older source and smaller interface scale explain the apparent size difference. The owner's process was not changed.
