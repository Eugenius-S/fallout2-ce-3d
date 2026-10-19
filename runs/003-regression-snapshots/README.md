# Task 003: regression snapshot acceptance

The command is `powershell -NoProfile -ExecutionPolicy Bypass -File tools/regression.ps1`.
Six committed references are in `tools/regression/reference/`; `gallery.jpg`
shows their coverage. These references preserve the current picture, including
existing visual defects, so later scene changes can be reviewed explicitly.

| Check | Evidence | Result |
|---|---|---|
| Exact camera and still pixels across independent launches | `still-a/`, `still-b/` | Actual view tile 25075; identical decoded pixels |
| Explicit update of all six references | `update-summary.json` | Six valid captures; references written |
| First complete regression | `pass-a.json` | Six passes, zero changed pixels |
| Second complete regression, same build | `pass-b.json` | Six passes, zero changed pixels |
| Deliberately disabled floor geometry | `floor-disabled.patch`, `floor-disabled/summary.json` | Exit 1, 1,392,871 changed pixels |
| Restore and rebuild | `restored-summary.json` | Entrance passes, zero changed pixels |
| Unit checks after restoration | CTest; exporter and regression unittest discovery | 4 + 30 + 9 passed |

The deliberate fault replaced only
`glDrawArrays(kTriangles, 6, gFloorVertexCount - 6)` with a zero-count draw.
`floor-disabled/highlight.png` marks affected pixels red;
`floor-disabled/difference.png` amplifies the deltas. The source was restored
byte-for-byte in `finally` and the normal EXE rebuilt before recovery was
checked. The patch is a test artifact and must not be applied during normal use.

Every image was captured by `tools/dev-snapshot.ps1`: an off-screen,
non-activating test window, a unique snapshot run token and only the PID
returned by that launcher stopped. No owner's game process was addressed,
no window-input tools were used and `tools/dev-launch.ps1` was not run.

Author review covered opt-in isolation, camera bounds, source restoration,
run ownership and cleanup, RGBA/alpha comparison, resolution changes,
failed captures, metadata mismatch and all-or-none validation before a
reference update. `reviewed: no` remains for Claude's independent review.
Snapshots retain their capture-time commit/EXE identities; earlier captures
precede the commits that saved their source changes.
