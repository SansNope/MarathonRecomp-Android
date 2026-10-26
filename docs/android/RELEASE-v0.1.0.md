# MarathonRecomp-Android v0.1.0

First Android release of the unofficial MarathonRecomp port. ARM64, Android10/API29 or newer, Vulkan. Game resource files are supplied separately from your own Xbox360 copy.

## Included

- Sonic2006-style launcher with Russian/English text, the new icon and Info/credits.
- Touch controls and layout editor, game/DLC import, storage and graphics settings.
- Four bundled Turnip driver profiles, automatic selection and recovery after a failed driver start.
- XenonRecomp register-local optimization group1, Vulkan swapchain pre-rotation and topology-aware worker affinity.
- Render scale with native-resolution HUD, quieter loading logs, driver provisioning stamp and crash diagnostics.
- Non-debuggable Release packaging, versionName0.1.0/versionCode3 and dedicated signing key with the development-to-release certificate lineage.
- Third-party notices and license texts in APK `assets/licenses/`.

The native game libraries and bundled drivers are compared byte-for-byte with the owner-tested development APK `63b87707…` before publication. This release changes packaging/version/signing and adds license texts; it introduces no new native optimizations.

## Tested scope

- **POCO F5 / SM7475 / Adreno725 / Android17:** menus, HUB/events, a boss, Wave Ocean and Dusty Desert; sound, touch and saves. Saved measurements show about60FPS outside loading transitions at60% render scale.
- **realme RMX5090 / Snapdragon8Elite / Adreno830 / Android16:** HUB/events and a level at125% render scale, working surface recreation after backgrounding. Performance was reported as excellent by the tester; numerical FPS was not measured.
- The owner installed and checked the icon/Info development APK `63b87707…` and reported it working. The newly signed Release APK is verified as an artifact; its installation/gameplay must be distinguished from that development-APK test.

## Known limitations

- Loading transitions can briefly lower FPS; Vulkan has no persistent pipeline cache yet.
- Other GPU families, Android10–15, physical gamepads, DLC/full playthrough and long battery sessions are untested.
- Touch UI does not yet switch to a menu D-pad or a dedicated cutscene Skip button.
- The original Marathon MSAA option is not implemented; changing the menu value does not enable MSAA.
- Compatibility/performance on every Android device is not promised. This is an early release.

## Installation and verification

Install `MarathonRecomp-Android-v0.1.0-arm64-v8a.apk`, then use the launcher to import your game folder/ZIP. Keep Vulkan driver on Auto initially. SHA-256 is supplied in `SHA256SUMS`.

The package identity stays `io.github.sansnope.marathonrecomp.android`. The signed certificate lineage preserves the update path from the maintainer's development builds. Independently built APKs can use different certificates. If Android rejects an update, retain your data and report the error instead of uninstalling first.

Please attach `log.txt` to Android bug reports, with the phone, Android version, driver/render mode and reproduction steps. Credits: SansNope, Anthropic Opus5.5, ITSeny, MarathonRecomp and the upstream component authors, testers; launcher artwork by GSNReboot (GameBanana). Details: [INFO](../../INFO.md), [notices](../../THIRD_PARTY_NOTICES.md), [signing](../../android/signing/README.md).

---

## Русский

Первый Android-релиз неофициального порта MarathonRecomp. ARM64, Android10/API29+, Vulkan. Файлы игры нужно импортировать из своей копии версии Xbox360.

Основа — проверенная владельцем сборка `63b87707…`: лаунчер в стиле2006, иконка и «Инфо», сенсорное управление, Turnip, первая группа PPC-оптимизаций, pre-rotation, affinity по топологии и масштабирование сцены с HUD в полном разрешении. Нативные библиотеки и драйверы сверяются с ней побайтно. Для v0.1.0 изменены упаковка, версия, подпись и добавлены тексты лицензий.

На POCO F5 измерено около60FPS вне загрузок при масштабе60%; на realme8Elite проверены HUB/уровень и125% рендера, численный FPS не измерялся. Переходы между сценами могут давать просадки. Другие устройства/GPU, Android10–15, геймпады, DLC, полное прохождение и долгая игра от батареи пока не проверены. Релиз ранний, универсальная совместимость не обещается.

В APK выключена отладка, версия0.1.0/code3. Для отдельного release-ключа сохранена цепочка обновления от прежних сборок владельца. Проверка подписи/содержимого нового APK не заменяет проверку его установки на устройстве. Сохранения перед обновлением полезно скопировать; при отказе Android обновляться сначала сообщите ошибку, не удаляя приложение.
