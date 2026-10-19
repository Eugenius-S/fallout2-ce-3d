# Task 004: OpenGL 4.1 core acceptance

Production changes are limited to the context initialization, two query
entry points and the loader's ceiling comment. No scene, camera, object,
devtools or shader behavior was changed.

- Release build passed; 4 engine tests, 30 exporter tests and 9 regression
  utility tests passed.
- `regression-summary.json`: all six original task-003 references matched
  with **zero changed pixels**, without updating any reference.
- `debug-context.log`: six OpenGL **4.1.0 NVIDIA 617.14** contexts, core
  profile `0x1`, context flags `0x3` (forward-compatible + debug). Each enabled
  the optional `GL_ARB_debug_output` callback; no error/deprecation messages
  occurred. `core41/` contains the captured entrance PNG and original JSON.
- `fallback-context.log` and `fallback/`: a temporary impossible **99.1**
  context request failed and the existing SDL renderer displayed Klamath in
  2D. The built-in F9 handler captured the vanilla framebuffer in the test
  process; the JSON identifies `sdl-renderer` and `KLADWTWN.MAP`.
- `restored-summary.json`: after restoring the exact source bytes and
  rebuilding, the normal 4.1 entrance again matched with zero changed pixels.

`unavailable-context-test.patch` records only the temporary failure stimulus:
the impossible context version and a timed in-process SDL F9 event to invoke
the existing dev snapshot handler after fallback. It was **not** retained in
the production renderer. No physical input, focus changes, owner's executable
or `dev-launch.ps1` were used. Every run used `tools/dev-snapshot.ps1` with
this worktree's build and the shared game directory.

The raw fallback PNG exposes an existing dev-snapshot row-pitch issue at an
odd logical width (1155): the vanilla capture passes padded SDL surface data
to a tightly packed PNG handler without repacking rows. Its diagonal image
artifacts are not evidence of a renderer-context failure. The live process,
map and 2D path are established by its JSON and log. Keep this capture issue
for task 009 after task 002; devtools was deliberately not changed here.

`scene-context.log` is appended in the game's working directory at startup;
it records the actual version, renderer, profile and flags. `F2CE_GL_DEBUG=1`
enables a debug context and the optional ARB callback. Only error and
deprecated-behavior messages are logged by that callback. OpenGL 4.3/KHR
entry points are not required. Failed initialization deletes our context and
resets SDL's GL attributes so the vanilla renderer can choose its own context.
