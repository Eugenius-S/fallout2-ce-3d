## Log

- Folded Task 107's painted corner-post rule into this task. Added geometry detection that splits a terminal post strip at its axis and places it on the perpendicular face. Added exporter tests for row detection, owner matching, and split geometry.
- Full regression found intended painted-post changes at Broken Hills entrance (76 pixels) and jail (60 pixels). Updated only those two references and reran both spots; each passed with zero changed pixels. See `runs/189-batch-walls/brokenhills-post-regression-diffs.jpg`.
- Matched current-main captures show 104 changed pixels at Modinn tile 19531. The same-owner candidates at Kladwtwn tile 25874 and GSTFARM tile 12439 show zero changed pixels in both tested views. The exact folded acceptance view at bathhouse tile 18930, zoom 2.5, turn 40, hours 8 and 17 also shows zero changed pixels. See the three-map, bathhouse, and Modinn sheets with JSON sidecars.
- The 24-view fly-tour control set reports zero changed control views. See `runs/189-batch-walls/fly-tour-changed.jpg`.
- Build succeeded. CTest: 20/20; exporter tests: 128/128; regression unit tests: 12/12. Full regression passed after updating and rerunning the two Broken Hills references.
- Acceptance remains open: the expected visible post effect is not present in the exact bathhouse acceptance view or the tested Kladwtwn/GSTFARM candidates. Modinn changes are visible, but this does not establish the rule across maps. PR remains draft. Next step is to diagnose visibility/face selection for these locations before claiming completion.
