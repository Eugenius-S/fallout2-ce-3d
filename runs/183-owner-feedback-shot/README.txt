183: the owner feedback shot, step 1 (the plain shot)

Three shots on three maps, each the frame as the game presents it in the 3D
view (scene + the game's own picture over it) with the JSON sidecar next to
it. Taken by --dev-owner-shot-once (the pump stands in for the F9 key press;
the key path is the same takeShot()).

  kladwtwn/  Klamath downtown, view tile 25075   capture frame_ms=9.62
  newr1/     New Reno street 1, entrance         capture frame_ms=12.63
  vctyctyd/  Vault City courtyard, entrance      capture frame_ms=11.20

Frame-time bar (acceptance: no frame over 50 ms at the key press): each
folder's devperf.log carries the `perf owner_shot capture frame_ms=...` line
(the wall time the capture added to the frame that took it: the GL readback
plus the JSON build; the PNG encode and the file writes run on a worker
thread). The steady frame windows around it read frame_avg_ms=8.9 to 22.8;
capture plus a steady frame stays under 50 ms with margin.

newr1 and vctyctyd ran with -OwnerShotDelay 6 and -HoldSeconds 12 (steady
frames before the capture). vctyctyd leaves the full view right after the
load (the city's game state takes over), so it ran with no delay and the
capture sits at the first ready present.

The JSON string fields keep the game's own encoding (cp1251 on this Russian
install): "map": "Центр города", "city": "Кламат".
