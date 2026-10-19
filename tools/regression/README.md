# Regression snapshots

From the repository root, with an existing Release build and exported cache:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/regression.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools/regression.ps1 -Spot entrance
```

The runner takes six independent background snapshots through
`tools/dev-snapshot.ps1`. It never builds, exports resources, starts the owner's
game or sends input to a window. Only its own `fallout2-ce-test.exe` PID is
stopped; the script waits for that process to exit before the next capture.
The test process receives a fixed screen size and scale from `spots.json`. It
forces windowed mode and does not save `fallout2.cfg`, so its result is
independent of the owner's screen settings.
For isolated config checks, `dev-snapshot.ps1 -ConfigDirectory` can read a
separate `fallout2.cfg` and `ddraw.ini` from a `build/task009-*` folder while
keeping the game's resource directory as its working directory.

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
