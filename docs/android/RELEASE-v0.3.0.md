# MarathonRecomp-Android v0.3.0

Two optional settings are available under **Graphics** in the Android launcher. Both default to off. Select **Play** to save them and fully restart the game to apply changes.

- **Low graphics** caps shadows at the 512 setting, reduces objects rendered into distant shadow maps, limits anisotropic filtering to 2×, and disables real-time reflections and radial blur.
- **Reduced output resolution (75%)** scales each side of the final Vulkan output to 75% of the window, including the HUD. Use it independently of Low graphics. The HUD may look softer at this setting.

The test2 build was checked in gameplay on **POCO F5 (Adreno 725)** with both switches enabled. The log confirmed real-time reflections were skipped and the game rendered at 1800×810 in a 2400×1080 window. This release uses the same tested native library. Performance and compatibility on Mali still need device testing. If you try it on Mali, please include your device model, graphics settings and `log.txt` with feedback.

Android 10 or newer, ARM64 and Vulkan. Version **0.3.0 (code 16)** uses the existing package and release signing key.

Based on [MarathonRecomp by SonicNext Dev and contributors](https://github.com/sonicnext-dev/MarathonRecomp) and [Plume by renderbag and contributors](https://github.com/renderbag/plume). [Full source diff](https://github.com/SansNope/MarathonRecomp-Android/compare/v0.2.9...v0.3.0)

---

## Русский

В разделе **«Графика»** Android-лаунчера появились две настройки. Обе выключены по умолчанию. Нажмите **«Играть»** для сохранения настроек и полностью перезапустите игру, чтобы применить их.

- **Облегчённая графика** устанавливает разрешение теней 512, уменьшает число объектов в дальних картах теней, ограничивает анизотропную фильтрацию до 2× и отключает динамические отражения и радиальное размытие.
- **Уменьшенное разрешение вывода (75%)** уменьшает каждую сторону итогового буфера Vulkan до 75% от размера окна вместе с интерфейсом. Настройку можно включать отдельно от облегчённой графики. Интерфейс при этом может выглядеть менее чётким.

Сборку test2 проверили в игре на **POCO F5 (Adreno 725)** с обеими настройками. Лог подтвердил отключение динамических отражений и вывод 1800×810 при окне 2400×1080. В релиз вошла та же нативная библиотека. Производительность и совместимость с Mali ещё предстоит проверить. Если попробуете на Mali, приложите к отзыву модель устройства, настройки графики и `log.txt`.

Требуются Android 10 или новее, ARM64 и Vulkan. Версия **0.3.0 (код 16)** использует прежние пакет и ключ подписи.

В основе порта [MarathonRecomp команды SonicNext Dev и участников проекта](https://github.com/sonicnext-dev/MarathonRecomp) и [Plume от renderbag и участников проекта](https://github.com/renderbag/plume). [Полный список изменений в исходниках](https://github.com/SansNope/MarathonRecomp-Android/compare/v0.2.9...v0.3.0)
