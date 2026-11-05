# Task 187 partial evidence

The RPU scratch install exported 19 restored maps into the worktree's
`build/scene-cache`; the cache contains 36 map/elevation scenes. A follow-up
`export.py --check` passed for the entire map list. RP files and generated
scene data stay outside tracked files.

The exporter fixes cover two archive cases found by the first batch: engine
script-helper records with PID -1, and light intensity equal to the engine's
reset floor. Scene-export tests pass 127/127.

Two live 3D walk checks passed on restored RPU maps. Abbey reached tile 18702
from 22703 in 518 frames; San Fran Dr. Sheng reached tile 16100 from 20100 in
542 frames. Both logs keep `scene_view=full` throughout and report the
`scene-gl` renderer. `abbey-walk.jpg` and `sfsheng-walk.jpg` are runtime frames.

The San Fran Dr. Sheng run used a private copy of the existing RPU 2.4.34
scratch install under this worktree's ignored `build/` directory. Its source
scratch copy was left untouched.

`tools/tour/rp_spots.json` now has a center spot for each exported map and
elevation. The tour only selects this list when `--rp-game-root` points to an
install containing `mods/rpu.dat`; without that argument, an RP-only map is
rejected as having no spots. With the explicit root, two smoke runs stalled
at startup before any tour progress (one used an old game executable and one
used the rebuilt executable). The PR stays draft until that runner issue is
understood.
