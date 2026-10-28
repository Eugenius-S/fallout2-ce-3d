# 104: bottom edge

The 3D camera can move one additional 24-pixel map step below the EDG bottom boundary. The Klamath sheet uses the exit area shown in the owner reference; the Grazing Grounds sheet checks another map. At 1920x1080, scale 2, the green exit area moves farther above the interface. A narrow black strip remains above the interface because the existing map clip is unchanged. A 48-pixel trial exposed a larger strip.

The left, right and top stop on Klamath have the same center tile before and after: 25337, 28108 and 20100. The bottom moves from 28108 to 28509 at the Klamath exit and from 24546 to 24947 at Grazing Grounds. Classic bottom stops and decoded RGBA images are identical on both maps (0 changed pixels). Baseline and candidate use the same scene cache, resolution and starting positions.

The comparison images are normal isolated test-run captures with the interface; they are not frozen frames, so the light animation may differ. CTest passed 18/18.

Direct RGBA comparison of the 16 still regression scenes against the clean main build gives 0 changed pixels in every scene, with matching scene metadata. The stored-reference checker reports five failures on both the clean main build and this build with the same counts: KLAGRAZ e0 183 px, KLARATCV e0/e1/e2 3837/5185/6299 px, and KLATOXCV e2 550 px. The references were not changed. Exporter tests passed 66/66 and regression unit tests passed 12/12.

Baseline executable SHA-256: `651e41734455fd370fa94243141448a9e9948bbe010b31c7bba255ce1031b4d3`. Candidate executable SHA-256: `28f38476ba5b722a044d5ced038f61fd45f7b1d822d9ed2658a99eb3eb4fabde`. Raw test captures and logs are in the worktree's ignored build folder.
