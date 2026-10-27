# Close-up texture comparison

`closeup.jpg` shows the native cylindrical object from the owner's
`runs/inbox/2026-10-22-fly-closeup-tiles.png`, with its carpet and concrete
wall. Both captures use KLADWTWN elevation 0, tile 26106, a 1920x1080 window,
and the same fly-camera turn 90 degrees, pitch 20, distance 1.5 and lift 0.72.
The camera was reconstructed from the reference; it is not its original pose.
Before and after were shot against the same scene cache, from separate 092
isolated run folders via `dev-snapshot.ps1 -OutputDirectory`.

The game camera remains nearest-neighbour. For fly views only, strongly
magnified texels receive contrast-aware interpolation. Alpha is kept from
the nearest texel so transparency, picking and emissive markers stay intact.
This filters the original art; it does not invent additional texture detail.
