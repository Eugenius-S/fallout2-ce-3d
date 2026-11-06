# Route attempt 3

2026-11-06, RPU 2.4.34 isolated scratch copy. `origin/main` was
`b2f792de`. The cache was exported to the worktree's `build/scene-cache`;
the source scratch copy was read only. No files in `game/` or `game/mod-cache`
were used as output.

The previous watchdog log named missing `floor.bin` at SLVCAMP. Before this
attempt, elevation 0 was exported for the five base route maps and all 19
RPU restored maps. All 23 elevation-0 `floor.bin` files were present.

The bounded route started at ARTEMPLE and requested ARVILLAG, KLADWTWN,
DENBUS1, DENRES1, ABBASEM, SLVCAMP, EPAMAIN1 and HUBSTASH. It produced seven
static wall-plus-hero preview snapshots through SLVCAMP. The sidecars report
`scene_view=full`; they do not represent live fly-camera frames. The process
then stayed at EPAMAIN1 with `isoDisabled=1` and no snapshot until the
180-second timeout. This is not a completed RP route and provides no dynamic
sfall fire counts or impact classifications.

Raw process logs, game saves and captures remain in the ignored
`build/rp-gap-run-3/` folder. The attempt is recorded in the task Log; the PR
remains draft.
