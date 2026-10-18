# P0: GPU-вывод внутри CE (OpenGL 3.3 презентация)

Дата: 2026-10-18. Ветка `scene-renderer` от upstream `fb8ba51e`, коммит `49be0b4`
(C1 остался на ветке `sprite-lighting-c1`, `e14218e`, не развивается).
Техплан: `E:\Games\Fallout 2\docs\superpowers\plans\2026-10-17-fallout2-ce-3d-env-p0-p1.md`.

## Что сделано

`src/scene_renderer/` (новый модуль, включён в game и mapper цели):
- `gl_api.*` + `gl_procs.h` — минимальный загрузчик OpenGL 3.3 core через
  `SDL_GL_GetProcAddress` (без сторонних зависимостей);
- `scene_gl.*` — SDL_GL-контекст на окне CE, ванильный RGBA-буфер рисуется
  квадом на весь фреймбуфер с letterbox-масштабом (эквивалент
  `SDL_RenderSetLogicalSize`), swap на месте `SDL_RenderPresent`.

Интеграция в `svga.cc`:
- `_GNW95_init_window` сначала пробует GL-путь; при неудаче — прежний
  SDL_Renderer (fallback сохранён);
- `renderPresent()` ветвится по активному пути; конвертация палитры в RGB —
  тот же покадровый `SDL_BlitSurface`, поэтому фейды и palette cycling
  ведут себя идентично ванили;
- `handleWindowSizeChanged` под GL — no-op (drawable size запрашивается
  каждый кадр через `SDL_GL_GetDrawableSize`; `SDL_GetWindowSize` даёт
  DPI-виртуализованные точки и без этого леттербоксил в угол);
- фильмы под GL пока не выводятся (overlay — SDL_Texture; в рабочей копии
  ролики пропускаются конфигом) — вернуть до этапа E.

## Проверено

| Проверка | Результат |
|---|---|
| Сборка обеих целей (game+mapper) | PASS |
| Запуск на рабочей копии, автозагрузка слота 1 | PASS |
| Рендер мира/спрайтов/HUD/курсора без отличий от SDL_Renderer | PASS (`runs/p0-gl-final.png`, `p0-gl-screen3.png`) |
| GL-ошибки по кадрам (пробы glGetError) | 0 на всех вызовах |
| Падение GL-инициализации → ванильный путь | ветка сохранена (обработка ошибок контекста/шейдеров) |
| Resize/fullscreen-переключение | per-frame drawable query (оконный режим проверен; эксклюзивный фуллскрин — по конфигу при старте) |

## Выловленные при подъёме баги (история для отладки)

1. Загрузчик GL: разыменование указателя функции вместо прямого каста
   (`*reinterpret_cast<...>` от адреса функции) — первый же вызов прыгал в
   мусор, segfault на старте.
2. `layout(location=N)` требует GLSL 330 (не 150) — шейдеры подняты до
   `#version 330 core`.
3. `GL_UNPACK_ROW_LENGTH` перепутан с `GL_UNPACK_ALIGNMENT` (0x0CF2 vs
   0x0CF5) — выравнивание 800 давало INVALID_VALUE на каждой загрузке.
4. `glDrawArrays` с `GL_TRIANGLE_FAN` (0x0006) вместо `GL_TRIANGLES`
   (0x0004).
5. DPI: `SDL_GetWindowSize` (точки) vs `SDL_GL_GetDrawableSize` (пиксели
   фреймбуфера) — рендер «в углу» окна при масштабе 125%.

## Что дальше (P1, после приёмки вида пользователем)

Живой участок kladwtwn: экспорт окружения адаптированным конвертером донора
(`tools/scene_export/`), сцена в `game/mod-cache/scene/`, адаптер снапшота
объектов перед `renderPresent()`, сцена + UI-полоса в GL, picking лучом,
переключатель ваниль/сцена на живой сцене. Контракты — в техплане (§3–§5).
