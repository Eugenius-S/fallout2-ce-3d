# Inbox: the owner's finds

What the owner notices and sends in chat goes here first, one line each: the
date, what is wrong, where (map, hex, turn), the picture in `runs/inbox/`.
A find is not urgent unless the owner says "now". When an agent takes a task
in that area, it moves the matching lines into that task's Context; a line
that fits no task becomes a new task when the board runs dry.

- 2026-10-21 The 3D scene cuts the map edge farther out than the original
  game (kladwtwn, the bottom edge by the cornfield): the game cuts closer, so
  the wall there would stick out past the edge
  (`runs/inbox/2026-10-21-map-edge-cut.webp`, the red line is where the
  original cuts). Not now. First screenshot the exit zones of the original
  maps to learn where the game cuts a map, then fit the clean edges (016)
  to that.
- 2026-10-21 Dev mode: teleport the hero to any spot of any map (map,
  elevation, hex), and let an agent do it in the owner's running game on the
  owner's word in chat (for example a command file the game watches). Start
  options exist (`--dev-map`, `--dev-elevation`, a view hex); a jump inside a
  running game does not. Not now.
- 2026-10-21 The dev panel's floor change put the hero inside the rock on
  the lowest floor of the toxic caves (klatoxcv elevation 2). The hex check
  (`docs/p1-devtools.md`, "Tile choice on a floor change") looks at the floor
  tile and blockers within 4 hexes, not at whether the hero can walk out.
  Belongs with the teleport line above: pick a hex the hero can walk from (a
  path to the map's entrance or to a few hexes around), or let the owner name
  the hex.
- 2026-10-24 No glows in the owner's live Klamath run. 022 is in main and
  050 (the wide halo) merged on 2026-10-25; Klamath screens are dead art, but
  the burning barrels should glow. Look again in the next build before
  calling it art.
- 2026-10-25 (task 075) The 3D furniture looks gone in the owner's live game: the
  bathhouse shelves read flat (`runs/inbox/2026-10-25-bathhouse-shelves-flat.png`,
  `2026-10-25-bathhouse-interior.png`), though glm's eval shots had them in
  3D; the stove the owner sent glm the day before belongs here too. First
  check whether the owner's build or cache lost the solids (040, 039), or
  whether these things were never covered. Not now.
- 2026-10-25 (task 075) A table in the bathhouse sticks through its walls
  (`runs/inbox/2026-10-25-bathhouse-interior.png`). Not now.
- 2026-10-25 (task 076; 04, 07's holes and 09 in 075; 03, 08, 10 in 081) The owner's live
  review after 071 round 4 and 072
  (`runs/inbox/2026-10-25-review2-01..12.png`). The owner's verdict: fix
  the 3D-fication algorithm itself, no hand-made cases.
  - 01: the STOP corner still meets wrong.
  - 02: a corner of the dark building on the left: a striped strip of
    other material on the corner.
  - 03: a wall shows through another wall (the light patch in the middle
    of the dark wall).
  - 04: a stove fell out through a house wall, and the wall's texture is
    broken around it.
  - 05, 06: the corner of the dark house: a light striped strip down the
    corner.
  - 07: holes where the stove is; another corner with stripes.
  - 08: walls and triangular pieces stick out of the roofs in the 3D view.
  - 09: clothes lines stick out over the roof.
  - 10: a corner of the house is broken (an open door leaf and the
    corner's material).
- 2026-10-25 Dialogue answer options: use a hanging indent. Keep bullet
  markers to the left of the text column, and align every wrapped continuation
  line with the first text character after the bullet, rather than with the
  bullet itself. The text should have a straight left edge independent of
  the markers. Not now; backlog only.
- 2026-10-25 A brick house corner in flight (owner's shot
  `runs/inbox/2026-10-25-corner-wall-floats.png`): the lower stretch of both
  walls near the corner is missing, so the brick bottom floats above the
  street, and a flat brown box (the corner core) stands under it alone; the
  bricks above also look see-through near the edge. Belongs to the corner
  work of 076/083 (`_corner_fits`, `_corner_core`) or to glm's build.
  Task 081: main shows the floating bottom too (wall pictures end above the
  floor); fixed there with feet. The see-through bricks were not seen on
  main; if they stay, they come from 082's `walls.cc` (PR #104).
- 2026-10-25 Two sun options in the [3d] settings, after 082 lands (it
  rewrites `ce/src/scene/sun.cc`): (1) the darkness of building shadows as a
  slider (none on main or in PR #104 today); (2) how far the sun swings from
  overhead, default 30 degrees each way (morning -30, evening +30; this
  replaces 082's 15), adjustable by the player. The owner gave glm these
  instructions for 082 directly; check what 082 lands before cutting a task.
- 2026-10-26 (073, resolved in the 073 branch) The apparently dark
  monitr7 screen and inert slime pool were caused by the test fixture:
  it froze palette cycling, then restored unmapped _cmap entries as zeros.
  Correct cycle.cc rest-phase colors restore both glows. The final comparison
  also reframes an entire slime pool, rather than cropping its edge.
  No new emission marks or game-source repair was needed.
- 2026-10-25 The owner, on the night look: at night nothing changes
  visually; there should be a moon of some kind (his words, for later: a
  moon light at night, presumably a dim cool directional like the sun
  line but weak).
- 2026-10-26 (task 099; found in 090) The point lights' wall normal in
  `sceneLighting` (uLightMode 2) takes `dFdx/dFdy(vWorld)` after the alpha
  `discard`; next to a discarded pixel NVIDIA gives zero derivatives and the
  normal turns NaN. 090 fixed it for the sun only; the same fix in the
  point lights changes 179 to 1155 px in the caves (klaratcv, klatoxcv),
  so it needs its own task with a look check there.
- 2026-10-27 (093 leftover 4, task 100) The owner reproduced the thing poking
  out of the bathhouse wall in the live 3D game:
  `runs/inbox/2026-10-27-shadow-leftover-4b-bathhouse.jpg`. A grey box
  sticks out of the bathhouse's west wall (the long wall facing the alley),
  a few hexes up from its south corner, the hero standing at that corner.
  Inside (`2026-10-27-shadow-leftover-4c-bathhouse-inside.jpg`, roof off) the
  owner names it: a table with junk on it inside the bathhouse, its picture
  crossing the wall. So it is a scene object, not the painted crates 093
  compared. From above (`-4d-bathhouse-top.jpg`, fly view) the junk is not on
  the table at all: the box, plate and cans sit against the alley wall, and
  two grey pipes lie outside it in the alley. The items are placed away from
  their table, through the wall.
- 2026-10-27 (task 102) Inside the bathhouse, roof off, in flight (owner's shot
  `runs/inbox/2026-10-27-bathhouse-inside-light-strips.jpg`): the inner face of
  the far wall shows vertical strips of bright specks, the window columns of the
  wall's outside picture lit as if daylight came through. The owner reads them
  as stray strips of light. Check what the inside face of a wall shows (its own
  art, a flat dark face, or the outside picture mirrored) against the original.
- 2026-10-27 (096) The bathhouse's south corner in the 3D view, roof off (owner's shot
  `runs/inbox/2026-10-27-bathhouse-corner-piece-gone.jpg`): the corner piece
  (the striped pillar seen at that corner in `2026-10-27-shadow-leftover-4b-bathhouse.jpg`)
  is gone; the corner is a plain grey box. The owner: the corner element fell off.
  In the isometric camera of the same 3D view the striped pillar is in place
  (owner's shot in chat, not on disk); only the turned perspective camera
  loses it. Likely 096's corner pieces.
- 2026-10-27 (096) A house corner from close by in flight (owner's shot in chat,
  not on disk): the shadow on one wall runs on past the corner onto the next
  face while the corner piece with its striped pillar stays lit. The owner: the
  shadow flowed round the corner, ignoring the corner piece. One more case of
  096's class; check it there once 096 lands.
- 2026-10-27 (task 101) Roof horns, fewer than before but still there (owner's shot
  `runs/inbox/2026-10-27-roof-horns.jpg`, flight over Klamath with the 095
  build): dark wall-top strips stand up through the big brick building's roof
  (a broken dashed line across it, a black triangle at its right edge) and
  through the roof left of the bathhouse. Walls poking above the roof they sit
  under; follow-up of the 081 roofs work.
- 2026-10-27 (task 103) The see-through halo round the hero is gone (owner's shot
  `runs/inbox/2026-10-27-hero-see-through-halo-gone.jpg`, the 095 build, inside
  the bathhouse by its south wall): the wall in front of the hero is drawn solid,
  with no cut-out round him. Find which change lost it (git bisect between the
  last build where the owner saw it and 095).
- 2026-10-27 (096) More corner shadows from the owner's live run: at Vic's
  house the shadow flows round the corner too
  (`runs/inbox/2026-10-27-vic-house-shadow-round-corner.jpg`), and the bottom-edge
  shot `runs/inbox/2026-10-27-bottom-edge-exit-zone.jpg` has a shadow flaw by the
  house (the owner did not mark which; compare it with the original view).
- 2026-10-27 (tasks 101-104) The owner's later shots in `runs/inbox/2026-10-27-*`:
  roof ends and fringe (101), walls from inside and holes (102), the halo on
  walls (103), the camera's bottom edge (104).
- KLAGRAZ e0 still differs from the stored regression reference by 132 px; clean 098 baseline has the same deviation.
- 2026-10-27 The 4:3 full-screen art (main menu, splash and loading screens,
  death and ending slides, the like) extended to 16:9 and 21:9. The owner's
  method, to extend the originals rather than replace them: regenerate the art
  wider from the original, downscale it to the game's pixel size, add dithering
  to match, then lay the untouched original over the middle with a small mask
  at its edges (a gradient to 0) so the original blends into the extension.
  Not now; a note only.
- 2026-10-27 A list of the unique objects that will need their own 3D model,
  because the generic wall, roof and box rules cannot shape them: the big
  stone head in Arroyo, the Vault City gate, the Reno signs, the vertibirds,
  and the like (other one-off landmarks, vehicles and large signs). Go
  through the maps and the prototypes once, name each object with its map and
  shot, and order them by how much a player sees them. Not now; a note only.
- 2026-10-28 The owner's live review on main `9705aca4`, Klamath
  (`runs/inbox/2026-10-28-*`): the STOP corner seam and a dark house's
  corner seam (110); the trough and the sandwich-board sign dark whole with
  no shadow on them, corner pieces shaded whole (109); the hero's cut-out
  only on roofs, not walls (103); no glow on the slime pools and no
  visible point lights in the toxic caves (111). The bottom stop (108)
  passed with no remarks.
- 2026-10-28 A sky in the 3D view (flight and turned camera), like the
  first-person port https://github.com/Apolotary/fallout2-first-person
  (`runs/inbox/2026-10-28-sky-reference.webp`: a dusk gradient sky above
  the walls instead of black). The owner: the 3D view looks better with
  it. Could follow the time of day (sun, night look). The donor's sky is one
  full-screen shader (`viewer/render.js`, `SKY_FS` and `MOODS`: horizon
  haze, a gradient to the zenith, the sun's glow, stars at night, two far
  mesa ridges), drawn first from the camera's ray. Not now; a note only.
