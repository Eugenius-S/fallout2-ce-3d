# Task 014 verification evidence

`full-regression.txt` and `.json` are the normal 16-spot regression run. It
used no `-Update` flag: all 16 spots passed with zero changed pixels.

The configuration captures use the same test executable (SHA-256 recorded in
`config-hashes.txt`) and fixed 1920x1080 output, 960x540 logical framebuffer,
scale 2, windowed mode. `missing-default.json`, `on.json`, and
`invalid-default.json` show the setting enabled and the Full scene active;
`off.json` records it disabled with a vanilla framebuffer. `on.png` and
`off.png` are the corresponding visual samples. The four isolated fixture
config hashes did not change during capture. The owner config hash also
matches its recorded pre-run value.

`artemple-fallback.*` records the default-on, same-map ARTEMPLE e0 case where
no 3D scene asset exists, so the game presents vanilla 2D. The tour JSON files
show the KLADWTWN e0 → KLARATCV e1 → KLADWTWN e0 sequence, including requested
map, elevation, and view tile.

The tested executable was built from source commit `6def1a0`; its metadata
contains `ce_commit=bc26534` from the build's generated version header. The
executable SHA-256 identifies the actual binary used for all runtime captures.
This folder contains verification outputs only, not configs, game resources,
or executables.
