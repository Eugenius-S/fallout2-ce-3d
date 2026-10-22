# Inbox: the owner's finds

What the owner notices and sends in chat goes here first, one line each: the
date, what is wrong, where (map, hex, turn), the picture in `runs/inbox/`.
A find is not urgent unless the owner says "now". When an agent takes a task
in that area, it moves the matching lines into that task's Context; a line
that fits no task becomes a new task when the board runs dry.

- 2026-10-20 Vic's shack on the Klamath trapper map breaks up from a turned
  view (`runs/039-shapes-klamath/owner-vic-shack.png`): in 039.
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
- 2026-10-22 Up close in the fly camera the tiles look like raw pixel noise
  again (`runs/inbox/2026-10-22-fly-closeup-tiles.png`: a round tank with
  its cap, the carpet floor and the wall beside it). The owner remembers
  these tiles were already made nearly clean. First find out whether an
  earlier fix (texture filtering, mip or upscale of tiles and shapes) was
  lost, is off in the owner's config, or never covered this case. Not now.
