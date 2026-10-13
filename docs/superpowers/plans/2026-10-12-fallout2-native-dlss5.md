# Fallout 2 Native DLSS5 — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking. Исполнитель работает сам; дополнительные агенты не нужны.

**Goal:** проверить DLSS5 на нативной сцене, затем показать одну карту Fallout 2 в отдельном x64 просмотрщике.

**Architecture:** собственный D3D12 рендер передаёт Color/Depth/Motion в NGX DLAA; локальный RenoDX выполняет экспериментальный neural pass. Сначала проверяется этот путь, затем переносится карта из закреплённого Python/WebGL проекта.

**Tech Stack:** C++20, Win32/D3D12, HLSL, CMake >=3.24, MSVC v143, Windows SDK >=10.0.22621.0, WIC, DirectXMath, nlohmann/json v3.11.3, Python >=3.10.

## Global Constraints

- Обязательная спека: `E:\Games\Fallout 2\docs\superpowers\specs\2026-10-12-fallout2-native-dlss5-design.md`.
- Новый проект: `E:\Projects\fallout2-native-dlss5`; Steam и старый эксперимент — только чтение.
- Только `kladwtwn`, статическая сцена, 1280×720, SDR, две камеры; без gameplay и frame generation.
- Gates строго G0 → G1 → G2 → G3. До G1 запрещён перенос карты.
- Нет silent fallback, обновления DLL/драйвера и GitHub-публикации. Любой PASS сопровождается путями к свежим доказательствам.
- Не обещать официальную DLSS5 SDK интеграцию и не считать успех DLAA доказательством neural pass.

## 1. Разметка проекта и интерфейсы

Все следующие пути относительно нового проекта. Это задания реализации, а не утверждение, что перечисленные файлы уже существуют.

| Файлы | Ответственность |
|---|---|
| `CMakeLists.txt`, `CMakePresets.json` | `f2native` и `f2native_tests`, x64, /MD, SDK paths, Release |
| `src/main.cpp`, `src/app.{h,cpp}` | Win32 окно, CLI, режим, frame loop, exit codes |
| `src/gpu.{h,cpp}` | device/queue/swapchain, resource states, fence, timestamp/readback |
| `src/render.{h,cpp}`, `shaders/scene.hlsl`, `shaders/present.hlsl` | Color/Depth/Motion, синтетическая сцена, финальный вывод |
| `src/camera.{h,cpp}` | матрицы, previous state, две проекции, фиксированный маршрут |
| `src/neural.{h,cpp}` | NGX init/create/evaluate/release; без IPC |
| `src/scene.{h,cpp}`, `src/projection.{h,cpp}`, `src/texture.{h,cpp}` | format 6, геометрия из upstream, PNG через WIC |
| `src/metrics.{h,cpp}` | JSONL/CSV, CPU/GPU timings и идентичность прогона |
| `tools/stage-runtime.ps1`, `tools/run.ps1` | раздельные runtime-каталоги, hash check, свежие логи |
| `tests/contracts.cpp`, `tests/scene.cpp` | motion/depth/alpha/reset и чтение сцены |
| `docs/SETUP.md`, `docs/RESULTS.md`, `sources.lock.json` | команды сборки, версии, gates, ограничения |

Публичные границы модулей — зафиксировать в заголовках до реализации:

```cpp
enum class Mode { Native, Carrier, Neural };
struct FrameInputs {
    ID3D12Resource* color;
    ID3D12Resource* depth;
    ID3D12Resource* motion;
    ID3D12Resource* output;
    uint32_t width, height;
    bool resetHistory;
};
// NGX-ошибка бросает исключение с именем операции и hex result;
// catch в main пишет failure.json, возвращает 3, не продолжает neural-run.
class NeuralPass {
public:
    void initialize(ID3D12Device* device, const std::filesystem::path& runtimeDir);
    void create(ID3D12GraphicsCommandList* commands, uint32_t width, uint32_t height);
    void evaluate(ID3D12GraphicsCommandList* commands, const FrameInputs& frame);
    void shutdown(); // после GPU fence, до освобождения device
};
```

