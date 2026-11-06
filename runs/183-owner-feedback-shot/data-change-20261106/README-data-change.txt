2026-11-06 ~18:24 gate on the 183 branch tip (75e23f7a): 132 pass, 22 fail,
all 22 from one rendering change: PR #293 (task 145, "Match the original
map edge", merged 09:46 that morning).

The PR changed how the 3D view renders the map edge and hid the exit-grid
marker cards, and its own reference refresh stopped short (the PR marks
task 145 blocked after two tour timeouts). This folder's 22 spots - outdoor
encounter maps (city gates, mountain views, the special scenes) - were not
in that refresh: their dark map-edge band renders differently now, and
every other pixel in every frame is identical. The morning's 5-pixel
cave-wall strip (enc-cave1/enc-cave5/enc-cave5-top, x=598 y=207..215) is
the same PR's change seen on cave maps; those references were refreshed
first, these 22 second. An earlier note blaming a game-data rebuild was
wrong: the game's files never changed that day (master.dat and the mods/
archives keep their old dates; the export fingerprint moved because the
map-edge code changed which resources the exporter reads).

Evidence in this folder:
  data-change-sheets-1.jpg / data-change-sheets-2.jpg
      per-spot crops, reference (pre-#293) left, new capture right: the
      edge band is the only difference in all 22 spots.
  enc-mountn1 (and every other spot folder): reference.png (pre-#293),
      devhero-*.png (new capture), difference.png, highlight.png.

The captures are stable: a second full pass an hour later reproduced every
capture in the pixel comparison (148 of 148 common spots identical), and
the pre-merge and post-merge builds of the 183 branch render identically,
so none of this comes from the 183 work.

Action: references for the 22 spots refreshed against the post-#293 render
(-Update, one spot per run), then a fresh full gate (153/154 in-run plus
one black-frame capture flake passing its single-spot rerun with 0 changed
pixels).
