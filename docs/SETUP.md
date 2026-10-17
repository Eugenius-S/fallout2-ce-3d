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

## G1 ядро — PASS (2026-10-13, вечер)

- carrier: 2002/2002 NGX DLAA evaluates, 140.5 fps, median 7.0 ms (`runs/g1/carrier-smoke/summary.txt`).
- neural: 2033/2033 evaluates, 140.6 fps (`runs/g1/neural-smoke/summary.txt`); ReShade.log: `feature 18 created via the NGX core after DLSS/DLAA for NR input 1280x720 -> output 1280x720` + `inline feature 18 evaluation succeeded (count=600)` — экспериментальный DLSS5 нейропроход (RenoDX) исполняется на нативной сцене с нашими depth/motion. Лог: `runs/g1/neural-reshade.log`.
- Selftest: 4/4 контракта PASS (static motion, pan motion vs CPU, depth near/far, alpha hole).

Остаток G1 (не блокирует ядро): PNG carrier-vs-neural одинаковой позы, короткий захват движения, iso-камера. Затем G2 (экспорт kladwtwn) и G3 (бенчи native/carrier/neural 15+60 c).

## G1 — PASS (2026-10-13)

- Selftest 4/4 PASS (см. выше).
- Carrier: 2002/2002 evaluates, 140.5 fps; neural: 2033/2033, 140.6 fps; feature 18 создан и исполняется (600+ inline evals) — `runs/g1/neural-reshade.log`.
- PNG одной позы (static, PrintWindow): `runs/g1/carrier-static.png` vs `runs/g1/neural-static.png` — сцена (чекер-пол, 2 бокса, альфа-карта с дырой) рендерится и в DLAA, и через нейропроход; на статике отличия минимальны (ожидаемо для дефолтного NR стиля).
- Захват движения: `runs/g1/neural-pan-1..3.png` (pan-маршрут, нейро включён) — смаза/артефактов, блокирующих приёмку, не видно на кадрах.
- Iso-камера: `--camera iso` работает (smoke 30 кадров, 144 fps).
- ReShade banner подтверждает прокси-хук в carrier/neural; native-режим запускается без нейробиблиотек (проверено G0-стиль smoke).
- G1 = PASS. Визуальная приёмка изображения пользователем — ожидается (спец. требование: не объявлять приёмку без него).

Далее: G2 (экспорт kladwtwn → формат-6 → WIC-текстуры → камеры) и G3 (бенчи).

## G2 этап 1 — экспорт карты — PASS (2026-10-13)

`kladwtwn` elevation 0 «Klamath Downtown» экспортирован закреплённым экспортёром (commit 0da1cd4, venv Python 3.14.3 + requirements-tested.txt установились без подмен):
- `vendor/fallout2-first-person/viewer/data/kladwtwn/`: scene.json (sha256 `4b4e20d2...`), tiles.png, sprites0.png, light.png; итого 2 718 319 байт.
- Проверено: format=6; floor base64 → 20000 байт (100×100 uint16); proj-коэффициенты (squ/sqv/origin/hpx/roofPx/bbPx); spawn tile 25075; 1 sprite-страница.
- Содержимое: 303 тайла, 2431 спрайт, 1259 стен, 535 билбордов, 201 декаль, 39 криттеров, двери/ящики/барабаны.

Далее (Task 3/4): C++ reader format-6 (nlohmann/json + WIC PNG), порт `Proj/card/hexPx` из geom.js, геометрия floor→walls→props→critters, coverage-отчёт, две камеры, численные проверки. Затем G3.

## Проекция — порт и сверка — PASS (2026-10-13)

`src/projection.{h,cpp}` — точный порт geom.js (Proj, card, hexPx, hexFromPx, rowPlane). Числовая сверка с upstream JS (node tools/projection-ref.js на реальной scene.json): 12/12 чисел совпали с допуском 1e-9, selftest вырос до 14 PASS / 0 FAIL. Референс-числа: groundToPx 0,0,0→4784,-1190; hexFromPx 600,400→6997; hexPx 25075→4992,764; rowPlane 10,5→4.9475; card sprite0 shift=4.5 lean=0.169010; roofZ=1.879825.

## G1 — ПЕРЕОТКРЫТ (2026-10-13, вечер): верификатор подтвердил 4 дефекта, 3 исправлено, самотест теперь честный

Внешняя проверка коммита 8c05213 нашла: (1) camera.cpp записывал ТЕКУЩУЮ матрицу как previous → нейропроход получал нулевое движение, а самотест сравнивал ошибочный GPU-результат с такой же ошибочной CPU-моделью (вакуумный PASS); (2) iso-камера была перспективной, видео движения не было; (3) 140 fps при включённом vsync не доказывают цену нейропрохода; (4) resize не пересоздавал ресурсы и NGX-фичу.

