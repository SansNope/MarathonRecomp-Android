# MarathonRecomp-Android v0.3.3

**Mali Vulkan compatibility.** This release adds a fixed-descriptor rendering path for drivers that lack the features required by the bindless path. It targets startup failures such as those reported on the Infinix HOT 40 with Mali-G52.

- Orders vertex and index buffer updates on Mali, snapshots dynamic texture uploads, and decodes guest BC1/BC3 textures when the GPU lacks BC support. These changes address hazards that can affect videos and loading screens.
- Fixes the conditional visibility query used by effects such as lens flares. Adds fragment-shader clip planes when the driver lacks native clip distances and has enough shader inputs and outputs.
- Skips extracting the bundled Adreno drivers in Auto mode when the device is known to use another GPU.

Startup was verified on **POCO F5 (Adreno 725, Turnip)**. Testing on Mali is pending. If you try this release on Mali, please include your device model, Android version, and `log.txt` in your report. Tell us whether you reached the title screen and loaded a level, and whether videos or loading screens flicker. [Report an issue](https://github.com/SansNope/MarathonRecomp-Android/issues)

Version **0.3.3 (code 21)** requires Android 10 or later, ARM64, and Vulkan. Thanks to Gemini 4 Argon for the Mali compatibility audit.

[Full source diff](https://github.com/SansNope/MarathonRecomp-Android/compare/v0.3.2.2...v0.3.3)

---

## Русский

**Совместимость Vulkan с Mali.** В этой версии появился путь рендера с фиксированными дескрипторами для драйверов, которым не хватает возможностей bindless. Он предназначен для случаев отказа запуска, как в отчёте с Infinix HOT 40 на Mali-G52.

- Упорядочено обновление вершинных и индексных буферов на Mali. При загрузке динамических текстур сохраняется снимок данных, а гостевые BC1/BC3 декодируются на GPU без поддержки BC. Эти изменения устраняют проблемы синхронизации, которые могут проявляться в роликах и на экранах загрузки.
- Исправлен запрос видимости для эффектов вроде бликов солнца. Для драйверов без аппаратных плоскостей отсечения добавлено отсечение во фрагментном шейдере, если хватает лимитов ввода и вывода шейдеров.
- В режиме Auto встроенные драйверы Adreno не распаковываются при обнаружении другого GPU.

Запуск подтверждён на **POCO F5 (Adreno 725, Turnip)**. На Mali ждём отзывов. Если попробуете эту версию, приложите модель устройства, версию Android и `log.txt`. Укажите, дошли ли до титульного экрана и загрузили ли уровень, а также мерцают ли ролики или экраны загрузки. [Сообщить о проблеме](https://github.com/SansNope/MarathonRecomp-Android/issues)

Версия **0.3.3 (код 21)** требует Android 10 или новее, ARM64 и Vulkan. Спасибо Gemini 4 Argon за аудит совместимости с Mali.

[Полный список изменений](https://github.com/SansNope/MarathonRecomp-Android/compare/v0.3.2.2...v0.3.3)
