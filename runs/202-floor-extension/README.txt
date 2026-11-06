2026-11-06, task 202: the floor extension pass investigated; the quadrant
sea fill past a water corner shipped; the "depth test defeat" disproved.

The 168 bisect frames (runs/168-codex-finds/) showed extension quads past
a water corner "erasing the map's own screen area", read then as the pass
defeating the depth test. That reading was wrong. The wedges in those
frames are the MAP'S OWN NEAR TILES seen THROUGH the unfilled quadrant:
from a pose outside the map's corner, the sea strips cover the two edge
bands, the quadrant beyond BOTH edges is empty, and the sight line through
that empty wedge lands on the map's tiles. Adding the fill closes the
window - from that pose the frame turns to sea, which looks like erasure
but is the sea correctly standing past the edge. No depth state is wrong:
same MVP, the tiles write depth in flight, the sea at y=-0.03 (a hair
below the tiles) loses the depth test wherever tiles are nearer.

A/B evidence (dev-snapshot, sfdock e0, fly turn 180, distance 8; note the
pose differs from the 168 frames - main gained the map-edge work #293 in
between, which moved the game camera the orbit starts from):

  ab-sliver-zoom.jpg    the 245x7 horizon sliver that changes, 4x zoom:
                        left pre-fill - the tan wedge (the map through the
                        gap, its stepped edge = the strips' 1-wide columns),
                        right post-fill - solid sea. The gap is closed.
  ab-horizon-pair.jpg   the same in frame context (top strip).
  ab-corner0-pair.jpg / ab-dock27701-pair.jpg
                        the full diff bboxes: 874 changed pixels total,
                        all in that horizon sliver - no map erasure at a
                        representative pose.
  corner0-t180-filled-preview.jpg, turned180-postfill-preview.jpg
                        post-fill fly and turned game-camera views: the
                        deck intact, water where water is.

The fill: the strips' doubling schedule swept along both axes from each
water corner square (a quad per interval pair; each axis's intervals cover
[0, 2048], so their product tiles the quadrant without overlap), same sea
colour, same y as the strips. Stills never draw the extension (flight and
turned views only), so the regression gate's game-camera spots are
unaffected.
