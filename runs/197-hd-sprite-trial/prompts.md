# Prompts

Character source: `hmjmps`, Fallout 2's default male jumpsuit model (`ce/src/art.cc`). The source sheet has six standing directions (first frame from `hmjmpsaa.frm`) and all eight unarmed walk frames in direction 0 (`hmjmpsab.frm`). The source frames were placed on 4x4 transparent cells using the original FRM direction shifts and per-frame offsets.

## x4 generation

Input: the source sheet enlarged 4x with nearest-neighbor. Imagegen returned an 881x1785 PNG; the comparison sheet is resampled to the exact 4x source dimensions (656x1328).

> Edit this exact Fallout 2 sprite sheet as a conservative AI HD remaster trial. Preserve the 4 columns by 4 rows, all 14 occupied cells in the same order and position, their transparent spacing, each sprite's original scale relative to its cell, and every foot and animation anchor. The first six cells are the six standing directions of the default male blue jumpsuit character; the remaining eight cells are consecutive unarmed walking frames facing the same direction. Keep the original character identity, blue suit, yellow vault emblem, skin and hair colors, anatomy, silhouette, pose, viewing angle, and light direction. Make only subtle cleanups to pixel clusters and jagged edges so each sprite reads more clearly at four-times resolution while still looking like hand-made classic pixel art. No painterly texture, blur, gradients, extra outlines, new details, props, shadows, captions, borders, repositioning, merging, or missing frames. Keep transparent background and the same sheet composition.

Output: `generated-x4.png`. Imagegen request id: `exec-0e89aade-2c0b-42a1-806c-c658f90c596e`.

## x2 generation

Input: the source sheet at x1. Imagegen returned an 881x1784 PNG; the comparison sheet is resampled to the exact 2x source dimensions (328x664).

> Create the x2 candidate of this 1998-era Fallout 2 pixel-art sprite sheet. This must remain unmistakably pixel art, not a painting or 3D illustration: preserve visible square pixel clusters, hard pixel edges, limited original palette, and the original low-resolution look. Use the provided sheet as exact source. Keep the same 4-column by 4-row grid, all 14 occupied frames in the same order, no added or missing sprites, and each sprite's exact pose, silhouette, outfit, direction, and foot anchor. First six cells are the default male blue Vault jumpsuit standing in six directions; the next eight are the unarmed walking cycle in direction 0. Keep the yellow Vault 13 marking recognizable. Add no gradients, anti-aliasing, soft shading, painterly texture, realistic fabric, outline, background, captions, borders, or props. Make each source pixel resolve as a clean two-by-two block with only sparse subpixel cleanup where necessary. Preserve transparent background and sheet layout exactly.

Output: `generated-x2.png`. Imagegen request id: `exec-d32e5277-6e56-4b19-8f21-a4aa080ecd29`.

## x1 comparison

`comparison-x1.jpg` uses the x2 generation downsampled to the source cell dimensions. All three JPEGs place the original sheet beside the generated sheet at the labelled scale. The underlying generated PNGs are the resampled x2/x4 artifacts, not game assets.
