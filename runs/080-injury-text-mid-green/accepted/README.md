# Accepted injury-label color evidence

The 1:1 JPEG presentation crop is `injury-labels.jpg`, extracted from the raw frame rectangle `(840,220)-(1200,470)` without scaling or retouching. JPEG encoding changes pixel values, so use the raw PNG for exact color checks.

The source-bound character fixture passed at 2310x1298, scale 2. Its sidecar is `devshot-20261025-155111.json`; the raw frame `scr00205.png` is local at `E:\Projects\fallout2-ce-3d\game\shots\raw-task080\scr00205.png`.

- Source commit: `3a3c7e91`.
- EXE SHA-256: `b268dd121f0046f8b5820d5813e9cdca609809f0f2d493ec32b7543719c15202`.
- Raw PNG SHA-256: `6842AFBDFB5CA6D11D6A68F81B60A0968C21600E6C344781A9297397528C6E13`.
- JPEG SHA-256: `DC10A315526FFA34D472344357427A2B9DC7C36024EF7C10217BF3B775F06A99`; 360x250, 44,701 bytes.
- Exact RGB `(6, 79, 3)` appears in 3946 pixels of the injury crop. The old `(12, 35, 5)` and previous `(0, 123, 0)` targets appear zero times. Active green `(63, 180, 25)` remains visible in 605 pixels.

`process.stdout.log` records `UI_FIXTURE PASS`, with 91 visible font-101 character-screen runs. `process.stderr.log` contains the renderer startup diagnostic.
