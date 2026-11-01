# Fallout 2 CE 3D

[English](README.md) | **Русский**

https://github.com/user-attachments/assets/e2fb04bc-243a-49a0-a775-7aa46c1ee8d2

Этот проект строит вокруг оригинальных спрайтов Fallout 2 объёмный 3D-мир.
Пошаговый бой, диалоги, квесты, сохранения и правила остаются прежними. Камера
сохраняет косой ракурс и атмосферу Fallout: это та же игра, а не шутер.

## Установка

[Скачайте лаунчер](https://github.com/Eugenius-S/fallout2-ce-3d/releases/download/latest/fallout2-ce-3d-launcher.exe)
из постоянно обновляемого [релиза latest](https://github.com/Eugenius-S/fallout2-ce-3d/releases/tag/latest).

1. Установите Fallout 2 из Steam или GOG.
2. Запустите лаунчер. Он сам находит эти установки; если игра не нашлась,
   укажите её папку, где лежат `master.dat` и `critter.dat`.
3. Для первой установки и обновлений нужен интернет.

Лаунчер ставит или обновляет `fallout2-ce.exe`, `ce.dat` и
`scene-exporter.exe`, а затем собирает данные сцены из ваших файлов игры.
Существующий `fallout2.cfg` сохраняется; стандартный ставится, только если его
нет. Лаунчер не заменяет оригинальный `fallout2.exe` и не трогает сохранения.
При сохранении настроек игра может дописать недостающие значения в раздел `[3d]`.

## Что работает

- Полы, стены, крыши и крупные объекты складываются в 3D-сцену. Персонажи
  остаются оригинальными спрайтами и двигаются так же, как в Fallout 2.
- Освещение и тени от солнца и луны следуют игровому времени. Лампы и другие
  точечные источники дают местный свет и свечение.
- Перспективная камера плавно следует за игрой и умеет поворачиваться. Зум
  колесом мыши плавно переходит на новый уровень. Сцена работает при любом
  разрешении экрана.
- В игровой панели `3D OPTIONS` есть `Camera zoom` и `Shadow edge softness`
  для мягких краёв теней. Для осмотра сцены есть камера свободного полёта
  разработчика.
- Данные сцены пока собираются для локаций Кламата. Где данных сцены нет, игра
  показывает свой оригинальный 2D-вид.

## Переключение в 2D

Сценой управляют игровая панель `3D OPTIONS` и раздел `[3d]` в `fallout2.cfg`.
`Ctrl+F10` возвращает оригинальный вид. Чтобы игра запускалась без 3D,
укажите:

```ini
[3d]
enabled=0
```

## Сборка из исходников

Нужны CMake, Ninja, компонент C++ из Visual Studio, Python 3.12+ и файлы
Fallout 2. Скрипт сборки берёт локальные MSVC и Windows SDK, если они есть;
иначе находит установленную Visual Studio через `vswhere`.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release
ctest --test-dir build --output-on-failure
py -3 -m pip install -r tools/scene_export/requirements.txt
py -3 -m unittest discover -s tools/scene_export -p 'test*.py' -v
py -3 tools/scene_export/export.py --game-root ./game --maps klamath
```

Файлы игры и собранные данные сцены остаются у вас локально. Подробнее:
[настройка игры](docs/game-setup.md),
[технический план](docs/2026-10-17-fallout2-ce-3d-env-p0-p1.md) и
[3D-дизайн](docs/2026-10-17-fallout2-sprite-lighting-design.md) (на английском).

## Благодарности

- **Interplay и Black Isle**: за Fallout 2.
- **Alexander Batalov** ([fallout2-ce](https://github.com/alexbatalov/fallout2-ce)):
  за Fallout 2 Community Edition, без которого этого проекта не было бы.
- **Сообщество [fallout2-ce](https://github.com/fallout2-ce/fallout2-ce)**:
  за развитие движка, исправления, поддержку sfall и модов.
- **[Apolotary](https://github.com/Apolotary/fallout2-first-person)**:
  за fallout2-first-person. Оттуда взят подход к восстановлению объёма карт и
  часть конвертера.

## Лицензия

Код движка и конвертера распространяется на условиях исходных проектов:
Sustainable Use License 1.0 (см. `ce/LICENSE.md` и лицензии в
`tools/scene_export/`). Это неофициальный некоммерческий фанатский проект.
