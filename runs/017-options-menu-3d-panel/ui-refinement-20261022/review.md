# UI polish review

| Severity | Location | Before | After | Why | Evidence |
|---|---|---|---|---|---|
| Fixed | Frame sides | Noisy vertical source sampling | Clean native 5-row strip repeats vertically | Keeps the Pip-Boy bezel continuous | 0 pixel mismatches across 25,600 period checks per side |
| Fixed | Panel grid | Content sat 5px left of frame axis; controls had inconsistent right edges | Labels and right-aligned controls share a grid centered at local x=320 | Aligns legacy page and 3D panel in the same 640px container | At logical width 960, pageX=160 and screen center=480 |
| Fixed | 3D settings | Registry-only row list | General, Lighting, Camera, and automatic Other; registration order stays stable within groups | Gives related settings clear chunks and keeps future keys visible | 10/10 current descriptors covered once; row spans are disjoint |
| Fixed | Rows and sliders | Uniform 44px rows and distant slider tracks | Toggle/value rows 28px; slider rows 40px; track offset 22px with shared ±7px hit tolerance | Keeps labels and controls together while preserving accurate hit regions | Last row ends y=1002; frame ends y=1034; 32px clearance |
| Fixed | Selected control | 4x39px bar at row edge | Compact control outline; muted-yellow text remains for disabled settings | Shows focus at the selected control and separates disabled text | Heading, normal control, and focus RGB(60,248,0) on RGB(40,40,40): 10.26:1 |
| Medium | 640px logical scrollbar | No external margin available | Track remains at x=628..639 over the outer bezel; row hitboxes end at x=597 | Track avoids controls, but covers 12px of the 32px right bezel | Minimum-bottom capture `minimum-bottom/scr00028.png`; wide track x=804..815 is outside the frame |

Verdict: Approve for this bounded patch.

Task 017 remains in progress. Resolve the 640px scrollbar/bezel overlap after the separately paused interaction and persistence probe.

Not verified: disabled-setting contrast (no disabled row in the captures), Tab navigation, logical widths below 640px, and the interaction/persistence probe. The capture helper enforces a 640x480 logical minimum; the menu currently supports arrow-key scrolling and editing without Tab navigation.

Build: Release succeeded. CTest: 12/12 passed. Final wide-bottom capture SHA matches the Release executable.
