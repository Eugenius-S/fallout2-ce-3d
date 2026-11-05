## Save thumbnail round-trip evidence

`current-3d-vs-classic.jpg` shows slot 01 after a save and reload from the same isolated Klamath startup: 3D save at top, original/classic view at bottom. Both use a 1920x1080 window. Each run reports `SAVE_PREVIEW_FIXTURE PASS` with save and load UI captures; the load screens display the stored slot comment and thumbnail.

The preview box differs between these matched runs (mean RGB absolute delta 21.15, 17.22, 15.53); the surrounding UI is stable. Loaded screenshot metadata is in `3d-final/devshot-20261105-132921-001.json` and `classic-final/devshot-20261105-132932-001.json`; both images are `scr00001.png`. `fixture-result.txt` records the sanitized runtime PASS lines. The classic run exercises the original indexed-framebuffer thumbnail path.

An additional read-only copy of the existing slot 01 was loaded from a scratch root. Its preview bytes are all palette index 0, so that save has no prior image to compare. The supplied screenshot is from another save state and is not a matched baseline. The owner game save was only read and copied to scratch; it was not modified. A valid pre-existing save with a non-empty thumbnail is still needed for the old-preview comparison.

