# MarathonRecomp-Android v0.1.6

**The game now starts on Mali GPUs.** ARM64, Android 10 (API 29) or newer, Vulkan. Game files are imported from your own Xbox 360 copy.

> [!IMPORTANT]
> **Mali support is still early.** It works on the Mali devices we could test, but performance on Mali is not good yet — we will keep working on it. We do not promise that the port works on every device. Please report problems in an [issue](https://github.com/SansNope/MarathonRecomp-Android/issues/new/choose) with your phone, GPU and `log.txt`.

## Changes

Found with Mali testers (Mali-G57 MC2 on Helio G99, Mali-G710 on Pixel 7 Pro):

- **Startup crash on older Mali drivers fixed.** Marathon used 5 descriptor sets, while these drivers allow 4. On such GPUs the conditional-survey buffer now shares the sampler set, and shaders are remapped when they load.
- **Driver crash while compiling shaders, and broken graphics, fixed.** Every vertex shader of the game writes `ClipDistance` (Xbox user clip planes), which Mali-G57 and G710 do not support. On GPUs without `shaderClipDistance` it is removed from the shaders when they load. **User clip planes are then disabled**; we have not seen a visible difference yet.
- **2D textures read as 2D arrays** now get a proper 2D-array view (Vulkan requires the view type to match the shader).
- A broken graphics pipeline is no longer handed to the driver.
- **Diagnostics in `log.txt`:** GPU, driver and Vulkan version, limits and features; initialization steps; the exact pipeline that fails or crashes; a call stack in crash reports; the app's own logcat lines after the first pipeline failure.
- **Timing summary on by default:** the first launch creates `perf_capture.enabled` in the game folder, so `log.txt` contains a `[perf]` line every 2 seconds. This helps us compare performance across devices. Delete the file to turn it off; it is not created again.
- Version 0.1.6 (code 5), same signing key as v0.1.0/v0.1.5: installs over them and keeps your data.

## Tested

- **Pixel 7 Pro / Mali-G710 / Android 17:** starts and plays (Wave Ocean). Strong slowdowns in the orca chase; not measured yet.
- **Helio G99 / Mali-G57 MC2** (Samsung Galaxy Tab A9 / Android 15 and an OPPO phone / Android 16): starts and plays, no graphics artifacts after the fixes. Strong slowdowns reported; not measured yet.
- **POCO F5 / Adreno 725:** the same changes were checked with every Mali workaround forced on (test markers), picture correct and no errors.
- Tested with development builds of this version; realme RMX5090 (Snapdragon 8 Elite) has not been re-tested since v0.1.0.

## Known issues

- **Performance on Mali** — slow, especially in heavy scenes. To be investigated with the new `[perf]` logs.
- **Frame-rate cap below 60** may slow down scripted scenes such as the orca chase (suspected: parts of the game logic are tied to the frame). On weak devices try a 60 FPS cap with a lower render scale instead.
- **First loads on Mali are slower** — textures are converted to ETC2 on the CPU.
- **Adreno 610 (Snapdragon 680)** — broken 2D graphics on the title screen ([#2](https://github.com/SansNope/MarathonRecomp-Android/issues/2)).
- Bifrost Mali GPUs (G31, G51, G52, G71, G72, G76) are not supported.
- Full list: [STATUS](https://github.com/SansNope/MarathonRecomp-Android/blob/main/docs/android/STATUS.md).

Thanks to the Mali testers for the logs and screenshots.

---

## Русский

**Игра запускается на GPU Mali.** ARM64, Android 10 (API 29) и новее, Vulkan. Файлы игры импортируются из своей копии для Xbox 360.

> [!IMPORTANT]
> **Поддержка Mali пока ранняя.** На проверенных устройствах Mali игра работает, но производительность пока плохая — мы продолжим над ней работать. Мы не обещаем работу на всех устройствах. О проблемах пишите в [issue](https://github.com/SansNope/MarathonRecomp-Android/issues/new/choose) с моделью телефона, GPU и `log.txt`.

**Изменения** (найдено вместе с тестировщиками Mali: Mali-G57 MC2 на Helio G99 и Mali-G710 на Pixel 7 Pro):

- **Исправлено падение при запуске на старых драйверах Mali.** Марафон использовал 5 наборов дескрипторов, а эти драйверы разрешают 4. На таких GPU буфер conditional survey перенесён в набор сэмплеров, шейдеры перепривязываются при загрузке.
- **Исправлены падение драйвера при компиляции шейдеров и сломанная графика.** Все вершинные шейдеры игры пишут `ClipDistance` (пользовательские плоскости отсечения Xbox), а Mali-G57 и G710 это не поддерживают. На GPU без `shaderClipDistance` он убирается из шейдеров при загрузке. **Пользовательские плоскости отсечения при этом выключены**; видимой разницы пока не замечено.
- **2D-текстуры, которые читаются как массив,** получают отдельный вид «массив 2D» (Vulkan требует совпадения типа вида с шейдером).
- Сломанный графический конвейер больше не передаётся драйверу.
- **Диагностика в `log.txt`:** GPU, драйвер и версия Vulkan, лимиты и возможности; шаги инициализации; какой именно конвейер не создаётся или роняет драйвер; стек вызовов в отчёте о сбое; строки logcat приложения после первой ошибки конвейера.
- **Сводка производительности включена по умолчанию:** при первом запуске в папке игры создаётся `perf_capture.enabled`, и в `log.txt` раз в 2 секунды пишется строка `[perf]`. Это поможет сравнивать производительность на разных устройствах. Чтобы выключить — удалите файл, повторно он не создаётся.
- Версия 0.1.6 (код 5), тот же ключ подписи: ставится поверх v0.1.0/v0.1.5 с сохранением данных.

**Проверено:** Pixel 7 Pro (Mali-G710, Android 17) — запускается и играется (Wave Ocean), сильные просадки в погоне косатки, не измерялись. Helio G99 / Mali-G57 MC2 (Samsung Galaxy Tab A9 / Android 15 и телефон OPPO / Android 16) — запускается и играется, артефактов графики после исправлений нет; сообщают о сильных тормозах, не измерялись. POCO F5 (Adreno 725) — те же изменения проверены с принудительно включёнными обходами Mali, картинка правильная, ошибок нет. Проверялись тестовые сборки этой версии; realme RMX5090 (Snapdragon 8 Elite) с v0.1.0 повторно не проверялся.

**Известные проблемы:** производительность на Mali (будем разбирать по новым логам `[perf]`); лимит FPS ниже 60 может замедлять скриптовые сцены вроде погони косатки (предположение: часть логики игры привязана к кадру) — на слабых устройствах лучше лимит 60 и меньший масштаб рендера; первые загрузки на Mali медленнее из-за перекодирования текстур; Adreno 610 — сломанная 2D-графика ([#2](https://github.com/SansNope/MarathonRecomp-Android/issues/2)); Mali поколения Bifrost (G31, G51, G52, G71, G72, G76) не поддерживаются.

Спасибо тестировщикам Mali за логи и скриншоты.
