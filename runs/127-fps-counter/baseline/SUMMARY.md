# 127 frame-time baselines (2026-11-07, in-play 30 s windows)

Run: fresh branch build, owner demo copy as the game root, the worktree
scene cache, warm pool on. The load frames are cut from each CSV; the
window is the stable in-play tail (about 30 s).

| run | frames | average fps | 1% low fps | 1% low ms | worst ms |
|---|---|---|---|---|---|
| kladwtwn-e0-flycam | 1794 | 59.4 | 20.8 | 48.0 | 52.0 |
| kladwtwn-e0-gamecam | 1709 | 58.5 | 20.8 | 48.0 | 86.0 |
| klamall-e0-flycam | 1806 | 59.5 | 20.8 | 48.0 | 81.0 |
| klamall-e0-gamecam | 1726 | 58.8 | 20.8 | 48.0 | 50.0 |
| newr1-e0-flycam | 451 | 25.2 | 12.5 | 80.0 | 98.0 |
| newr1-e0-gamecam | 630 | 32.6 | 16.9 | 59.0 | 97.0 |
| vctyctyd-e0-flycam | 1801 | 59.5 | 20.8 | 48.0 | 87.0 |
| vctyctyd-e0-gamecam | 1809 | 59.4 | 20.8 | 48.0 | 92.0 |
