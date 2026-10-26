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
- 2026-10-26 (seen in 081) `tools/regression.ps1` fails 13 of 16 spots on
  main's own cache (hydrants in corrals, klatoxcv-e1 22777 px, klamall-e0
  3235 px). The references drifted from main; find which merge changed the
  game's view and retake them with a before | after.
