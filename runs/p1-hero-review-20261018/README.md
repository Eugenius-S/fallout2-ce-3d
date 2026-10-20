# Hero-card preview review, 2026-10-18

Accepted scope: the current CE player frame rendered as one upright card in the accepted Klamath wall scene. Camera angle, scale and target remain unchanged. P1 is still incomplete: picking, interactive doors, NPCs, UI overlay, camera follow and full donor row-plane occlusion remain outstanding. Walking/animation continuity was not established by this runtime check.

Executable SHA-256: `87d1827ce4d85a25a9b54958d3f9cb1af5e7f88be128120a435632ca28436e20`.
CE base `f6c1969`, outer base `b647b1e`; source changes remain uncommitted.

## Independent checks

Release build completed, including CE and mapper; incremental independent rebuild succeeded. All 25 exporter tests passed. CE diff check passed.

The reviewer launched the exact executable in `game/runtime-review-20261018`, inspected the real window through Computer Use, and inspected the matching GL PNG.

| Check | Result | Evidence |
| --- | --- | --- |
| Actual player visible in wall scene | PASS | `devhero-20261018-152759-000.png`, matching JSON, real window inspected |
| Native world anchor agrees with independent coordinate calculation | PASS | tile `25075`, GL ground anchor `(-12.541666667, 12.6875)` |
| Approved camera preserved exactly | PASS | Camera JSON string equals accepted wall-preview camera |
| Exact running EXE recorded | PASS | Snapshot SHA matches independently hashed sandbox executable |
| Return to ordinary game | PASS | After 1,800 updates; `hero-review-stdout.log`, vanilla PNG referenced by `devshot-20261018-152831.json`, real vanilla window inspected |
| Invalid projection rejected before preview | PASS | First squ value changed to `1e9` in sandbox only; `hero-badprojection-stdout.log`, ordinary game remained visible |
| Failed projection produces no false hero screenshot | PASS | Hero image count remained one; one-shot snapshot explicitly skipped |

The projection cache was restored after the negative test and its hash checked against the exported source cache. Current exporter provenance and the exact `projection.bin` are included here.

## Review fixes

- Movement offsets enter the world anchor once; artwork offsets determine the card shape separately.
- Odd frame widths use CE's integer half-width for the left edge.
- Live FRM/palette data is copied immediately before each presentation, including nested presentations; no Object or Art pointer survives the call.
- All six projection coordinate components are bounded, and invalid hero/projection data cannot produce a successful hero snapshot.

The frame is copied with `FrmImage` ownership and rendered through the existing common depth buffer. Full donor row-plane depth adjustment is not implemented or accepted here. Ordinary world clicks still belong to the vanilla projection; this preview is not an accepted interactive scene.

Run from the prepared game working directory with:

```text
fallout2-ce.exe --dev-tools --dev-load-game=1 --dev-map=kladwtwn --dev-elevation=0 --dev-hero-preview
```

Add `--dev-snapshot-once` for an actual GL PNG and metadata, or `--dev-floor-preview-return-after=1800` for a bounded return check. Physical hotkeys remain unverified by automation.
