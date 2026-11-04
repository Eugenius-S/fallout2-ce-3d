# Task 188 RP boot check

`tools/rp-check.ps1` ran against the isolated RP scratch install with the
Release executable built from commit `dd4f4102`. It booted and loaded six
maps: `ARTEMPLE`, `KLADWTWN`, `DENRES1`, `ABBEY`, `EPAMAIN1`, and `SLVCAMP`.
The checker verified each returned snapshot's map name. Per-map metadata is
in `results.json`; generated images, configs and saves remain under ignored
`build/` paths.

The negative check used a copy of the built executable with its PE signature
corrupted. The checker failed at process start with exit code 1 and did not
report a pass. The broken copy and raw console output remain in ignored
`build/` paths; the observed result and executable digest are in
`results.json`.

An exploratory sweep over every known map went beyond the required default
smoke set and stopped at `HUBSTASH.MAP`: the game's startup marker appeared,
then the 30-second no-progress watchdog fired before a snapshot. The
watchdog's diagnostic append also hit a locked console log. The six-map
default remains green; callers can pass a wider `-Maps` list to diagnose
additional maps. This attempt is recorded separately and does not claim that
HUBSTASH was verified.

The RP itself was read from an isolated scratch copy outside the checkout.
No RP/game files are included here. Both checks are local runtime evidence;
the PR CI still needs to pass before this task is ready.
