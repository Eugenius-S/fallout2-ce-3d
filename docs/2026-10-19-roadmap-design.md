# Fallout 2 CE 3D: roadmap and team design

Agreed with the project owner on 2026-10-19.

## Goals

Keep Fallout 2 authentic: rules, turn-based combat, dialogs, quests, saves and
the original character sprites stay as they are. Make it look better, run
easily on modern computers and big screens, and be friendlier to use.

- Buildings and landmark objects in 3D: automatic conversion for ordinary
  surroundings, hand-made models (GPT Image textures + modeling) for landmarks.
- A 2D-to-3D conversion that is correct everywhere: no draw-order mistakes.
- Dynamic light: the game's own light sources become real lights (replace or
  overlay the 2D lighting).
- Glow on lasers, plasma, signs, fire and the like.
- Building shadows; sprites darken in shadow and brighten in light.
- A sun that moves across the sky with game time; cloud shadows.
- Small green NPC conversation replies and player answers at the window's real
  resolution; other interface text stays pixel-rendered.
- Clean map edges: cut straight, black background.
- Camera zoom.
- Every new setting in the options menu, each feature can be switched off.
- Easy install with a clear onboarding page on GitHub; a macOS build.
- The repository is public and entirely in English.

Later, wanted but not required: anti-aliasing of any level, DLSS from the menu,
in-between animation frames for characters, camera rotation (sprites always
face the camera).

## Decisions

| Topic | Decision |
|---|---|
| Graphics API | OpenGL 4.1 core, the macOS ceiling: no compute shaders, no SSBOs. DLSS later through OpenGL-Vulkan interop on NVIDIA. |
| First milestone | A vertical slice: all seven Klamath maps brought to the final look before scaling to the whole game. |
| Camera rotation | An option, off by default, in 90-degree steps; things get their shapes by an algorithm ([design](2026-10-20-shapes-and-rotation-design.md)). |
| Language | Everything in the repository in English: code, docs, tasks, handoff notes, commits, PRs. |
| Visibility | The repository becomes public after a history audit (task 005); the owner flips the switch. |

## Team and process

Three agents (Claude, GPT Sol 6.1, GLM 5.3) and the owner, who accepts the
visuals. Agents work one at a time and any of them may run out of limits at
any moment. How they take, hand off, finish and review tasks is in
[AGENTS.md](../AGENTS.md), the only instruction for every agent.

Split: Claude keeps the strategy (the board's order, cutting big tasks into
bounded ones with script checks, the specs, reviews of risky PRs) and the
design-heavy scene core; GPT takes large self-contained features; GLM takes
well-bounded smaller tasks.

## Milestones

**M0 Foundation.** The process files; a regression snapshot test; the scene
code split into modules; an OpenGL 4.1 core context; the history audit; the
docs translated; CI that builds the game.

**M1 Vertical slice: Klamath.** In this order:

1. A correct scene on all Klamath maps (KLADWTWN, KLATRAP, KLACANYN, KLAGRAZ,
   KLAMALL, KLARATCV, KLATOXCV), every elevation, roofs that hide as in the game.
2. Camera: zoom; clean map edges on black.
3. Settings: the `[3d]` store and the options menu with a scrolling
   Pip-Boy-style panel. It comes before the effects so each effect is born
   with its switch.
4. Light: game light sources as lights, sprites lit and tinted; the sun by game
   time with building shadows, sprites darkened in shadow; glow; cloud shadows.
5. Sharp text for small green NPC conversation replies and player answers;
   other interface text stays pixel-rendered.

**M2** Landmark buildings in 3D. **M3** The whole game: all maps, combat,
transitions, interiors. **M4** Install, onboarding, macOS. **M5** The later
list above. Each gets its own design session before its tasks are written.

## Architecture

- **Game-to-scene bridge** moves from `ce/src/devtools/` to `ce/src/scene/`:
  the code that feeds objects, the mouse and the interface into 3D is not a dev
  tool. `devtools/` keeps only dev launch, snapshots and the mouse probe.
- **`ce/src/scene_renderer/`** is split by purpose: window and present, floor
  and walls, live objects, mouse picking, camera (zoom, edges), light, post
  effects (glow; later anti-aliasing and DLSS). Each module has a small header;
  a task normally touches one or two modules.
- **Settings.** All new options live in a `[3d]` section of `fallout2.cfg`,
  described with the engine's `SettingDescriptor` schema. Every feature has a
  key, a default and a menu line; a feature without a menu line is not
  accepted. A master switch turns 3D off and leaves plain 2D Fallout 2.
- **Failures.** A map without a built scene, or with a broken cache, is shown
  in 2D with the reason in the log; the game never crashes for it. An effect
  that fails to start (unsupported GPU, shader error) switches itself off and
  logs; everything else keeps working.

## Definition of done for any task

- The game builds; engine unit tests and exporter tests pass.
- The regression snapshots (fixed Klamath spots against reference images) show
  no unexpected difference. An intended change updates the references in the
  same PR, with before and after images.
- Visual tasks put their snapshots in `runs/<task>/` and in the PR.
- All checks run in the background test instance; the owner's open game is
  never touched or closed.