Исправлено (коммит после этого):
1. camera.cpp: prevViewProj захватывается ДО lookAt (порядок исправлен); iso — настоящая ортографическая камера; детект camera cut по скачку глаза.
2. --camera iso|free в CLI; PNG-последовательность движения вместо «видео» (neural-pan-1..3.png).
3. --novsync (Present 0/0) для бенчей; G3 будет использовать + GPU-timestamps.
4. Render::resize пересоздаёт sized-ресурсы + NGX-фичу (releaseFeature/create) + forceReset; нул-гард (WM_SIZE приходит до initialize); main передаёт render.resize.

Повторный selftest: 12/14 PASS, **pan-motion FAIL — но теперь тест НЕ вакуумный**: GPU-поле движения ненулевое, связное, зависит от камеры (см. [field]-дамп: нули только на скай-пикселях, градиенты от объектов). Остаточное расхождение GPU-vs-CPU поля: x на левой половине перевёрнут по знаку, масштабы различаются. Гипотезы: (a) в самотесте точка соответствия неверна (пиксель показывает объект, а не пол — частично учтено unprojection), (b) конвенция знака x в motion PS. Требует одной адресной итерации с info-queue/GPU-based validation.

G1 возвращён в IN PROGRESS честно: нейропроход и DLAA работают, кадры рендерятся, НО корректность векторов движения пока не подтверждена попиксельной сверкой.

## G1 — ПОВТОРНО ПРОВЕРЕН — PASS (2026-10-14)

Три фикса по итогам верификации (коммит «G1 reopened» + этот):
1. Pan-motion тест: пересечение с плоскостью **Y=0** (пол), а не z=0 — ожидалось movement (0.9527, 2.8376) независимо посчитано верификатором; GPU выдал (0.952637, 2.835938) — совпадение в допуске 0.25 px. Подтверждено: векторы движения корректны.
2. Alpha-hole тест: контрольная статичная поза восстанавливается перед проверкой (две renderFrame(Static)) — delta 0.6498 PASS.
3. Render::resize: сброс stateColor_/stateMotion_/stateDepth_ (принадлежат старым ресурсам) + уже сделанные bbStates_.

Повторный selftest: **14/14 PASS**. Повторный прогон нейропрохода с исправленными векторами: `runs/g1/neural-fixed/summary.txt` + `runs/g1/neural-reshade.log` (feature 18, count=600).

Замечание о «140 fps везде»: с --novsync и native, и carrier дают ~141 fps — присутствует средовой троттлинг (DWM/драйвер, лимитер не найден в nvidia-smi). Это НЕ блокер G1; цена нейропрохода будет корректно измерена в G3 через GPU-timestamps вне_present-пути.

G1 = PASS (повторно). Далее G2 этап 2 (format-6 reader + геометрия) и G3.

## G2 этап 3 — Кламат в upstream-просмотрщике — PASS (2026-10-14)

Сервер: `tools/serve.py` (venv-питон, http://127.0.0.1:8000/). URL: `/fp.js?map=kladwtwn&iso=1` (fp.js — ES-модуль, страница — index.html; navigate напрямую на fp.js нельзя). Проверено в браузере: карта загрузилась (format 6 принят), текстуры/здания/предметы/криттеры/NPC отрисовались, iso-камера центрируется на spawn (25075), панорама перетаскиванием и зум колесом работают, криттеры анимируются, ошибок нет. Скриншоты: `runs/g2/klamath-iso-general.png`, `runs/g2/klamath-iso-pan.png`. Вкладка оставлена пользователю (deliverable).

Повторный запуск (из корня проекта):
```
cd vendorallout2-first-person && ..\.venv\Scripts\python.exe tools\serve.py
# открыть: http://127.0.0.1:8000/fp.js?map=kladwtwn&iso=1
```
Ограничение: это просмотр локации (без боя/диалогов/квестов). DLSS5 на этом этапе не подключался; нативная D3D12-реализация не тронута. Следующий шаг — перенос сцены в DirectX 12 (format-6 reader), после оценки Кламата пользователем.

##Iso-камера с перспективой — PASS (2026-10-14)

По запросу пользователя в iso-камеру просмотрщика добавлен параметр `lens=<мм>` (эквивалент полнокадрового телеобъектива; по умолчанию 0 = чистая ортографика как в игре). Реализация: четвёртая строка матрицы проекции делает clip-w убывающим по глубине камеры (w = 1 - k·dot(toCamera, p−center), k = 24/lens/(2·DEPTH)) — позиции считаются как раньше, но ближние объекты слегка крупнее. Проверено: `fp.js?map=kladwtwn&iso=1&start=0&lens=120` — лёгкая перспектива без шлейфов, 140 fps, 0 ошибок консоли. Скриншот: `runs/g2/klamath-iso-lens120.png`.

Повторный запуск (из корня проекта):
```
cd vendorallout2-first-person && ..\.venv\Scripts\python.exe tools\serve.py
# открыть: http://127.0.0.1:8000/fp.js?map=kladwtwn&iso=1&start=0&lens=120
```
Сила перспективы: lens=60 заметнее, lens=200 слабее, lens=0 выключить. Изменение только в viewer/fp.js (isoView), режим первого лица и D3D12-прототип не затронуты.