`FrameInputs` заимствует ресурсы текущего frame slot; не хранить их после evaluate. `Gpu` владеет их lifetime. Native не создаёт NeuralPass. Все три режима используют один Scene/Camera/Render. Ошибка аргументов/файлов = exit 2, GPU/NGX = exit 3, невыполненный gate = exit 4; success = 0.

## Task 1 — G0: окружение, исходники, native skeleton

- [ ] Снять `git status --short` старого проекта и hashes из `docs/baseline/native-prototype-2026-10-12.json`; остановиться при несовпадении библиотеки, не заменять lock новым hash автоматически. Записать текущие GPU/driver/toolchain. Проверить наличие MSVC/CMake/Python; если инструмент отсутствует, сообщить точный недостающий компонент, не менять стек наугад.
- [ ] Создать новый каталог, локальный Git и ветку `codex/native-dlss5`. Скопировать туда спеку, план и манифест. Добавить `.gitignore`: `build/`, `.venv/`, `runtime/`, `runs/`, `vendor/`, `*.dll`, `*.exe`, `*.addon64`, `*.dat`, `*.log`; upstream/generated game data внутри vendor также не должны попадать в Git.
- [ ] Получить закреплённые исходники следующими командами; проверять `$LASTEXITCODE` после каждого git вызова. Выполнять из корня нового проекта:

```powershell
git clone https://github.com/Apolotary/fallout2-first-person.git vendor/fallout2-first-person
git -C vendor/fallout2-first-person checkout --detach 0da1cd449f1dc7f2f6986ff7e8728a4b6ae20584
git clone https://github.com/NVIDIA/DLSS.git vendor/ngx
git -C vendor/ngx checkout --detach 374959484e79a640feaba44c93ac8cfb0a03f5b5
git clone https://github.com/jlrouzies-fr/DLSS5-Feeder.git vendor/feeder-reference
git -C vendor/feeder-reference checkout --detach 3f624855276c4bde55145c712782477639b30e85
git clone --branch v3.11.3 --depth 1 https://github.com/nlohmann/json.git vendor/json
git -C vendor/json checkout --detach 9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03
```

- [ ] Сохранить `git rev-parse HEAD` каждого checkout и лицензии в `sources.lock.json`/`third-party-notices/`. Для NGX использовать `include/` и `lib/Windows_x86_64/x64/nvsdk_ngx_d.lib`; не arm64 и не debug lib с Release /MD. Линковать Windows библиотеки `d3d12`, `dxgi`, `dxguid`, `d3dcompiler`, `windowscodecs`, `ole32`, `user32`, `gdi32`.
- [ ] Создать CMake targets и минимальный native frame loop: resize, present, fence, shutdown. Сначала native должен показать цветной треугольник и закрыться без device errors. Команды, которые обязан поддерживать результат:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

- [ ] Сделать `stage-runtime.ps1`: создать только внутри `runtime/native`, `runtime/carrier`, `runtime/neural`; скопировать один Release EXE и шейдеры; для двух последних проверить и скопировать четыре локальных файла из манифеста, создать ini из спеки. В native не копировать proxy DLL. Повторное выполнение с незнакомыми файлами завершается ошибкой, без удаления. Сохранить manifest фактически staged файлов. G0: сборка и native smoke PASS. Коммит только собственных исходников/документов.

## Task 2 — G1: синтетическая сцена и нейропроход

