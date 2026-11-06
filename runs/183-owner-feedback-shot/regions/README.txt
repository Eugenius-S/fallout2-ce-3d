183 step 2: the region mark

Two marked shots, each with its clean PNG, the marked PNG and the JSON with
the regions list. The runs used --dev-owner-mark-once/twice (the automated
stand-in for the owner's Ctrl+F9 and drag: it enters the mark mode after the
view settles, drops the fixed rectangles and saves through the same
enterMarkMode/saveMark path the key press drives).

  one-rect/   kladwtwn, one rectangle over the center
              regions: 1 - 83 tiles, 23 squares, 3 objects
              (exit-grid scenery "Сетка выхода" under the rect)
  two-rects/  newr1, the center rectangle and the lower-left one
              regions: 2 - 83/30 tiles, 23/10 squares, 3/0 objects

Each region holds its pixel rect, the hexes whose centers project into it,
the square tiles covered, and the objects under those hexes (prototype id,
name, object type, tile). The marked PNG draws the red outlines on the clean
capture; the clean PNG stays untouched. File names: <stamp>-<map>-e<elev>.png
(clean), the same stem with -marked.png, the JSON holds both names. The
evidence PNGs became full-res JPEGs for the runs/ size limit; the originals
sat in build/test-run/runs/{8162dd66a3dc,c2a344468896}/ (session scratch).

The interactive path differs only in where the rectangles come from: the
owner drags them (mouse events consumed by the mark mode, the game holds:
no scripts, no map transitions, no camera updates), Enter or a second
Ctrl+F9/Alt+F9/F9 saves, Esc cancels.
