# Task 187 partial evidence

The RPU scratch install exported 19 restored maps into the worktree's
`build/scene-cache`; the cache contains 36 map/elevation scenes. A follow-up
`export.py --check` passed for the entire map list. RP files and generated
scene data stay outside tracked files.

The exporter fixes cover two archive cases found by the first batch: engine
script-helper records with PID -1, and light intensity equal to the engine's
reset floor. Scene-export tests pass 127/127.

One live 3D walk check passed on Abbey: start tile 22703, destination 18702,
518 frames, `scene-gl` / `scene_view=full`; the log and sidecar show no view
fallback during the walk. `abbey-walk.jpg` is the real runtime frame. A second
restored area and RP-only fly-tour spots remain to be checked.
