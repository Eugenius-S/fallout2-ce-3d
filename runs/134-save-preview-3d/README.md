## Save preview evidence

`current-3d-vs-classic.jpg` compares the same Klamath startup and save slot after two round trips: a 3D save at the top, an original-view save at the bottom. Both use 1920x1080 output, a 960x540 logical buffer, scale 2, and the same executable. The save/load fixtures passed in both modes. The JSON sidecar records the details for each half.

`oldsave-existing-preview.jpg` and `oldsave-existing-preview-detail.jpg` show a pre-existing slot from the archived `runtime-review-20261018` test profile. The visible load card reads Narg, 25 JUL 2241, 0824, Arroyo Temple Entrance, and the prior thumbnail is visible. Its `SAVE.DAT` preview record at offset 131 is 29,792 bytes, with 29,495 non-zero bytes and 84 palette indices. The source profile was only read and copied to the isolated scratch root; the source save was not modified. The fixture passed.

All JPEG evidence is below 1 MB. Full raw captures and the scratch game copy are preserved under ignored `build/134-oldsave-runtime-review-capture-source/` and `build/134-oldsave-runtime-review-game/`.
