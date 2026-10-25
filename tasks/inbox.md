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
- 2026-10-22 Some walls have no back side again: seen from behind in the
  fly camera they vanish. The owner remembers every wall had one before.
  Same visit as the close-up noise above; look for a lost back face or
  culling change in the walls since then. Not now.
- 2026-10-24 The owner's live review of the 046 build (his screenshots:
  runs/046-characters-in-shadow/review-2026-10-24/): the 3D view has holes
  in walls; close them. May share a root with the missing back sides line
  above.
- 2026-10-24 Same review: defects at house corners; the owner wants one
  universal fix for all corners, not spot patches.
- 2026-10-24 Same review: the computer stands must get real 3D too.
- 2026-10-24 Same review: a straight card cutout disappeared somewhere; it
  was good before (his words; which card is unconfirmed, asked in chat).
- 2026-10-24 Same review: no glows in his live Klamath run. 022 and 050
  are in main and [3d] glow defaults on; Klamath screens are dead art, but
  the burning barrels should glow, so check the fire glow live before
  calling it art.
- 2026-10-24 Same review (shots 25..35 in
  runs/046-characters-in-shadow/review-2026-10-24/): a cabinet draws in
  front of a person standing before it; the z order of the two cards is
  wrong.
- 2026-10-24 Same review: the notice standee (the A-frame board) has the
  wrong shape in 3D.
- 2026-10-24 Same review: the two buckets overlap; merge them into one
  read.
- 2026-10-24 Same review: a floor tile that was converted to 3D before now
  shows as 2D again; find out why it dropped out.
- 2026-10-24 Same review: the fence corner's geometry is broken and the
  shadows expose it (the corners line above is the walls side of it).
- 2026-10-24 Same chat (round 7): the holes in the shack wall by the
  barrels may be fine, the owner says, it is a shack after all (the walls
  holes line above; low priority unless he says otherwise).
- 2026-10-24 Same chat: pin the doors to the 3D wall planes. The owner
  says he already tasked claude with it; no task file on the board carries
  it, so claude claims it or it becomes a task when the board runs dry
  (the PLANE mechanism exists, 037).
- 2026-10-24 Around the dialogue UI, turn the free screen area black so the street world is hidden. Reference: `runs/inbox/2026-10-24-dialog-black-background.png` (owner source: `C:\Users\jiraf\AppData\Local\Temp\codex-clipboard-3865ace3-a630-45ed-b714-034d45600988.png`). Not now.

- 2026-10-24 Russian custom menus still use English labels while the game language is Russian. Review menu localization and match the active UI language. Not now.
