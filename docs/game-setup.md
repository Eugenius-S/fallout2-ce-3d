# Подготовка игры в `game/`

С 2026-10-18 игра для разработки — оригинальная Fallout 2 из Steam
плюс Restoration Project (RPU) от BGforge с русским переводом.

## Источник

- RPU v2.4.34, https://github.com/BGforgeNet/Fallout2_Restoration_Project/releases/tag/v2.4.34
- `rpu_v2.4.34.zip` — SHA-256 `b8f8a9c900a553c4e781e74bcf805f5663bc37b3633ebfbebfc928b70470285e`
- `rpu_v2.4.34_translations.zip` — SHA-256 `cc9ca1e12e477cbf4cd0156097d4b9185f370033e5ac8ce9c4ef16f741b45644`
- CE в своём README заявляет полную поддержку Restoration Project.

## Шаги

1. Скопировать Steam-установку в `game/` копированием, а не жёсткими
   ссылками: RP перемещает файлы, и Steam-установка должна остаться
   нетронутой.
2. Положить в `game/` собранный `fallout2-ce.exe` и `fallout2.cfg` проекта.
3. Распаковать `rpu_v2.4.34.zip` в `game/` с перезаписью и запустить
   `bash ./rpu-install.sh` из `game/`. Скрипт переносит `patch000.dat`,
   старый `data/` и `f2_res.dat` в `backup/rpu/`.
4. Из архива переводов скопировать в `game/mods/` файлы `rpu_russian.dat`
   и `upu_russian_sound.dat`. Оба уже перечислены в `mods/mods_order.txt`.
5. В `game/fallout2.cfg` выставить `language=russian` и
   `f2_res_dat=mods\f2_res.dat`.
6. Пересобрать сцену: `py -3 tools/scene_export/export.py --game-root ./game`.

RP требует новую игру: сохранения с оригинала не подходят. Dev-запуск
по умолчанию начинает новую игру с первым готовым персонажем
(`--dev-new-game`), см. [p1-devtools.md](p1-devtools.md).

Копия игры без RP лежит в `game-vanilla/` (тоже вне Git) для сравнения.
