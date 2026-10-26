# MarathonRecomp-Android v0.1.5

Experimental support for Mali GPUs, ported from [UnleashedRecomp-Android](https://github.com/SansNope/UnleashedRecomp-Android). ARM64, Android 10 (API 29) or newer, Vulkan. Game files are imported from your own Xbox 360 copy.

> [!IMPORTANT]
> **Mali support needs testing.** It has not been run on any Mali device yet. If you have a phone with a Mali GPU, please try this release and open an [issue](https://github.com/SansNope/MarathonRecomp-Android/issues/new/choose) with your phone, GPU, Android version and `log.txt` — whether the game works or not.

## Changes

- **GPUs without BC textures (Mali, PowerVR):** the game's BC (DXT) textures are converted to ETC2/EAC on the CPU when they load (bcdec + etcpak), or to RGBA if the GPU has no ETC2. Loading takes longer on these GPUs.
- **Vulkan 1.2 feature path:** descriptor indexing, scalar block layout, buffer device address and mirror-clamp sampling are enabled through the Vulkan 1.2 core features, so drivers that do not list the old extension names (such as stock Mali) are accepted. A GPU without the required features now stops with a clear message in `log.txt`.
- Texture/sampler descriptor sets are limited to what the device reports; `MIRROR_ONCE` sampling falls back to `MIRRORED_REPEAT` when unsupported.
- Non-Adreno GPUs keep using the system Vulkan driver in **Auto** (unchanged).
- For testing on Adreno: an empty `bc_fallback.enabled` file in the game folder forces the ETC2 path; with the word `decode` inside it forces the RGBA path.
- Version 0.1.5 (code 4), same signing key as v0.1.0: installs over v0.1.0 and keeps your data.

## Tested

- **POCO F5 / Snapdragon 7+ Gen 2 / Adreno 725 / Android 17:** v0.1.5 starts and plays normally (the Vulkan 1.2 path is also used by Turnip on Adreno).
- **Mali:** not tested before the release. In UnleashedRecomp-Android, Valhall GPUs work (confirmed there: Mali-G57 MC2, G68 MP5, G77 MC9 and newer); Bifrost GPUs (G31, G51, G52, G71, G72, G76) are not supported. *Correction: the first version of these notes wrongly listed G57, G68 and G77 as Bifrost.*
- **Update:** the first reports from two Helio G99 devices (Mali-G57 MC2) show that the game does not start on them yet (a crash in the Mali driver / failed pipeline creation). This is being investigated; see [STATUS](https://github.com/SansNope/MarathonRecomp-Android/blob/main/docs/android/STATUS.md).
- realme RMX5090 (Snapdragon 8 Elite) was tested with v0.1.0, not yet with v0.1.5.

## Known issues

- Adreno 610 (Snapdragon 680): broken 2D title/menu graphics reported with the default driver ([#2](https://github.com/SansNope/MarathonRecomp-Android/issues/2)).
- Scene transitions can briefly lower FPS. Full list: [STATUS](https://github.com/SansNope/MarathonRecomp-Android/blob/main/docs/android/STATUS.md).
- We do not promise that the port works on every device.

## Credits

Mali support and the texture fallback come from UnleashedRecomp-Android 1.5.0. Texture conversion uses [bcdec](https://github.com/iOrange/bcdec) (MIT/Unlicense) and [etcpak](https://github.com/wolfpld/etcpak) (BSD-3-Clause). Full credits: [INFO](https://github.com/SansNope/MarathonRecomp-Android/blob/main/INFO.md), [third-party notices](https://github.com/SansNope/MarathonRecomp-Android/blob/main/THIRD_PARTY_NOTICES.md).

---

## Русский

Экспериментальная поддержка GPU Mali, перенесённая из [UnleashedRecomp-Android](https://github.com/SansNope/UnleashedRecomp-Android). ARM64, Android 10 (API 29) и новее, Vulkan. Файлы игры импортируются из своей копии для Xbox 360.

> [!IMPORTANT]
> **Поддержку Mali нужно протестировать.** Ни на одном устройстве с Mali она ещё не запускалась. Если у вас телефон с GPU Mali, попробуйте этот релиз и откройте [issue](https://github.com/SansNope/MarathonRecomp-Android/issues/new/choose) с моделью телефона, GPU, версией Android и `log.txt` — и если игра работает, и если нет.

**Изменения:**

- **GPU без текстур BC (Mali, PowerVR):** текстуры игры в формате BC (DXT) при загрузке перекодируются на CPU в ETC2/EAC (bcdec + etcpak), а если GPU не поддерживает ETC2 — распаковываются в RGBA. Загрузка на таких GPU дольше.
- **Путь Vulkan 1.2:** descriptor indexing, scalar block layout, buffer device address и mirror clamp включаются через возможности ядра Vulkan 1.2, поэтому подходят драйверы, которые не перечисляют старые имена расширений (например, стоковый Mali). GPU без нужных возможностей теперь завершает запуск с понятным сообщением в `log.txt`.
- Наборы дескрипторов текстур и сэмплеров ограничены лимитами устройства; `MIRROR_ONCE` заменяется на `MIRRORED_REPEAT`, если не поддерживается.
- GPU не от Adreno в режиме **«Авто»** по-прежнему используют системный драйвер Vulkan.
- Для проверки на Adreno: пустой файл `bc_fallback.enabled` в папке игры включает путь ETC2, со словом `decode` внутри — путь RGBA.
- Версия 0.1.5 (код 4), тот же ключ подписи, что у v0.1.0: ставится поверх v0.1.0 с сохранением данных.

**Проверено:** POCO F5 (Adreno 725, Android 17) — v0.1.5 запускается и играется нормально. **Mali не проверялся.** В UnleashedRecomp-Android поколение Valhall работает (там подтверждены Mali-G57 MC2, G68 MP5, G77 MC9 и новее); Bifrost (G31, G51, G52, G71, G72, G76) не поддерживается. *Исправление: в первой версии этого текста G57, G68 и G77 ошибочно названы Bifrost.* **Обновление:** первые отчёты с двух устройств на Helio G99 (Mali-G57 MC2) — игра на них пока не запускается (падение в драйвере Mali / ошибки создания конвейеров); разбираемся, подробности в [STATUS](https://github.com/SansNope/MarathonRecomp-Android/blob/main/docs/android/STATUS.ru.md). realme RMX5090 (Snapdragon 8 Elite) проверялся на v0.1.0, на v0.1.5 ещё нет.

**Известные проблемы:** Adreno 610 (Snapdragon 680) — сломанная 2D-графика титульного экрана и меню со встроенным драйвером ([#2](https://github.com/SansNope/MarathonRecomp-Android/issues/2)); просадки FPS при переходах между сценами. Мы не обещаем работу на всех устройствах.
