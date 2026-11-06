---
id: 202
title: The floor extension pass: depth semantics on the fly camera, the quadrant sea fill, marker colours
milestone: M1
status: doing
owner: glm
suggested: glm
priority: high
tier: 3
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

Make the floor extension pass (the sea beyond the map edge) safe to
extend: understand why its quads defeat the depth test on the fly
camera, ship the quadrant sea fill past a water corner on proven
semantics, and let a marker quad inside the extension range keep its
own colour.

## Context

All three are the same pass's flaws, found across 2026-11-05/06:

- The codex P2 on #303 (logged in 168's Log, Find 4): the edge strips
  stay perpendicular to their edges, so the quadrant beyond a water
  corner square gets no quad (sfdock e0 has all four corners
  water-flagged). The attempted fill (the strips' doubling steps, two
  quads a step) blanks the map on the fly camera: quads 2-16 tiles out
  erase the map's own screen area (bisect frames in
  runs/168-codex-finds/), and even ONE 1x1 corner quad renders about
  100x its ground footprint (about 1700 px changed vs about 11
  expected), though the depth test is on and the tiles write depth in
  flight. Reverted; the hull fix shipped alone as #314.
- Marker colours: a quad inside the extension range cannot carry its
  own colour - the pass paints every vertex with the flat sea colour
  (the marker experiment during 168, reverted with the same commit).
- The pass lives in scene_draw.cc (gFloorExtensionFirst/Count, flat
  gFloorSeaColor, flight/turned views only) and DRAWS LAST in the
  floor pass; the tiles write depth in flight (glDepthMask
  flying?1:0), so some state or math of the extension draw does not
  match the tile pass - the projected position of a quadrant quad does
  not match the ground math.

## Acceptance

- A written diagnosis in the Log: what actually happens when an
  extension quad covers the map's screen area on the fly camera, with
  an A/B probe frame in runs/202-floor-extension/ that demonstrates
  the mechanism.
- The quadrant sea fill past a water corner shipped only after the
  probe: sfdock e0's corners get their quadrant, the regression gate
  stays 154/154 (or the moved spots are the corner spots themselves,
  refreshed with the owner's word), and the fly frames show no erased
  map area.
- Marker quads inside the extension range render their own colour in
  both stills and fly frames - OR the Log records why this item moves
  to the first consumer of it.

## Out of scope

The sea rule itself (which squares count as water), the hull (#314),
and any other pass.

## Log

### 2026-11-06 glm
Claimed as the first item of the owner's order: all known 3D-ification
flaws found by glm and gpt go first. The investigation starts from
scene_draw.cc's extension draw state against the tile pass state.

### 2026-11-06 glm: the diagnosis, and it flips the finding
There is no depth-test failure. Same MVP for tiles and the extension,
the tiles write depth in flight (glDepthMask flying?1:0), the sea sits
at y=-0.03, a hair below the tiles, and loses the depth test wherever
tiles are nearer. The 168 bisect frames' "erasure" is the map's own
near tiles seen THROUGH the unfilled quadrant: from outside the map's
corner the strips cover the two edge bands, the quadrant beyond BOTH
edges is empty, and the sight line through that wedge lands on the
map. The stepped wedge edge in the A/B zoom (ab-sliver-zoom.jpg, left)
is the strips' 1-wide columns tiling the gap's boundary. Adding the
fill closes the window - the frame turns to sea, which reads as
erasure at that extreme pose but is the sea correctly standing past
the edge. The "1x1 quad renders 100x its footprint" note was the same
window plus a grazing pose near sea level, not projection gone wrong.
(The exact 168 pose is not reproducible any more: main gained the
map-edge work #293 in between, which moves the game camera the fly
orbit starts from.)

### 2026-11-06 glm: the quadrant fill
The strips' doubling schedule swept along both axes from each water
corner square: a quad per interval pair, each axis's intervals cover
[0, 2048], so the product tiles the quadrant without overlap (about
484 quads a corner, same sea colour, same y). Evidence in
runs/202-floor-extension/ (README.txt): the A/B at the fly pose shows
874 changed pixels total, all in a 245x7 horizon sliver - the gap
closing; the turned game-camera view is sane; ctest 20/20. The full
gate runs next.

### 2026-11-06 glm: the marker-colour item moves
No consumer exists since the #314 revert: the marker quads were wanted
for the hull work, which shipped through the exporter instead. A
capability with no caller would be dead code; the item goes to the
first consumer that needs own-coloured quads inside the extension
range (a candidate: the exit-zone markers if the edge work 145 ever
wants them on the sea).

### 2026-11-06 glm: the codex P1 on #320 moves to 205
Codex's P1 on the PR: no main-versus-change fly-tour evidence for the
fill, and the still gate cannot exercise flight/turned views at all.
The tour run is cut to task 205 (the tool exists, 078; the run and the
standing rule live there). This task's gate re-runs after the earlier
one died with the session restart.
