# Slider tracks align with their labels

The track and knob now share the vertical center of the lowercase letter
body. The previous fixed row + 22 offset put them below the labels and values.
Labels and numeric values keep their original row + 4 origin and spacing.

For pixel text, an off-screen font-101 `x` probe measures its ink rows without
recording extra interface text. For sharp text, a small renderer metrics
helper uses the current weight, preview size, baseline rounding and rasterized
`x` glyph. Drawing, dragging and click hit testing share `sliderTrackY`.

## Captures

- `first-slider-before-after.jpg`: before above, after below, ordinary text.
- `options-russian.jpg` and `options-english.jpg`: the complete custom panel.
- `options-russian-sharp.jpg`: the panel with sharp text active and drawn.
- `verification.json`: captures, hashes and pixel measurements. All five
  ordinary-text tracks moved up 28 physical pixels at scale 2 (14 logical
  pixels). In sharp mode the first track is centered within 0.5 physical
  pixels of the lowercase body in the label. Label and value crops are
  pixel-identical before/after in both modes.

Raw captures are 1920x2160 at scale 2; panel JPEGs are crops, not rescaled.
The first-slider comparison is cropped from the same raw before/after frames.

## Verification

- Release build and all 15 CTests pass. The embedded-font test compares the
  measured x-height center with actually rendered glyph ink for both weights,
  all five preview sizes and output scales 1 through 4 (40 combinations).
- 55 exporter tests and 12 regression unit tests pass.
- Both pixel and sharp-text background mouse probes drag the zoom slider,
  then cancel successfully. Their config hashes before/after are identical:
  `mouse-cancel-pixel.json` and `mouse-cancel-sharp.json`.
- All 16 world regression scenes match the current-base controls from 066
  with 0 changed pixels. `regression-summary.json` records the results.
  Committed references were not changed; their independent drift is task 058.

Reproduce with `tools/dev-snapshot.ps1 -Preferences -PreferencesPosition Bottom
-ScreenWidth 1920 -ScreenHeight 2160 -Scale 2 -Language russian -SharpText
-CaptureConsole -OutputDirectory build/slider-check`, then the same command
without `-SharpText` or with `-Language english`.

No font assets or game settings were changed, and the owner's running game
was not replaced, focused or restarted.