- [ ] В render/camera создать текстурный пол, две перекрывающиеся коробки, alpha-card с отверстием; поддержать неподвижную камеру, боковое движение и camera cut. Текстуры теста генерировать процедурно. Создать Color/Depth/Motion/Output из спеки и debug/readback для каждого.
- [ ] В `tests/contracts.cpp` проверить пример (-128,0), zero motion, alpha hole и depth near/far; GPU readback сравнить с CPU-ожиданиями, не тестировать шейдер копией того же шейдера. После resize/cut проверить reset первого evaluate и отсутствие stale resources.
- [ ] Реализовать NeuralPass по публичным заголовкам NGX и pinned host reference. Проверять каждый Init/Create/Evaluate result; input/output state transitions явные. ReShade должен увидеть создание D3D12 device до NGX init. Не обходить его через собственный unwrapped device. Стартовую диагностику загрузки ограничить 5 секундами; если аддон не готов, сохранить ошибку и завершить, не крутить Init бесконечно.
- [ ] Реализовать CLI: `--mode native|carrier|neural`, `--scene synthetic|<scene.json>`, `--camera iso|free`, `--width`, `--height`, `--warmup`, `--seconds`, `--run-dir`, `--route static|pan`, `--capture-poses`. Некорректные значения отвергать. CLI `--mode` должен совпасть с staged профилем; neural не может работать из native-каталога.
- [ ] `tools/run.ps1` принимает `-Mode`, `-Scene`, `-Camera`, `-Route`, `-Warmup`, `-Seconds`; фиксирует размер 1280×720, создаёт уникальный run-dir, запускает EXE из соответствующего runtime с тем же working directory, ожидает завершения, переносит свежие ReShade.log и ini в run-dir. До запуска сохраняет прежний журнал в отдельный архив, не смешивает его с новым. Сохраняет command line, EXE hash, PID, время старта/конца и exit code.
- [ ] Проверить оба типа проекции каждым режимом. Рабочая команда после реализации launcher:

```powershell
powershell -NoProfile -File tools/run.ps1 -Mode neural -Scene synthetic -Camera free -Route pan -Warmup 15 -Seconds 60
```

- [ ] Сохранить 300 последовательных успешных carrier evaluations, свежие feature-18 create/inline evaluation сообщения и отсутствие последующих сбоев. Снять PNG одинаковых поз carrier/neural и короткий видеозахват движения. Проверить, что именно NGX output выводится на экран. Если изображения не осмотрены, G1 ещё не принят. При провале соблюдать лимит двух исправлений/двух часов из спеки, записать `docs/RESULTS.md` и остановиться до Task 3.
- [ ] Зафиксировать G1 и измерения отдельным коммитом документов/исходников; локальные медиа остаются в runs. Указать их пути и SHA-256 в отчёте.

## Task 3 — G2: экспорт и чтение одной карты

- [ ] Только после G1 установить Python-зависимости экспортёра и выполнить:

```powershell
py -3 -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r vendor\fallout2-first-person\requirements-tested.txt
Push-Location vendor\fallout2-first-person
try {
    & ..\..\.venv\Scripts\python.exe tools\fp_export.py --game 'D:\SteamLibrary\steamapps\common\Fallout 2' kladwtwn
    if ($LASTEXITCODE -ne 0) { throw 'Export failed' }
} finally { Pop-Location }
```

- [ ] Ожидаемый файл: `vendor/fallout2-first-person/viewer/data/kladwtwn/scene.json`. Проверить `format=6`, map, elevation, список PNG и decoded floor/roof по 20000 байт; записать hash всех выходных файлов и Python/package versions. Отсутствие нужной карты — ошибка данных, не повод заменить её другой без отражения в спеке.
- [ ] В scene/texture реализовать чтение JSON/PNG, range/size checks, диагностировать файл и поле при ошибке. В `tests/scene.cpp` добавить испорченные маленькие synthetic fixtures: неверный format, base64 size, sprite page вне диапазона, отсутствующая PNG. Они должны завершать загрузку ошибкой, без GPU crash. Лицензированные игровые данные не использовать как коммитимые fixtures.
- [ ] В projection портировать `Proj`, `card`, `hexPx` с закреплённого geom.js; сопоставить результаты минимум на трёх точках пола и sprite anchor через маленький upstream JS reference script и CPU-тест. Не выдумывать коэффициенты «похожие на изометрию».

