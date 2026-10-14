# fallout2-native-dlss5 — SETUP

Спека: `docs/superpowers/specs/2026-10-12-fallout2-native-dlss5-design.md`.
План: `docs/superpowers/plans/2026-10-12-fallout2-native-dlss5.md`.
Манифест исходников: `docs/baseline/native-prototype-2026-10-12.json`.

## Окружение (проверено 2026-10-12)

| Компонент | Значение |
|---|---|
| GPU / драйвер | RTX 5070 / **617.14** |
| MSVC | 14.40.33807 (v143), VS BuildTools **18.2.1** (`C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools`) |
| CMake | **4.4.4** — отсутствовал, установлен через winget (`Kitware.CMake`) в ходе G0 |
| Ninja | winget (`Ninja-build.Ninja`) — см. «Отклонение от плана» ниже |
| Python | 3.14.3 (экспортёру понадобится проверка `requirements-tested.txt` — Task 3) |
| Git | 2.54.0.windows.1 |

## Отклонение от плана (зафиксировано)

1. **Генератор.** План предписывает `cmake -G "Visual Studio 17 2022"` — VS17 на машине нет (есть BuildTools 18.2.1, чей VC-компонент не регистрируется для vswhere-запросов CMake: `vswhere -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64` пуст, `vcvars64.bat` отсутствует). Используется **Ninja-генератор** с явным окружением MSVC 14.40 + Windows SDK 10.0.22621.0 (пол плана соблюдён: тот же компилятор, тот же SDK-минимум). Скрипт: `tools/configure-build.ps1`.
2. **Установка CMake и Ninja через winget** — компоненты заявленного стека отсутствовали; план требует «сообщить точный недостающий компонент». Оба установлены в ходе G0, зафиксированы здесь и в `sources.lock.json.toolchain`.

## Сборка

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\configure-build.ps1            # configure + build (Release)
ctest --test-dir build -C Release --output-on-failure                                    # contracts (Ninja: ctest из PATH CMake)
powershell -NoProfile -ExecutionPolicy Bypass -File tools\stage-runtime.ps1              # staging 3 runtime-каталогов
```

Артефакты Ninja: `build\f2native.exe`, `build\f2native_tests.exe` (single-config — без подкаталога `Release\`; `stage-runtime.ps1` учитывает оба варианта).

## Запуск

```powershell
# native smoke (G0): ~2 c, окно с треугольником, выход 0
runtime\native\f2native.exe --smoke-frames 120
```

CLI (актуален на G0; расширяется в Task 2): `--smoke-frames N`, `--width W`, `--height H`.

Коды выхода: 0 успех, 2 аргументы/файлы, 3 GPU/NGX, 4 невыполненный гейт.

## Staging

`tools/stage-runtime.ps1` создаёт `runtime/{native,carrier,neural}`: копирует Release EXE; для carrier/neural — 4 файла из манифеста с проверкой SHA-256 (`E:\Games\Fallout 2\host64`, read-only) и `ReShade.ini` из спеки §4 (`NeuralUplift=0`/`1`), пустые `effects\`/`textures\`. Повторный запуск с незнакомыми файлами в runtime — ошибка без удаления. Инвентарь: `runtime/staged.json`.

## G0 — PASS (2026-10-12)

- Старый проект: HEAD `287d6a1` (= манифестный `e8e5f71` + документация Codex), dirty-пути совпадают с манифестом (5 файлов). Runtime-хеши 4/4 OK.
- Исходники: 4 клона на точных коммитах (см. `sources.lock.json`), лицензии в `third-party-notices/`.
- Сборка: Ninja+MSVC, x64, Release. `ctest`: contracts **PASS** (формула motion −128,0 из спеки §7).
- Staging: 13 файлов OK.
- Native smoke: `SMOKE OK: rendered 120 frames`, exit 0; визуальное доказательство `runs/g0/native-triangle.png` (RGB-треугольник на тёмном фоне, PrintWindow-захват); числовое — readback-диагностика кадра 5: строка 360, non-clear пиксели x=[480..799] = расчётный интервал треугольника (±0.5 NDC → 320..960/180..540, на середине высоты 480..800).
- Диагностический scaffolding, оставленный сознательно: one-shot readback кадра 5 (`[grid]` в stderr) — основа для readback-тестов G1; debug-слой D3D12 включён всегда, сообщения info-queue печатаются в stderr (`[d3d12]`).

## Известные грабли (для executor'а)

- `stage-runtime.ps1` падает с IOException, если какой-то `f2native.exe` ещё запущен (файл залочен) — закрывать инстансы перед стейджем; «тихий» stale-exe в runtime = источник ложных результатов (дважды срабатывало: старый exe без новых диагностик).
- PowerShell 5.1 читает UTF-8 ps1 без BOM как ANSI: не использовать эм-даши/не-ASCII в `.ps1` (ломает парсер через байт 0x94→`"`).
- Растяжка 720p-окна на ультравайд: окно создаётся 1280×720 client; ресайз обрабатывается.

## Task 2 (G1, синтетическая сцена) — статус 2026-10-13

Рендер сцены собран и самопроверен: `--selftest` PASS (static motion ~0; pan motion соответствует CPU-проекции; depth near/far; alpha-hole). Дебаг-инфраструктура: D3D12 debug layer + InfoQueue дамп в stderr (Graphics Tools установлен 2026-10-13 с разрешения пользователя).

Грабли, съевшие время (все устранены):
1. SRV desc `MipLevels=0` инвалид для 1-mip ресурса → deferred device removal без сообщений (debug layer отсутствовал). Фикс: MipLevels=1. Урок: без Graphics Tools debug layer молчал — установка обязательна для G1+.
2. Backbuffer без явных барьеров COMMON→RT / RT→PRESENT → removal на Present (WARP воспроизводит стабильно). Фикс: bbStates_ трекинг + барьеры (как было в G0).
3. `SetDescriptorHeaps` не вызывался → таблицы дескрипторов невалидны. Фикс: перед таблицами.
4. Readback CopyTextureRegion с footprint-форматом != формату ресурса → E_INVALIDARG на Close. Фикс: формат параметром (R32_FLOAT для depth).
5. Draw-оффсеты 42/78 превышали VB (60 вершин) → семплирование за концом буфера. Фикс: 36/54.
6. Allocator Reset E_FAIL при неисполненном закрытом списке (flushInit был DIAG-disabled) — списки нужно исполнять перед Reset.

Следующий шаг: carrier-режим (NGX DLAA) → 300 evals + feature-18 через RenoDX → G1 evidence.
