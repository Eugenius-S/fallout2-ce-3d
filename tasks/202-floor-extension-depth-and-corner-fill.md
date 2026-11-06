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

- A written diagnosis in the Log: the exact state difference (depth
  mask, matrix, winding, clip) or math error that makes an extension
  quad overdraw nearer tiles on the fly camera, with a probe frame in
  runs/202-floor-extension/ that demonstrates the mechanism (a probe
  whose pixels land where the diagnosis says).
- The quadrant sea fill past a water corner shipped only after the
  probe: sfdock e0's corners get their quadrant, the regression gate
  stays 154/154 (or the moved spots are the corner spots themselves,
  refreshed with the owner's word), and the fly frames show no erased
  map area.
- Marker quads inside the extension range render their own colour in
  both stills and fly frames.

## Out of scope

The sea rule itself (which squares count as water), the hull (#314),
and any other pass.

## Log

### 2026-11-06 glm
Claimed as the first item of the owner's order: all known 3D-ification
flaws found by glm and gpt go first. The investigation starts from
scene_draw.cc's extension draw state against the tile pass state.