## Task 4 — G2: видимая карта и камеры

- [ ] Портировать последовательно floor/roof → walls/trims/doors → boards/decals → props/forms → static critters. После каждой группы сверять её счётчики и контрольный вид с upstream. Использовать геометрическую first-person ветку для обеих камер, как требует спека.
- [ ] Учитывать заменённые hidden-элементы, atlas padding, anchor, alpha discard и световую карту. Составить coverage для всех непустых визуальных полей, включая явно отключённые декоративные эффекты. Не завершать G2 при пропавших props/forms/персонажах.
- [ ] Камеры: Tab переключает iso/free с reset; WASD перемещение, правая кнопка мыши поворот free, колесо zoom iso, Esc выход. Начальная точка определяется `spawn.tile` через `hexPx` и `Proj`; world up и масштабы берутся из единой системы координат. Детерминированный тестовый pan: смещение вдоль экранного права на 4 единицы пола за 10 секунд и обратно за следующие 10 секунд, повтор; неизменный угол камеры. Это маршрут сравнения, не управление персонажем.
- [ ] В интерактивном окне показать режим и FPS; запуск другого режима — через launcher/новый процесс. GUI и текст не включать в входной Color нейросети. Сделать именованные снимки исходного Color, конечного output, depth и motion. Повторить численные motion/depth проверки на карте. G2: сравнение с upstream просмотрено, coverage заполнен, обе камеры управляются.

## Task 5 — G3: измерение и передача

- [ ] Запустить три независимых 15+60-секундных прогона `native`, `carrier`, `neural` с абсолютным Scene path `E:\Projects\fallout2-native-dlss5\vendor\fallout2-first-person\viewer\data\kladwtwn\scene.json`, `-Camera free -Route pan`. Затем проверить iso, resize и повторный старт neural. В benchmark vsync off, размер фиксирован, foreground/occlusion записаны; свёрнутый/закрытый другой программой прогон недействителен.
- [ ] Сохранить frame CSV, GPU timestamps/frequency, свежие журналы и ini, PNG одинаковых именованных поз, короткое видео с движением. Разница native→carrier показывает стоимость carrier/перехватов, carrier→neural — добавочную стоимость neural комплекта; не приписывать всё время одному внутреннему kernel.
- [ ] В `docs/RESULTS.md` заполнить G0–G3 статусами PASS/FAIL/NOT RUN и доказательствами. Отдельно: работает ли feature 18, достигнут ли порог скорости, оценено ли изображение пользователем. Не писать «стало фотореалистично» без изображений и приёмки.
- [ ] Повторно сравнить hashes Steam-файлов и исходного runtime, `git status` старого проекта. Не исправлять обнаруженные чужие изменения. Записать сборочную команду и три команды запуска в `docs/SETUP.md`; сделать финальный локальный коммит собственных исходников/документов. Проверить `git ls-files`: архивов/атласов/DLL/generated data там нет.

## Промпт исполнителю

> Реализуй спецификацию `E:\Games\Fallout 2\docs\superpowers\specs\2026-10-12-fallout2-native-dlss5-design.md` по плану `E:\Games\Fallout 2\docs\superpowers\plans\2026-10-12-fallout2-native-dlss5.md`. Новый проект — `E:\Projects\fallout2-native-dlss5`, игровые данные — `D:\SteamLibrary\steamapps\common\Fallout 2`. Сначала G0 и G1: нативная синтетическая сцена и доказанный neural pass. Только после G1 переноси `kladwtwn`. Не меняй обе установки игры и не запускай прежнюю цепочку Feeder. Не заменяй DLL или API догадками. Работай небольшими локальными коммитами, не публикуй GitHub. Если G1 не проходит в установленный лимит, оставь воспроизводимый отчёт и остановись. Итог: исходники, точные команды, результаты gates, измерения и изображения; никакого PASS по одному логу carrier.
