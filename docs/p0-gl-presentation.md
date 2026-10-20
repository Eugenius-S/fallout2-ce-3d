# P0: GPU output inside CE (OpenGL 3.3 presentation)

Date: 2026-10-18. Branch `scene-renderer` off upstream `fb8ba51e`, commit `49be0b4`
(C1 stayed on the `sprite-lighting-c1` branch, `e14218e`, not developed).
Tech plan: `E:\Games\Fallout 2\docs\superpowers\plans\2026-10-17-fallout2-ce-3d-env-p0-p1.md`.

## What was done

`src/scene_renderer/` (a new module, included in the game and mapper targets):
- `gl_api.*` + `gl_procs.h`: a minimal OpenGL 3.3 core loader through
  `SDL_GL_GetProcAddress` (no third-party dependencies);
- `scene_gl.*`: an SDL_GL context on the CE window; the vanilla RGBA buffer
  is drawn as a quad over the whole framebuffer with letterbox scaling (the
  equivalent of `SDL_RenderSetLogicalSize`); the swap sits where
  `SDL_RenderPresent` was.

Integration in `svga.cc`:
- `_GNW95_init_window` first tries the GL path; on failure the old
  SDL_Renderer (the fallback is kept);
- `renderPresent()` branches by the active path; the palette-to-RGB
  conversion is the same per-frame `SDL_BlitSurface`, so fades and palette
  cycling behave identically to vanilla;
- `handleWindowSizeChanged` under GL is a no-op (the drawable size is
  queried every frame through `SDL_GL_GetDrawableSize`; `SDL_GetWindowSize`
  returns DPI-virtualized points and without this it letterboxed into a
  corner);
- movies are not presented under GL yet (the overlay is an SDL_Texture; in
  the working copy the movies are skipped by config): to restore before
  stage E.

## Verified

| Check | Result |
|---|---|
| Building both targets (game+mapper) | PASS |
| Running on the working copy, auto-load of slot 1 | PASS |
| Rendering world/sprites/HUD/cursor with no difference from SDL_Renderer | PASS (`runs/p0-gl-final.png`, `p0-gl-screen3.png`) |
| GL errors per frame (glGetError probes) | 0 on every call |
| GL init failure → vanilla path | branch kept (context/shader error handling) |
| Resize/fullscreen switch | per-frame drawable query (windowed checked; exclusive fullscreen from the config at start) |

## Bugs caught during bring-up (history for debugging)

1. GL loader: dereferencing the function pointer instead of a direct cast
   (`*reinterpret_cast<...>` of a function address): the first call jumped
   into garbage, segfault at start.
2. `layout(location=N)` needs GLSL 330 (not 150): the shaders were raised
   to `#version 330 core`.
3. `GL_UNPACK_ROW_LENGTH` mixed up with `GL_UNPACK_ALIGNMENT` (0x0CF2 vs
   0x0CF5): an alignment of 800 gave INVALID_VALUE on every upload.
4. `glDrawArrays` with `GL_TRIANGLE_FAN` (0x0006) instead of `GL_TRIANGLES`
   (0x0004).
5. DPI: `SDL_GetWindowSize` (points) vs `SDL_GL_GetDrawableSize`
   (framebuffer pixels): the render sat "in a corner" of the window at
   125% scale.

## What is next (P1, after the user accepts the look)

A live kladwtwn area: exporting the surroundings with the adapted donor
converter (`tools/scene_export/`), the scene in `game/mod-cache/scene/`, an
object snapshot adapter before `renderPresent()`, the scene + the UI strip
in GL, picking by ray, a vanilla/scene switch on a live scene. The
contracts are in the tech plan (§3–§5).
