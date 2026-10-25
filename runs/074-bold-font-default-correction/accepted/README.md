# Accepted runtime evidence for task 074

All accepted captures use source `b2b11252`, EXE SHA-256 `b42652371149fd364a573aba284cf1497cdf267118d88a18056148fabd3ce704`, 2310x1298 resolution, and scale 2. Original JSON sidecars and fixture stdout/stderr are kept next to each image.

## Inventory and dialogue

- `inventory/scr00202.png`: 34 visible font-101 inventory runs use the compact size role, `D1.22`. SHA-256: `7C52160BC846D07589E9C4C48D896AB6629B7B52A62AD51B9DD6DBC63439BE0D`.
- `dialog/scr00203.png`: Russian Klamath conversation, 11 eligible font-101 runs, with global Bold default `E1.29`. SHA-256: `0C30864B4E2501B47D48115C72564ADD204317C2F8683F86C9EA6823AC3DD27C`.

Both fixtures completed and emitted PASS. The snapshot wrapper's PNG copy step looked in the `shots/dev` folder, while these images were written to `game/shots`. They were copied from the exact image named in each source-matched JSON sidecar; the wrapper issue happened after capture and did not affect the game run.

## Character and PipBoy

- `character/scr00199.png`: injury text uses the approved inactive dark green. SHA-256: `BF269D316F5D8A53156A1C199444F0F4A6786B0C879950509BD96C1DA3EC7C32`.
- `pipboy/scr00200.png`: underline follows glyph ink, with the approved one-pixel gap. SHA-256: `60987C39333853CD184B6A0E35C430EC841C2E8ED9B3CBD1386EEF55A1EE571A`.

## Floating bark lifecycle

- `bark/devhero-20261025-143840-000.png`: initial Russian bark. SHA-256: `AA7F86E2392845433E8B0B6685C72FF9F347EDC52D777D814B6396596AB004A5`.
- `bark/devhero-20261025-143841-000.png`: shorter replacement. SHA-256: `EC6B6CD0184CC6DE6ACB083CB481A6DAF7DA78FF58B5A627E16E6E3AC79AD0B6`.
- `bark/devhero-20261025-143844-000.png`: after natural expiry. SHA-256: `0E2903F11FDC084F2EF0B7B70D602AFB22B7EA25969CE1676250651B71386092`.

The fixture found 1955 visible pixels in the initial line and 595 in the replacement, with no visible old run. After expiry both fixture lines were absent and the clear-run count was zero.

## Live game comparison

The owner's running GLM build was commit `e08785c8`, before this font implementation; its config was 2310x1298 at scale 2. Earlier reviewed captures used 1920x1440 at scale 3. The source and interface scale explain the apparent size difference. The owner's process was not changed.
