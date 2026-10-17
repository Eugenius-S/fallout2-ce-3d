# fallout2-sprite-lighting

Динамическое освещение оригинальных спрайтов Fallout 2 + перспективная камера
на базе fallout2-ce. Спецификация:
`E:\Games\Fallout 2\docs\superpowers\specs\2026-10-17-fallout2-sprite-lighting-design.md`.

Направление заменяет `fallout2-native-dlss5` (прототип сохранён как история).

## Структура

- `ce/` — клон https://github.com/fallout2-ce/fallout2-ce (свой git, hash
  закреплён в `docs/pinned-commit.md`; в этот репозиторий не включён).
- `build/` — Ninja+MSVC сборка CE (см. `tools/configure-build.ps1`).
- `game/` — рабочая копия чистой игры (DATs пользователя, в git не входит).
- `docs/` — отчёты этапов, инструкции.
- `runs/` — артефакты проверки (скриншоты, логи).

## Сборка

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release
```

Требуется сеть при первой конфигурации: FetchContent подтягивает
SDL 2.32.8 и zlib 1.3.1. На этой машине MSVC-среда задаётся вручную
(сломанный vcvars в BuildTools) — скрипт это учитывает. Рантайм —
статический `/MT` через `CMAKE_MSVC_RUNTIME_LIBRARY`: upstream определяет
`_STATIC_CPPLIB`, при дефолтном `/MD` линковка падает LNK2038/LNK1319.

## Правила (из спецификации)

- Эталонная установка Steam (`D:\SteamLibrary\steamapps\common\Fallout 2`)
  только для чтения; CE не запускать в ней.
- Локальные коммиты, без push. Русификатор, совместимость сохранений и
  60 FPS не объявляются без отдельных проверок.
- DLSS5 — необязательный поздний этап F, не условие успеха.
