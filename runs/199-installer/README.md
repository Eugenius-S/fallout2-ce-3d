# 199: Proper installer

Live Windows evidence for [task 199](../../tasks/199-installer.md). All game
operations ran on a separate copy of the game data at
a separate scratch directory outside the repository; the installed game and cache in the
owner's checkout were not used.

## Screenshots

- `interface-en.jpg` and `interface-ru.jpg`: the running installer, release
  version and short notes in each UI language.
- `install-cycle.jpg`: three live frames from one test copy: first install,
  automatic update on the next launch, and rollback to the previous build.

## Checks

- MSVC Release build: `fallout2-ce-3d-launcher.exe`, SHA-256
  `FA6A4D82606C7009C3932D5957A860DEF99B9BE9F0FA12AA78098CA9361FBF85`.
- CTest: 20/20 passed. The standalone scene exporter build also succeeded.
- A loopback release served test builds `test-build-a` and `test-build-b`.
  The installer completed a clean install, updated on launch with the setting
  enabled, and restored the previous version. It generated the seven Klamath
  scene groups in the test copy.
- With an isolated `%APPDATA%`, the first run selected Russian to match the
  Windows UI locale (`ru-RU`, language id 1049), and detected an existing
  Steam install without changing it. A separate settings file confirmed that
  the language and automatic-update choice persist between launches.
- The Russian option fetched the official RPU v2.4.34 translation archive
  directly from its upstream release. The download was 140,656,143 bytes;
  `rpu_russian.dat` (6,405,961 bytes) was installed in the scratch copy,
  `fallout2.cfg` was set to `language=russian`, and
  `mods/mods_order.txt` contains `rpu_russian.dat`.

The RPU repository has no GitHub license metadata and no root `LICENSE` file,
so this change does not assume redistribution rights. The launcher does not
bundle or rehost the file; it fetches it from the [official RPU
release](https://github.com/BGforgeNet/Fallout2_Restoration_Project/releases/tag/v2.4.34).
The RPU project's credits list its Russian translators. The upstream
[translation notes](https://github.com/BGforgeNet/Fallout2_Unofficial_Patch/blob/master/docs/translations.md)
say the UPU Russian text is based on 1C and Drobovik & Co.; that page covers a
different package and is not treated as a license for the RPU asset.
