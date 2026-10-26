# Regression snapshots

From the repository root, with an existing Release build:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/regression.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools/regression.ps1 -Spot entrance
```

The runner first exports the selected spots' maps with this checkout's
exporter into `build/scene-cache` (the cache `dev-snapshot.ps1` reads), so a
run tests the exporter and the build of one commit, never a cache another
build left behind; it never writes the game's `mod-cache`. Task 058 found the
references of 8543064e taken from such a stale cache. Then it takes 16
independent background snapshots through `tools/dev-snapshot.ps1`; a capture
that shows no snapshot within the timeout gets one more try (the first error
stays as `capture-error-1.txt`). It never builds, starts the owner's game or
sends input to a window. Only its own `fallout2-ce-test.exe` PID is
stopped; the script waits for that process to exit before the next capture.
The test process receives a fixed 1920x1080 screen size and scale 2 from
`spots.json`. It runs as `build/test-run/fallout2-ce-test.exe` with the fixed
config from `tools/test-config/fallout2.cfg` (sound off, zoom 1, no rotation),
so it never reads or writes the owner's `game/fallout2.cfg`; the game's
resource directory stays its working directory.

`spots.json` names each map, elevation and exact view hex. The player remains
at the map entrance. `--dev-view-tile=N` bypasses camera bounds and starts the
3D camera at that hex without a glide. `--dev-still-snapshot` requires
`--dev-tools`: it seeds the map RNG with 1, disables color cycling and stops
background tickers before map loading (AI, idle animations and timers).
Synchronous map-enter scripts still initialize the world. This is a still
fixture, not a gameplay or animation test.

Every capture carries its actual camera tile, map, elevation, render path,
source commit, EXE SHA-256 and a unique run token. The launcher accepts only
JSON from that token. Comparison rejects a fallback 2D image, wrong hex,
missing PNG or invalid metadata even during a reference update.

Results are in `runs/regression/<timestamp>-<pid>/`: captures, JSON, logs,
`summary.json` and `summary.txt`. Changed pixels produce `difference.png`
(delta amplified 8x) and `highlight.png` (changes marked red); failed comparisons
also copy `reference.png`. Exit code 0 means every selected spot passed; a
missing reference, capture error, changed resolution or excessive pixel
difference returns nonzero.

Comparison uses decoded RGBA pixels. The default permits a channel delta of
at most 2/255; at most 0.001% of pixels may exceed that delta. An unchanged
build on the current machine produces **zero** changed pixels. The references
depend on the configured game resources, resolution and GPU rendering; a
different installation is an explicit baseline change, not an automatic pass.

To accept an intended visual change:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/regression.ps1 -Update
# Or update only the deliberately changed view:
powershell -NoProfile -ExecutionPolicy Bypass -File tools/regression.ps1 -Spot entrance -Update
```

Only if **all selected captures are valid** are PNG/JSON references replaced.
An existing reference is copied to `reference-before.png` in the run folder.
Commit the changed references with the intended code change and include the
before/after images in its PR. Keep curated verification evidence under
`runs/NNN-slug/`; ordinary regression runs are ignored by Git.

The comparison and metadata checks can be tested without game files:

```powershell
py -3 -m unittest discover -s tools/regression -p 'test*.py' -v
```
