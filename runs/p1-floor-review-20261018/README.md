# P1 static-floor review — 2026-10-18

## Verdict

The static-floor technical slice passes runtime review. P1 as a whole is not complete: the preview draws the floor only, with a fixed perspective camera. Walls, doors, live objects, picking, and lighting are not accepted by this review.

Reviewed code is uncommitted on CE base `f6c1969` and outer repository base `b647b1e`. No push was performed.

## Independent runtime evidence

The reviewer launched the fresh build in `game/runtime-review-20261018/`, using copied dev saves/config/cache and read-only DAT hardlinks. The original game installation and its save files were not used for test writes. Window inspection used Computer Use; no PowerShell input injection was used.

Final executable SHA-256:

`ff6c701409380cc46a78af41a0c53cffc980884fd88aab0d9b1be9cd046179f9`

Final invocation:

```text
fallout2-ce.exe --dev-tools --dev-load-game=1 --dev-map=kladwtwn --dev-elevation=0 --dev-floor-preview --dev-snapshot-once --dev-floor-preview-return-after=900 --dev-tools-commit=b647b1e-dirty
```

| Check | Result | Evidence |
| --- | --- | --- |
| Fresh build loads Klamath e0 | PASS | Window observed before/after preview |
| Actual GL floor is visible | PASS | `devfloor-20261018-143809-000.png` and matching JSON |
| Saved GL image matches window | PASS | Independent window observation plus PNG inspection |
| Actual camera and executable recorded | PASS | `devshot-20261018-143809-000.json` |
| Return after GL rendering restores vanilla presentation | PASS | `return-stdout.log`, `scr00000.png`, `devshot-20261018-143825.json`, observed window |
| Corrupt sidecar refuses preview and keeps vanilla presentation | PASS | `corrupt-stdout.log`; a three-byte sandbox sidecar was used and then restored |
| Failed preview produces no new success screenshot | PASS | Corrupt run logged snapshot skipped; existing prior evidence was retained |
| Physical Ctrl+F10/F9 input | UNVERIFIED | Automated keyboard input also failed for Escape and Alt+D; the frame-count return exercises the same disable helper but does not prove keyboard delivery |

The corrupt-cache test used the immediately preceding build SHA-256 `88b9126344177f1b589c23550439bdd1b2ca6060debd1e41f174a6e980b87caf`. The final build adds the bounded return test and detailed camera metadata; the cache loader is unchanged.

## Review corrections

1. Resource priority now follows the active configuration. With `master_patches=data` and `critter_patches=data`, those directories override archives. Current-directory fallback remains last. The former claim that loose `data` always came last was incorrect.
2. Atlas UVs address the inner 88-pixel square within each 100-pixel padded cell. Y coordinates now match the top-down LodePNG upload instead of selecting another atlas row.
3. Preview snapshots read the real GL backbuffer and flip readback rows before PNG encoding. They no longer save the vanilla software framebuffer while claiming to show the preview. JSON refers to the exact generated PNG.

Current exporter suite: 16 tests passed, including a real DAT2 fixture conflicting with a configured patch directory. Release CE and mapper builds succeeded; CE `git diff --check` passed.

Input/cache fingerprint: `518a7633e11f5a1479501bbff65774dfd0763e5cecc63894e0f16e6f0f39a1e4`. The corresponding `provenance.json` is retained here.

## Next slice

Render the donor's static wall geometry and doors in the same perspective view, first at a useful close camera position. Then connect CE's live object state. Do not label the floor-only screenshot as a complete volumetric Klamath or completed P1.
