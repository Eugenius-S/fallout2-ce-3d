# Working on Fallout 2 CE 3D

You are one of three agents on this project: `claude`, `gpt` (GPT Sol 6.1) or
`glm` (GLM 5.3). The owner is the critic and moderator. Read
[the roadmap](docs/2026-10-19-roadmap-design.md) once; it says what we build
and why.

Agents work one at a time in this one checkout. Any agent may stop at any
moment when its limits run out, so leave the work so that the next one can
continue from the last commit without asking anybody.

## Rules

- Everything in the repository is in English.
- Never commit game files, `game/`, `game-vanilla/`, `build/`, tokens or keys.
- The owner may have the game open (`game/fallout2-ce.exe`). Never close it,
  never kill it, never steal its focus. Check your work with the background
  test instance (`tools/dev-snapshot.ps1`), which runs as
  `fallout2-ce-test.exe`; that is the only process you may stop.
- Do not run `tools/dev-launch.ps1` unless the owner asks: it starts the
  owner's game.
- Python is `py -3` on this machine (`python` is a Store stub).
- Keep changes to upstream CE code (`ce/src/*.cc` outside `scene/`,
  `scene_renderer/`, `devtools/`) small: a hook or a call, the logic lives in
  our modules.
- Graphics stay within OpenGL 4.1 core: no compute shaders, no SSBOs.
- Every new player-facing feature gets a key in the `[3d]` settings, a default
  and a line in the options menu, and switches itself off with a log line if it
  fails to start.

## Build and check

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release
ctest --test-dir build --output-on-failure
py -3 -m unittest discover -s tools/scene_export -p 'test*.py' -v
py -3 tools/scene_export/export.py --game-root ./game
powershell -NoProfile -ExecutionPolicy Bypass -File tools/dev-snapshot.ps1 -Map kladwtwn -Elevation 0
```

`dev-snapshot.ps1` prints the snapshot JSON; the PNG lies next to it in
`game/`. `-Probe` also checks the mouse (see `docs/p1-devtools.md`).

## Tasks

One task is one file `tasks/NNN-slug.md`. Show the board:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/tasks.ps1
```

The header of a task file:

```yaml
---
id: 003
title: Regression snapshot test
milestone: M0
status: todo        # todo | doing | done
owner: none         # claude | gpt | glm | none
suggested: gpt      # who fits best; any agent may take it
claude-only: no     # yes: only claude takes it
user-check: no      # yes: the owner looks at the result after the merge
depends-on: []      # task ids that must be done first
reviewed: no        # claude (or the owner, for claude's tasks) sets yes
---
```

The body has `Goal`, `Context`, `Acceptance`, `Out of scope` and `Log`.

### Taking a task

1. `git status` must be clean. `git switch main`, `git pull`.
2. If a task is `doing`, finish it first: read its `Log`, `git switch` to its
   branch and continue.
3. Otherwise take a free task: `todo`, all `depends-on` are `done`, not
   `claude-only` (unless you are claude). Prefer the lowest-id free task
   `suggested` for you; if there is none, the lowest-id free task.
4. Claim it on `main`, so the board shows it taken: set `status: doing` and
   `owner: <you>`, commit `task NNN: start`, push `main`. Then create the
   branch `task/NNN-slug` and push it.
5. If another agent may be working in this checkout at the same time, work in
   your own worktree next to it and never switch branches here:

   ```powershell
   git worktree add --detach ../fallout2-ce-3d-<you> origin/main
   # claim there: edit the task header, commit, then
   git push origin HEAD:main
   git switch -c task/NNN-slug
   git push -u origin task/NNN-slug
   ```

   A worktree has no `game/` and no game build: tasks that run the game
   (snapshots, regression) are done in this checkout, one agent at a time.
   After the merge: `git worktree remove ../fallout2-ce-3d-<you>`.

### While working

- Commit every meaningful step. Each commit that ends a working session or a
  step adds a `Log` entry in the task file, in the same commit:

  ```markdown
  ### 2026-10-19 gpt
  Done: what now works, with paths.
  Next: the exact next step.
  Check: the command or snapshot that shows it.
  ```

- Push the branch after each such commit.
- If you learn the task is wrong or too big, write it in the `Log`, split it
  into new task files with the next free ids, and say so in the PR.

### Finishing

1. Every line of `Acceptance` holds; the build, the tests and the regression
   snapshots pass. Visual changes: snapshots in `runs/<NNN-slug>/`.
2. Set `status: done`, add the last `Log` entry, commit, push.
3. `gh pr create` using the template, with the snapshots; then
   `gh pr merge --squash --delete-branch`; then `git switch main`, `git pull`.

## Review

Claude reviews every `done` task with `reviewed: no` when it has limits:
reads the PR diff, runs the checks, sets `reviewed: yes`, and turns each
problem into a new task. Pull requests from outside contributors are merged
only after Claude's review.
