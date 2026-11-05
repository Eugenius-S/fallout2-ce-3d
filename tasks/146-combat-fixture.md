---
id: 146
title: A background combat fixture: start a fight and capture each phase
milestone: M1
status: blocked
owner: gpt
suggested: any
priority: normal
tier: 3
claude-only: no
user-check: no
depends-on: []
reviewed: no
---

## Goal

The background test run can start a fight on any map and capture it, so a
combat change in the 3D view (135 and after) is checked without the owner.

## Context

- 135 step 1 (#211) keeps the 3D view on in combat, but nobody can capture a
  fight in the background: `tools/dev-snapshot.ps1` has no combat switch, and
  the toxic caves' geckos attack only after the hero walks in.
- Add `-Combat` to `dev-snapshot.ps1` and a matching `--dev-*` flag in
  `ce/src/devtools/`: after the map loads, start combat as the game does
  when the player presses A, then capture, in one process:
  1. the player's turn with the mouse over a hostile critter in attack mode
     (the to-hit number shows);
  2. right after one attack on it (the projectile or the hit, the floating
     damage text);
  3. the enemies' turn (a critter walking or attacking);
  4. after the fight ends (end combat).
  Each capture gets its JSON sidecar like the other snapshots, with the
  combat phase and the scene's availability (`preview policy`).
- Hostiles: take the map's own hostile critters; where a map has none,
  `-CombatTarget TILE` may place one documented test critter. Say in the
  Log which way each Klamath map uses.
- The flag never touches the owner's game or `game/mod-cache`, like the
  rest of the background run.

## Acceptance

- `dev-snapshot.ps1 -Map klatoxcv -Combat` and `-Map kladwtwn -Combat`
  each give the four captures in 3D; a strip of both in
  `runs/146-combat-fixture/`.
- The same with `-Classic` gives the 2D picture, for comparison.
- The help at the top of `dev-snapshot.ps1` describes the switch.
- ctest passes.

## Log

- 2026-11-01 Cut for 135 (no way to capture a fight).

### 2026-11-02 gpt

Done: Added `-Combat` and `-CombatTarget`, a devtools fixture that selects a map critter with aggressive AI or creates an aggressive test copy on a free nearby tile, and sidecar fields for the requested phase and actual combat/turn/scene state. The script requires exactly four phase captures and checks their sidecars. `klatoxcv` is expected to use its map hostiles; `kladwtwn` falls back to a test copy when no aggressive critter is present. This map split is inferred from the fixture rules, not confirmed by a completed capture.

Two background attempts on `klatoxcv` loaded the map and available 3D preview but timed out before the fixture started. The second log showed `sceneStartupReady() == false` with a loaded hero/map. A readiness bypass using the started preview is included as a hypothesis and has no runtime confirmation. Per the two-attempt limit, no more captures were run.

Next: Re-run the four-phase captures on both maps in 3D and Classic, confirm or fix the startup readiness bypass, verify map-hostile versus test-critter selection, then create the two-map phase strips.

Check: Release build passed; `ctest --test-dir build --output-on-failure` passed 18/18. Failed attempt logs: `runs/146-combat-fixture/klatoxcv-3d/process.stdout.log` and `runs/146-combat-fixture/klatoxcv-3d-attempt-2/process.stdout.log`.

### 2026-11-03 gpt continuation

Done: kept the task claimed as `doing` by `gpt` and copied the two existing
attempts' stdout/stderr logs into this branch under `runs/146-combat-fixture/`.
They confirm the preview loaded but the combat fixture never became ready.
Next: continue with task 147 under the owner's two-attempt exception. Task 146
still needs working startup readiness and all eight 3D/2D phase captures.
Check: PR #220 remains draft; its last CI run passed. No third runtime attempt
was made.

### 2026-11-03 merge origin/main

Merged `origin/main` (`d8cff6b0`) into this branch. Resolved the `tools/dev-snapshot.ps1` overlap by keeping the combat fixture arguments, four-capture polling, and main's scripted-input/scene-input options together. PowerShell parser and `git diff --check` pass. Runtime combat proof remains blocked as described above; PR stays draft.

- CI `runs-size` public-content subcheck identified absolute drive-qualified cache paths in the two existing attempt logs. Replaced those environment-specific paths with `build/scene-cache`; rerunning the public-content check after committing this cleanup.

### 2026-11-04 gpt: merge fresh origin/main

Merged `origin/main` at `3b288fd6` with no conflicts. The two recorded combat attempts did not reach the combat fixture, so PR #220 stays draft and the task remains `doing` under the two-attempt limit.

### 2026-11-04 gpt: merge current main

Fetched and merged `origin/main` at `8e135a73867b` into the existing task
branch; no conflicts. Pushed merge commit `be7d1efbe5fd1be30e0a90a1606d3de5da125d82`.
No new runtime attempt was made because the two documented combat starts
still fail before the fixture begins. PR #220 remains draft and task 146 stays
`doing` with the four 3D and four Classic captures outstanding.

Check: Release build passed; CTest passed 20/20. No pixel-regression or
combat-runtime result is claimed for this merge.

### 2026-11-05 gpt: startup preview correction

Done: Added `gCombatFixtureRequested` to the startup-preview trigger. The
combat-only command did not request any of the flags that made
`gStartupFloorPreviewAttempted` true, so `prepareCombatFixture()` could only
wait forever. Release build and CTest passed 20/20.
Next: Keep PR #220 draft. A capture after this fix still logged the initial
`COMBAT_FIXTURE WAIT` and `static wall+hero preview started` but never logged
`COMBAT_FIXTURE START` or a phase capture. This is the second consecutive
uncompleted runtime attempt; stop runtime retries. Diagnose the update-loop
readiness with a bounded code-level probe before any new capture.
Check: `runs/146-combat-fixture/klatoxcv-3d-fixed-startup/process.stdout.log`.
The run was stopped after its timeout. `build/scene-cache` was absent, so the
game selected the shared checkout's default cache path. I did not manually
modify or clean it; the game may have touched that path during startup. Export
to a worktree-local `build/scene-cache` before any future runtime attempt.

### 2026-11-04 gpt: merge current main

Merged fetched `origin/main` at `46f697873c84`. The only conflict was the task
header; kept main's current board fields (`todo`, `any`, tier 3) and preserved
the combat task text and blocker log. No new runtime attempt was made after
the two documented startup timeouts. PR #220 stays draft; four-phase 3D and
Classic captures remain outstanding.
