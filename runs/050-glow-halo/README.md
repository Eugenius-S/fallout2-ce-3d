# Task 050: the glow halo

Every pair is the same fixed view twice: the glow on (`--dev-glow`), then
off. Each folder holds both full frames and `onoff.png`, the pair side by
side (off left, on right).

- `cave/`: the toxic caves (klatoxcv e1, view 19091), the slime pools spill
  a wide green halo over the rock around them.
- `vault-screens/`: the starting vault (vault15 e0, view 13475), the live
  blue monitors bleed over their housings and the wall. The map was added
  as the `vault15` group in `export.py`: its screens (monitr, bvs, comp
  arts) carry the cycling colours Klamath's dead screens have none of.
- `torch/`: the mall at night (klamall e0, view 21700, ambient 16384), a
  fire barrel glows red over the room around it.
- `night-fire/`: the town campfire at night (kladwtwn e0, view 22958,
  ambient 16384): the embers are the only fire art in Klamath (no burning
  barrel exists in this game's art outside the mall one above).
- `town/`: the daylit town (kladwtwn e0, view 25075) stays clean: the wide
  halo feeds only from the marked texels (the emissive buffer's alpha), so
  daylight sand and walls do not haze.

Owner files: `owner-wanted-look.webp` (the reference spill), the `owner-*`
"now" shots.
