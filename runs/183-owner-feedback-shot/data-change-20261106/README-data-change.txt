2026-11-06 ~18:24 gate on the 183 branch tip (75e23f7a): 132 pass, 22 fail,
all 22 from one game-data change.

The owner rebuilt their dat files a second time that day (the first rebuild,
11:56, moved a 5-pixel cave-wall strip in enc-cave1/enc-cave5/enc-cave5-top;
those references were already refreshed). This second change (made between
12:37 and 17:26 machine time) removed a translucent amber wash overlay that
the old data drew over parts of outdoor encounter maps: city gates, mountain
views, and the special-encounter scenes. Everything else in every frame is
pixel-identical.

Evidence in this folder:
  data-change-sheets-1.jpg / data-change-sheets-2.jpg
      per-spot crops, reference (old data) left, new capture right: the
      wedge-shaped wash is the only difference in all 22 spots.
  enc-mountn1 (and every other spot folder): reference.png (old data),
      devhero-*.png (new capture), difference.png, highlight.png.

The captures are stable: a second full pass an hour later reproduced every
capture byte-for-byte in the pixel comparison (148 of 148 common spots
identical), and the pre-merge and post-merge builds of the 183 branch render
identically, so none of this comes from the branch.

Action: references for the 22 spots refreshed against the current data
(-Update, one spot per run), then a fresh full gate.
