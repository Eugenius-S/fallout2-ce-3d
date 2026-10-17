# Отчёт этапа A: игровая основа (fallout2-ce на копии чистой игры)

Дата: 2026-10-17. Спецификация:
`E:\Games\Fallout 2\docs\superpowers\specs\2026-10-17-fallout2-sprite-lighting-design.md`.

## Закреплённая основа

- Upstream: https://github.com/fallout2-ce/fallout2-ce, commit
  `fb8ba51e016b09597bc0b4277fbac26899e43342` (2026-10-17, «Typesafe sound.h
  enum usage (#868)») — см. `pinned-commit.md`. Клон в `ce/`, в этот
  репозиторий не включён. Изменений в исходниках CE нет.
- Сборка: `tools/configure-build.ps1` (Ninja + MSVC 14.40, SDK 22621,
  CMake 4.4). Собрались все цели: `fallout2-ce.exe`, `mapper-ce.exe`,
  `ce-dat-tool.exe`, `ce-frm2png.exe`.

### Особенности тулчейна на этой машине

1. VS BuildTools имеет сломанный vcvars (vswhere не видит VC.Tools) —
   окружение (PATH/INCLUDE/LIB) задаётся вручную скриптом.
2. CE определяет `_STATIC_CPPLIB` (статический STL). При дефолтном для CMake
   динамическом рантайме (`/MD`) линковка падает LNK2038/LNK1319
   (libcpmt.lib = MT против MD-объектов). Решение без правки upstream:
   `CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>` —
   весь билд статический (/MT), exe автономный.

## Рабочая копия игры

- Источник: `D:\SteamLibrary\steamapps\common\Fallout 2` (только чтение,
  эталон не изменялся, CE в нём не запускался).
- `game/`: `master.dat`, `critter.dat`, `patch000.dat`, `fallout2.cfg`.
  Контрольные суммы (md5) — `runs/a-dat-checksums.txt`:
  - master.dat `5acf4f9a8bb017af0f7b35c7f920a48b`
  - critter.dat `9b16e92ac88d168e81126b72e6f9ab3c`
  - patch000.dat `6afa3fde17b95191cb6bae18bfd10291`
- Важно: CE читает/пишет `fallout2.cfg` и создаёт `data/` **рядом со своим
  exe**, не в текущем каталоге. Правильная раскладка — exe в папке игры
  (`game/fallout2-ce.exe`), как в штатной установке CE. Экспериментальный
  запуск exe из `build/` оставил после себя `build/fallout2.cfg` с дефолтами —
  это объясняет, почему `skip_opening_movies=1` в `game/fallout2.cfg`
  сначала не действовал.
- `game/fallout2.cfg`: добавлена секция `[ui]` `skip_opening_movies=1`
  (пропуск IPLOGO/INTRO/CREDITS при проверках; логика — `src/main.cc:96`).

## Что проверено

| Проверка | Результат | Свидетельство |
|---|---|---|
| Сборка закреплённого CE без изменений | PASS | 594/594 целей ninja |
| Запуск нашего exe на рабочей копии | PASS | `runs/a-launch*.log`, скриншоты |
| Декодирование MVE-роликов и слайдов | PASS | `runs/a-afterclick*.png` |
| Splash → меню → новая игра (Temple of Trials), HUD и мир отрисованы | PASS | `runs/a-state4.png` |
| Сохранение (SLOT01: SAVE.DAT, ce.sav, sfallgv.sav) и загрузка сейва в нашем билде | PASS | `game/data/SAVEGAME/`, сессия пользователя 15:20, фуллскрин |

Проверки «переход карты, бой, диалог, инвентарь» из гейта этапа A —
интерактивные: автоматизированы частично, окончательное подтверждение за
ручной сессией на рабочей копии (пользователь уже играет в наш билд).
Загрузка сейва в пользовательской сессии фактически покрывает и «загрузку
карты».

## Отличия CE от оригинала (версия-закрепление)

Из README upstream (+SFALL_COMPATIBILITY.md для скриптов sfall):

- Разрешение не ограничено 640x480 (`[screen] resolution_x/y`, `windowed`).
- Напарники могут тащить/торговать за ГГ; прямое экипирование напарников.
- Инвентарь/лут в 2 колонки, бартер в 4 ряда, расширенная AP-полоса.
- Ctrl-клик для быстрого переноса предметов; подсветка
  предметов/трупов/контейнеров/криттеров (sfall-mods.ini).
- Музыка играет между картами; звук 44.1 кГц стерео, .ogg/.wav помимо .acm.
  Замечание: при отсутствии OGG-файлов ничего не меняется для чистой игры.
- Авто-открытие дверей, встроенное меню HELP, ESC отменяет выбор этажа
  лифта, запоминается последний слот сохранения, спиффи-амперметр патронов.
- «Dozens of small things»: лучший pathfinding, меньше графических глитчей.
- Формат сейва расширен (в сейве появляется `ce.sav`, `sfallgv.sav`);
  сейвы оригинального exe испытывать только на копиях (сделано — вся работа
  в `game/`, эталон не тронут).

Итог: точного поведения оригинала CE не обещает (как и требует
спецификация); различия перечислены выше и дополняются по мере проверки.

## Открытые зависимости

- Русификатор (этап B): пакет не выбран пользователем. Поддержка русского
  НЕ объявляется. После выбора — фиксация источника/версии/сумм и
  воспроизводимая инструкция установки на копию.
- 60 FPS (этап C/D): не измерялось, не объявляется.

## Инструкция воспроизведения

```powershell
cd E:\Projects\fallout2-sprite-lighting
powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release
Copy-Item build\fallout2-ce.exe game\
# запуск: game\fallout2-ce.exe
```
