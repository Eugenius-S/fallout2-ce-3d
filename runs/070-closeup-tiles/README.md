# Close-up texture comparison

`closeup.jpg` shows the native cylindrical object from the owner's
`runs/inbox/2026-10-22-fly-closeup-tiles.png`, with its carpet and concrete
wall. Both captures use KLADWTWN elevation 0, tile 26106, a 1920x1080 window,
and the same fly-camera turn 90 degrees, pitch 20, distance 1.5 and lift 0.72.
The camera was reconstructed from the reference; it is not its original pose.
Before and after were shot against the same scene cache, from separate 092
isolated run folders via `dev-snapshot.ps1 -OutputDirectory`.
The before build is main 4a3b54fa plus only the fly-lift capture helper
(SHA-256 dc687b2bf558f21ede9772ef37f81a0142ab62adf101856471f6580a17415132).
The after build includes the texture filter
(SHA-256 6adf34f73c7e45a07e1c71003b1a8a3940608ec342783132751153751c567aa8).

The game camera remains nearest-neighbour. For fly views only, strongly
magnified texels receive contrast-aware interpolation. Alpha is kept from
the nearest texel so transparency, picking and emissive markers stay intact.
This filters the original art; it does not invent additional texture detail.
