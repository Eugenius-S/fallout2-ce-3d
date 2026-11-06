183 steps 3 and 4: the teleport and the walkable landing

The teleport: dev-teleport.txt next to the exe, one line "map.elev" (the
safe pick decides the hex) or "map.elev.hex" (the owner's named hex stands
as it is). The game polls the file about twice a second, jumps through the
engine's own safe transition, puts the hero down and brings the 3D view
back. The dev panel's TP button and the T key (panel open) drive the same
engine; the filter's digits name the hex there.

A map's scripts can hold the game in combat/dialog/movie: the request is
held, not eaten - the file survives until the game can take it.

  jump-to-klatoxcv-e2/   from kladwtwn, file "klatoxcv.2": the safe pick
              lands on 18930 (the JSON's player.tile), the 3D view back
  jump-to-newr1-named-hex/  from klatoxcv e2, file "newr1.0.24305": the
              named hex stands verbatim (watchdog: NEWR1.MAP e0 24305)
  log-floor-change-klatoxcv-e2.txt  the floor change (step 4): from
              klatoxcv e0, file "klatoxcv.2" is a same-map elevation
              change, so it goes through the floor change's own pick:
              "devtools: elevation 2 lands on 18931, 1 candidates tried"
              - the candidate passed the walk check (a path to a free
              neighbor), so the landing is not a rock pocket. On klatoxcv
              the map entrance sits on another elevation, so the "path to
              the entrance" criterion degrades to that walk-out check.

The old floor-change bug (the hero inside the rock on klatoxcv e2) came
from the pick testing floor and blockers only. The pick now also requires
that the hero can walk from the landing: a path to the entrance, to a few
hexes around it, or - on floors without their entrance - a path to a free
neighbor.

The evidence pictures are full-res (kept under the runs/ size limit, the
newr1 one a JPEG rendition, its JSON's image name points at the PNG the
feature wrote); the logs are the runs' process.stdout.log with the teleport
and floor-change lines.
