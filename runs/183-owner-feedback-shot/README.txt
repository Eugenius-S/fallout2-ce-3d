183: the owner feedback shot, step 1 (the plain shot)

Three shots on three maps, each the frame as the game presents it in the 3D
view (scene + the game's own picture over it) with the JSON sidecar next to
it. Taken by --dev-owner-shot-once (the pump stands in for the F9 key press;
the key path is the same takeShot()).

The feature writes PNGs; runs/ keeps them under 1 MB a file, so the evidence
copies here are full-res JPEG renditions (quality 92). The PNG originals sit
in the run folders of the capture session, build/test-run/runs/<run id>/:
kladwtwn 332624f94504, newr1 dae44a166d31, vctyctyd a8dffeaf6491 (run
folders are session scratch; game/shots/dev/ keeps the owner's originals).

  kladwtwn/  Klamath downtown, view tile 25075   capture frame_ms=9.62
  newr1/     New Reno street 1, entrance         capture frame_ms=12.63
  vctyctyd/  Vault City courtyard, entrance      capture frame_ms=11.20

Frame-time bar (acceptance: no frame over 50 ms at the key press): each
folder's devperf.log carries the `perf owner_shot capture frame_ms=...` line
(the wall time the capture added to the frame that took it: the GL readback
plus the JSON build; the PNG encode and the file writes run on a worker
thread). Around it the 2 s windows read frame_avg_ms 17.2 to 35.3 (a
background run under F2CE_PERF=1; the owner's game carries neither
overhead), so the capture frame lands at about 27 ms on kladwtwn
(17.6 avg + 9.62) and 42 ms on newr1 (29.0 + 12.63), both under the bar.
vctyctyd's capture sits at the first ready present (the map leaves the full
view right after its load), so its 11.20 ms is the capture cost alone.

newr1 and vctyctyd ran with -OwnerShotDelay 6 and -HoldSeconds 12 (steady
frames before the capture). vctyctyd leaves the full view right after the
load (the city's game state takes over), so it ran with no delay and the
capture sits at the first ready present.

The JSON string fields keep the game's own encoding (cp1251 on this Russian
install): "map": "Центр города", "city": "Кламат".
