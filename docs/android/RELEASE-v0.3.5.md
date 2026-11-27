# MarathonRecomp-Android v0.3.5

This update broadens Vulkan compatibility, reduces rendering and loading work, and adds tools for checking game files and sharing diagnostics.

### Vulkan compatibility and graphics

- Added shader constants backed by a buffer for drivers without buffer device addresses or scalar block layout. Vulkan can also select a compact fixed-descriptor layout and fit shader inputs and outputs within smaller driver limits.
- Added a D24+S8 depth fallback, point filtering for formats without linear filtering, and version-aware initialization for Vulkan 1.0 and 1.1 drivers.
- Restored hardware user clip planes where the driver supports them, including on the tested Adreno. The clip-distance workaround from v0.3.4.1 remains active by default on devices without hardware support.
- Cascaded shadows render directly into the shadow texture layers, avoiding up to four depth-copy passes per frame. The final gamma pass, ImGui rendering, shader constant uploads, and draw preparation also do less work. Experimental Vulkan copy and clear paths can be enabled with diagnostic markers.

### Loading, CPU and display

- Loading screens are capped at 60 FPS when the gameplay limit is higher. During loading, small DDS uploads can share a staging buffer and a Vulkan queue submission. If you encounter texture problems during loading, you can disable multi-texture batching with `dds_multi_batch.disabled` in the game folder.
- Two frequently called guest math routines use native ARM64 NEON. The tested Group 1 PPC optimizations are enabled by default for Android builds.
- Android requests a display refresh rate that suits the configured FPS cap and waits for the FPS slider adjustment to finish. Reduced routine SpinDash logging. Steam Controller BLE polling is opt-in.

### Launcher and upstream changes

- **Game files → Verify game files** checks the base game and installed DLC against reference hashes. **Advanced → Share log** shares a ZIP containing logs, configuration and diagnostics. **About → Licenses** shows third-party notices and license texts.
- Backported pedestrian animation LOD controls, a fix for stalls when spawning pedestrians, direct cascaded shadows, and a fix for applying master volume twice. [Original MarathonRecomp #476](https://github.com/sonicnext-dev/MarathonRecomp/pull/476) · [#478](https://github.com/sonicnext-dev/MarathonRecomp/pull/478) · [#479](https://github.com/sonicnext-dev/MarathonRecomp/pull/479) · [#503](https://github.com/sonicnext-dev/MarathonRecomp/pull/503)

Requires Android 10 or newer, ARM64 and Vulkan. A pre-release build passed a full stage on POCO F5 (Adreno 725). Device testing for the final loading changes and performance measurements on Mali are pending. [Report an issue](https://github.com/SansNope/MarathonRecomp-Android/issues) · [Full source diff](https://github.com/SansNope/MarathonRecomp-Android/compare/v0.3.4.1...v0.3.5)

---

## Русский

Обновление расширяет совместимость с Vulkan, уменьшает объём работы при отрисовке и загрузке и добавляет инструменты проверки файлов игры и отправки диагностики.

### Совместимость Vulkan и графика

- На драйверах без адресации буферов или scalar block layout шейдеры могут получать константы из буфера. Vulkan также выбирает компактную раскладку фиксированных дескрипторов и уплотняет входы и выходы шейдеров при низких лимитах драйвера.
- Добавлены запасной формат глубины D24+S8, точечная фильтрация для форматов без поддержки линейной и выбор функций инициализации для Vulkan 1.0 и 1.1.
- Восстановлены аппаратные плоскости отсечения на драйверах с их поддержкой, в том числе на проверенном Adreno. На устройствах без аппаратной поддержки по умолчанию действует обход из v0.3.4.1.
- Каскады теней рисуются напрямую в слои текстуры, что убирает до четырёх копирований глубины за кадр. Сокращена работа в финальном гамма-проходе, отрисовке ImGui, загрузке констант шейдеров и подготовке команд отрисовки. Экспериментальные пути копирования и очистки Vulkan включаются диагностическими маркерами.

### Загрузка, CPU и экран

- Если игровой лимит выше 60 FPS, экран загрузки работает с лимитом 60 FPS. При загрузке уровней несколько небольших DDS-текстур могут отправляться на GPU одним пакетом. При проблемах с текстурами пакетную загрузку нескольких текстур можно отключить маркером `dds_multi_batch.disabled` в папке игры.
- Две часто вызываемые гостевые математические функции используют нативный ARM64 NEON. Проверенные PPC-оптимизации Group 1 включены по умолчанию в сборках Android.
- Android подбирает герцовку под лимит FPS и применяет новое значение после завершения настройки ползунка. Убраны регулярные записи SpinDash в лог. Опрос Steam Controller по BLE включается отдельно.

### Лаунчер и переносы из апстрима

- **Файлы игры → Проверить целостность** сверяет файлы базовой игры и установленных DLC с эталонными хешами. **Дополнительно → Поделиться логом** отправляет ZIP с логами, настройками и диагностикой. **Инфо → Лицензии** показывает уведомления и тексты лицензий сторонних компонентов.
- Из оригинального MarathonRecomp перенесены настройка LOD анимаций пешеходов, устранение задержек при их появлении, прямой рендер каскадов теней и исправление двойного применения общей громкости. [MarathonRecomp #476](https://github.com/sonicnext-dev/MarathonRecomp/pull/476) · [#478](https://github.com/sonicnext-dev/MarathonRecomp/pull/478) · [#479](https://github.com/sonicnext-dev/MarathonRecomp/pull/479) · [#503](https://github.com/sonicnext-dev/MarathonRecomp/pull/503)

Требуются Android 10 или новее, ARM64 и Vulkan. На предрелизной сборке прошли полный уровень на POCO F5 (Adreno 725). Проверка последних правок загрузки на устройстве и замеры производительности на Mali ещё впереди. [Сообщить о проблеме](https://github.com/SansNope/MarathonRecomp-Android/issues) · [Полный список изменений](https://github.com/SansNope/MarathonRecomp-Android/compare/v0.3.4.1...v0.3.5)

Подготовлено при участии OpenAI Codex и Gemini. Thanks to the MarathonRecomp contributors and Android testers.
