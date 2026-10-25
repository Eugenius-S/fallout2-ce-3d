# Working on Fallout 2 CE 3D

Three agents: `claude`, `gpt` (GPT Sol 6.1), `glm` (GLM 5.3). The owner
accepts the visuals and changes these rules as the work goes. Goals and
decisions: [the roadmap](docs/2026-10-19-roadmap-design.md).

- One agent works at a time, whoever has limits. Any agent may stop at any
  moment: the next one continues from the last pushed commit and the task's
  `Log`, without asking anybody.
- Nobody waits. Nothing waits for the owner's or Claude's approval; whatever
  the owner rejects later becomes a new task.
- Claude keeps the strategy (the order of the board, big tasks cut into
  bounded ones with checks a script can run, the specs, the reviews) and codes
  the design-heavy core; the board always has free work for `gpt` and `glm`.
- The owner's finds from chat go to [tasks/inbox.md](tasks/inbox.md), not
  into the current work, unless the owner says "now".

Where things are: [shapes and camera rotation](docs/2026-10-20-shapes-and-rotation-design.md),
[dev tools](docs/p1-devtools.md) (snapshots, tours, the fly camera, restarting
the owner's game), [regression](tools/regression/README.md), the shapes eval
in `tools/shapes/eval/` (`eval.py shoot | sheets | score`).

## Rules

- Everything in the repository is in English. Never write the long dash
  (U+2014): use a colon, a comma, a full stop or parentheses.
- Never commit `game/`, `game-vanilla/`, `build/`, game files, tokens or keys.
- The owner may have `game/fallout2-ce.exe` open: never close it, never steal
  its focus, never run `tools/dev-launch.ps1` unless the owner asks. Check
  your work with the background test instance (`tools/dev-snapshot.ps1`,
  `fallout2-ce-test.exe`), the only process you may stop; one agent at a time.
- Python is `py -3` (`python` is a Store stub).
- One task per thread: after its PR, start a new thread for the next task.
  Open a snapshot image only when numbers or JSON cannot answer the question,
  one at a time; every image stays in the context and is paid for on each
  later request.
- A task that changed what the game, a tool or a folder looks like ends with
  at most 4 outcome screenshots attached in the chat when the task is done
  (and committed under its `runs/NNN-slug/` folder), so the owner sees the
  result without running anything. Pure code or numbers work needs none.
- Upstream CE code (`ce/src/*.cc` outside `scene/`, `scene_renderer/`,
  `devtools/`) gets a hook or a call at most; the logic lives in our modules.
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

Read a check's result before the next step; never chain a merge after it.

## Tasks

One task is one file `tasks/NNN-slug.md` with `Goal`, `Context`,
`Acceptance`, `Out of scope` and `Log`. Its header:

```yaml
id: 003
title: Regression snapshot test
milestone: M0
status: todo        # todo | doing | done
owner: none         # claude | gpt | glm | none
suggested: gpt      # a preference: if that agent is out of limits, take it
priority: normal    # high: taken before the other free tasks
claude-only: no
user-check: no      # yes: the owner looks after the merge (never blocks)
depends-on: []
reviewed: no        # see Review
```

`tools/tasks.ps1` shows the board; `-Next <you>` prints the task to take:
your own doing task, else a stale one (no commit on its branch for three
days), else the first free `todo` suggested for you, else any free one;
`priority: high` tasks come first. Finish the task you took before taking
another: a hard task is split (step 4), not handed over.

1. Work in your own worktree, never switch branches in the main checkout:
   `git worktree add --detach ../fallout2-ce-3d-<you> origin/main`. It has no
   `game/`: the scripts use the main checkout's `game/` with the worktree's
   build. A changed exporter writes to the worktree (`export.py --game-root
   <main>/game --maps klamath --cache-root build/scene-cache`).
2. Claim by pushing the branch: `git switch -c task/NNN-slug`, set `status:
   doing` and `owner: <you>` in the task file, commit, `git push -u origin
   task/NNN-slug`. The board reads the header from the branch; nothing goes
   to `main` until the merge.
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

The checks are the review. The author sets `reviewed: yes` on finishing,
except when the PR retook regression references, changed a file format the
engine reads (`walls.bin`, `baked.bin`, `scene.json`) or came from outside
the three agents: then `reviewed: no`, and Claude reviews it when it has
limits (`tools/tasks.ps1 -Review`): reads the diff, runs the checks, sets
`reviewed: yes`, turns problems into new tasks. Outside PRs merge only after
that review.
