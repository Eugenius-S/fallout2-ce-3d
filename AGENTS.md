# Working on Fallout 2: Reimagined

Three agents: `claude`, `gpt`, `glm`. The owner accepts the visuals and
changes these rules as the work goes.

- `gpt` and `glm` share one pool of tasks: whoever has limits left takes the
  top free task (`owner: any`) in the board order below. `claude` keeps the
  strategy and the board. The per-city reviews are the owner's flights
  (`owner: owner`); the pool turns their shots into fixes.
- `gpt` works through exactly one `gpt-6-luna` subagent as its controller
  and writes no code itself. `glm` works by itself, with no subagent. Goals and
decisions: [the roadmap](docs/2026-10-19-roadmap-design.md).

- Agents work in parallel, each in its own worktree. Any agent may stop at
  any moment: the next one continues from the last pushed commit and the
  task's `Log`, without asking anybody.
- Nobody waits. Nothing waits for the owner's or Claude's approval; whatever
  the owner rejects later becomes a new task.
- Claude keeps the strategy (the order of the board, big tasks cut into
  bounded ones with checks a script can run, the specs, the reviews) and codes
  the design-heavy core; the board always has free work for `gpt` and `glm`.
- The owner's finds from chat go to [tasks/inbox.md](tasks/inbox.md), not
  into the current work, unless the owner says "now".
- Fold first. A new find or task first goes through the open tasks and
  batches: if one fits, it joins that one (a Context line there); a new task
  is cut only when nothing fits. Every new board entry (an inbox line, a
  task) names the batch it joined, or says why it stands alone.

