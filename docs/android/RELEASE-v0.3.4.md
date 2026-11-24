# MarathonRecomp-Android v0.3.4

**Vulkan compatibility update for Mali.** Shaders now read buffer addresses without requiring `shaderInt64`. This addresses a startup blocker on drivers that expose Vulkan buffer device addresses without 64-bit shader integers, including some Mali-G52 and G76 drivers.

- Corrects Vulkan array-texture bindings and handles fixed descriptor allocation failures.
- Specializes software clip planes when creating graphics pipelines so the fragment shader can omit clip-plane discard when clipping is disabled.
- Queries MSAA support for the formats the renderer uses and adds Vulkan capability details to `log.txt`.

The APK is for Android 10 or later, ARM64 and Vulkan. Device verification for this release is pending. Reports of missing scene graphics on Helio G200 in v0.3.3 remain under investigation. If the issue persists in v0.3.4, please include the scene, graphics settings and a fresh `log.txt` in your report. [Report an issue](https://github.com/SansNope/MarathonRecomp-Android/issues)

Thanks to Gemini 4 Argon and Carbon for the Mali Vulkan analysis.

[Full source diff](https://github.com/SansNope/MarathonRecomp-Android/compare/v0.3.3...v0.3.4)

---

## Русский

**Обновление совместимости Vulkan на Mali.** Шейдеры теперь читают адреса буферов без требования `shaderInt64`. Это устраняет препятствие для запуска на драйверах, которые поддерживают адреса буферов Vulkan без 64-битных целых в шейдерах, в том числе на части Mali-G52 и G76.

- Исправлена привязка многослойных текстур Vulkan и обработка отказов выделения фиксированных дескрипторов.
- Состояние программной плоскости отсечения учитывается при создании конвейера, чтобы компилятор мог убрать отбрасывание фрагментов по выключенной плоскости.
- Поддержка MSAA проверяется для используемых форматов рендера. В `log.txt` пишутся дополнительные возможности Vulkan.

APK предназначен для Android 10 и новее, ARM64 и Vulkan. Проверка этой версии на устройствах ещё впереди. Сообщения о пропавшей графике на Helio G200 в v0.3.3 остаются на проверке. Если проблема повторится в v0.3.4, приложите свежий `log.txt`, название сцены и настройки графики. [Сообщить о проблеме](https://github.com/SansNope/MarathonRecomp-Android/issues)

Спасибо Gemini 4 Argon и Carbon за разбор Vulkan на Mali.

[Полный список изменений](https://github.com/SansNope/MarathonRecomp-Android/compare/v0.3.3...v0.3.4)