Where things are: [shapes and camera rotation](docs/2026-10-20-shapes-and-rotation-design.md),
[dev tools](docs/p1-devtools.md) (snapshots, tours, the fly camera, restarting
the owner's game), [regression](tools/regression/README.md), the shapes eval
in `tools/shapes/eval/` (`eval.py shoot | sheets | score`), the fly tour
(`tools/tour/tour.py`, its spots in `tools/tour/spots.json`).

## Rules

- Everything in the repository is in English, except `README.ru.md`, the
  Russian copy of `README.md`: change both together. Never write the long dash
  (U+2014): use a colon, a comma, a full stop or parentheses.
- Never commit `game/`, `game-vanilla/`, `build/`, game files, tokens or keys.
- The owner may have `game/fallout2-ce.exe` open: never close it, never steal
  its focus, never run `tools/dev-launch.ps1` unless the owner asks. Check
  your work with the background test instance (`tools/dev-snapshot.ps1`,
  `fallout2-ce-test.exe`), the only process you may stop. Test games from
  different worktrees run side by side, each writing only its own run
  folder; runs from one worktree queue up.
- Python is `py -3` (`python` is a Store stub).
- Project folders follow "merged, then gone" (the owner, 2026-11-07): when a
  task's PR lands in main, its worktree folder leaves the projects root the
  same day (`git worktree remove --force`); open-task folders with local
  debris move to `_archive/worktrees` instead. Experiment outputs the owner
  called keepsakes (regenerated sprites, art trials, marketing sources) are
  never deleted: they live committed under `runs/<task>/`, or moved to
  `_archive` and `!marketing`.
- After every code merge into main the game is verified before anybody
  plays it: rebuild Release from fresh `origin/main`, run one off-screen
  `tools/dev-snapshot.ps1` launch check on the demo copy, and only then put
  the exe into `game/fallout2-ce.exe` (the previous one stays as
  `fallout2-ce.exe.bak-<date>`). Main must always be playable. Docs-only
  merges skip the check.
- Limits go on re-reading the context: every call pays for the whole
  thread so far (measured in 079: 99% of the tokens). So:
  - one task per thread; after its PR, or once the thread is long, start a
    new one from the task's `Log`;
  - few calls: put read-only commands in one call; `dev-snapshot.ps1`
    waits for its worktree's test slot itself, so never poll for it;
  - open a snapshot image only when numbers or JSON cannot answer, one at a
    time, cropped or scaled down; every image stays in the thread.
- Evidence in `runs/` is a few JPEG sheets or crops of at most 1 MB each
  (CI refuses more); raw captures stay in `game/shots/`.
- A task that changed what the game, a tool or a folder looks like ends with
  at most 4 outcome screenshots attached in the chat when the task is done
  (and committed under its `runs/NNN-slug/` folder), so the owner sees the
  result without running anything. Pure code or numbers work needs none.
- A change to what the 3D scene shows (the exporter, its caches, the
  renderer) runs the fly tour against main (`py -3 tools/tour/tour.py
  --before <main's scene cache or checkout>`) and attaches its
  `changed.jpg` to the PR (in `runs/NNN-slug/`); a spot that should change
  and did not, or changed and should not, is fixed before the merge.
- Upstream CE code (`ce/src/*.cc` outside `scene/`, `scene_renderer/`,
  `devtools/`) gets a hook or a call at most; the logic lives in our modules.
- On a local map the 3D view never turns off. An object that cannot be
  built in 3D yet stands as a flat picture (a billboard or a fixed plane),
  never as a switch back to 2D. The world map, dialogs and menus stay 2D.
- Speed is a feature: a steady 60 FPS, a warm-cache map change in 2 s, no
  stall over 100 ms on a settings change. Measure with the frame-time log
  (127) before and after any change that may cost time.
- A fix in the 3D scene is a rule that holds for every map, object and
  wall of its kind, never a case for one prototype, tile or map (the owner,
  2026-10-27). A PR names the rule and shows it on more than the one spot
  the task named.
- OpenGL 4.1 core: no compute shaders, no SSBOs.
- The game's own view stays pixel for pixel; new 3D geometry shows only in
  flight or a turned view, unless a task says otherwise.
- A player-facing feature gets a `[3d]` key with its literal default on the
  field in `Scene3dSettings`, registered in `ce/src/settings.cc`, an English
  menu label and description, and switches itself off with a log line if it
  fails to start.

## Build and check

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release
ctest --test-dir build --output-on-failure
py -3 -m unittest discover -s tools/scene_export -p 'test*.py'
py -3 -m unittest discover -s tools/regression -p 'test*.py'
py -3 tools/scene_export/export.py --game-root ./game
powershell -NoProfile -ExecutionPolicy Bypass -File tools/regression.ps1
```

For a locally installed Restoration Project copy outside the main checkout,
check that CE still boots and loads maps with
`powershell -NoProfile -ExecutionPolicy Bypass -File tools/rp-check.ps1
-GameRoot <scratch-game> -BuildExecutable build/fallout2-ce.exe`. The check
keeps test executables, configs, logs and saves under `build/`.

Read a check's result before the next step; never chain a merge after it.

## Board order

Take tasks in this order (the owner, 2026-11-04); the `tier` field in the
header carries it and `tools/tasks.ps1 -Next` sorts by it:

0. Out of turn, always first: anything that crashes the game or leaves a
   place the player cannot get past. Cut it with `tier: 0`.
1. Measurement tools: 127 (the FPS counter and frame-time log), then 183
   (the screenshot with its metadata).
2. Whole-game systems: speed (172 location load, 156 no stall on a settings
   change, 157 a steady 60 FPS), 173 (3D never off, the plane fallback),
   174 (multi-part objects), the Restoration Project (184-188).
3. Batches by kind: geometry, one class of objects across every map per
   task and per PR (189 walls, 190 roofs, 191 furniture and props, 192
   vehicles; the area exports 164-171 feed them); the UI as one batch
   (140, 130, 134, 149, 128, 193).
4. Local fixes per location, last: only from the owner's fly-through
   screenshots (183), one batch per city (175 on, `owner: owner` until the
   owner's shots land); local-only tasks wait here.

One batch is one PR in size. Speed work and geometry work run one after the
other, never side by side (both touch the renderer); work on other files
(RP sfall, tools, UI) may run in parallel with either.

## Tasks

One task is one file `tasks/NNN-slug.md` with `Goal`, `Context`,
`Acceptance`, `Out of scope` and `Log`. Its header:

```yaml
id: 003
title: Regression snapshot test
milestone: M0
status: todo        # todo | doing | done
owner: any          # claude | gpt | glm | any | owner (the owner's own step)
suggested: gpt      # a preference: if that agent is out of limits, take it
priority: normal    # high: taken before the other free tasks
tier: 3             # the board order below (0 first)
claude-only: no
user-check: no      # yes: the owner looks after the merge (never blocks)
depends-on: []
reviewed: no        # see Review
```

`tools/tasks.ps1` shows the board; `-Next <you>` prints the task to take:
your own doing task, else a stale one (no commit on its branch for three
days), else the top free `todo` in the board order (`tier`, then
`priority`, then the id). Never skip past a tier 0 task (a crash or a
blocker). Finish the task you took before taking
another: a hard task is split (step 4), not handed over.

1. Work in your own worktree, never switch branches in the main checkout:
   `git worktree add --detach ../fallout2-ce-3d-<you> origin/main`. It has no
   `game/`: the scripts use the main checkout's `game/` with the worktree's
   build. A changed exporter writes to the worktree (`export.py --game-root
   <main>/game --maps klamath --cache-root build/scene-cache`).
2. Claim with a lock: check that no open PR and no branch exists for the
   task's number (`gh pr list --search "NNN:"`, `git ls-remote origin
   'task/NNN-*'`); `git switch -c task/NNN-slug`, set `status: doing` and
   `owner: <you>` in the task file, commit, push, and open a draft PR named
   `NNN: <title>` at once. That draft PR is the lock: if another agent's PR
   for the number is already there, take the next task. The board reads the
   header from the branch; nothing goes to `main` until the merge.
3. Commit and push each meaningful step, one-line messages. Write a `Log`
   entry only when you stop or finish, in that commit:

   ```markdown
   ### 2026-10-19 gpt
   Done: what now works, with paths.
   Next: the exact next step.
   Check: the command or snapshot that shows it.
   ```

4. A task that turns out wrong or too big: say so in the `Log` and split it
   into new task files with the next free ids, in the same branch.
5. Finish: every `Acceptance` line holds, the build, tests and regression
   pass, visual changes have snapshots in `runs/<NNN-slug>/`. Set `status:
   done` and `reviewed` (below) in the last commit, `gh pr create` with the
   template, `gh pr merge --squash --delete-branch`. Docs and new task files
   ride in the same PR; no separate commits to `main` for bookkeeping.

## Review

The one merge gate is green CI (build, tests, regression). The PR body says
what it touched. The author merges as soon as CI is green; nothing else is
reviewed before the merge. The owner reviews each city later in a fly-through
with 183's shots (tasks 175 on), and the pool turns the finds into fixes.

The checks are the review. The author sets `reviewed: yes` on finishing,
except when the PR retook regression references, changed a file format the
engine reads (`walls.bin`, `baked.bin`, `scene.json`) or came from outside
the three agents: then `reviewed: no`, and Claude reviews it when it has
limits (`tools/tasks.ps1 -Review`): reads the diff, runs the checks, sets
`reviewed: yes`, turns problems into new tasks. Outside PRs merge only after
that review.
