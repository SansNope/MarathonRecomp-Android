# MarathonRecomp-Android v0.2.6

**Three fixes ported from the original MarathonRecomp.** ARM64, Android 10 (API 29) or newer, Vulkan. Installs over v0.2.5 or 0.2.6-test1 and keeps your saves. Version **0.2.6 (code 12)**, same signing key.

## Changes and upstream credits

The original fixes were developed in **[sonicnext-dev/MarathonRecomp](https://github.com/sonicnext-dev/MarathonRecomp)** by **[squidbus](https://github.com/squidbus)** and **[Hyper (hyperbx)](https://github.com/hyperbx)**. This release brings their work to the Android port; the original authorship and license notices are preserved.

- **ImGui pointer events:** prevent queued pointer positions from receiving the viewport transformation more than once across frames. By **squidbus** — [PR #412](https://github.com/sonicnext-dev/MarathonRecomp/pull/412), [commit 1248d2d](https://github.com/sonicnext-dev/MarathonRecomp/commit/1248d2d8d71329ccef76f973c4b3a00a977ed571).
- **Results-screen render script:** avoid reloading the already active render script when buffer recreation is not required, removing a source of pauses around the results screen. By **Hyper** — [PR #374](https://github.com/sonicnext-dev/MarathonRecomp/pull/374), [commit 397afe2](https://github.com/sonicnext-dev/MarathonRecomp/commit/397afe225cbdf33299afd0bdfc12cd2c3236ed92). The Android adaptation also checks the port's deferred buffer-recreation state so required reloads are preserved. A reduction in results-screen loading time has not been measured.
- **Post-processing after buffer recreation:** preserve the active Postprocess render process when the render script is reloaded. Upstream's example is End of the World's phase-shift filter. By **Hyper** — [PR #376](https://github.com/sonicnext-dev/MarathonRecomp/pull/376), [commit d526d93](https://github.com/sonicnext-dev/MarathonRecomp/commit/d526d93902ff8f4f69363932d213b71410830885). Only the functional fix was ported; the unrelated API mappings in that PR were not needed.

## Tested

- **POCO F5 (Adreno 725, Android 17):** the user tested **0.2.6-test1** and reported that the game works. The approximately **2 min 23 s** log contains no crash records or Vulkan/pipeline errors. Return from the background and a 180-degree rotation were recorded, with buffer recreation and rendering continuing afterward.
- **A725 Performance / Turnip sysmem**, render scale **45%**, cap **60 FPS**, performance capture and sampling enabled: **59.6 and 59.9 FPS** in two periodic summaries. The first window includes one 117.5 ms frame and a background pause of about two seconds; the second has no frames over 100 ms. These measurements do not establish a speed-up over v0.2.5.
- Native compilation and linking passed. Ten isolated checks of the render-script reload decision passed, including both Android resize flags; these are not gameplay tests.
- The release APK has the **same native libraries and assets** as the tested 0.2.6-test1 APK. The specific End of the World filter was not separately verified, and no new Mali or stock Qualcomm-driver validation was performed.

## Known issues

The known issues from [v0.2.1](https://github.com/SansNope/MarathonRecomp-Android/blob/main/docs/android/RELEASE-v0.2.1.md#known-issues) and [v0.2.0](https://github.com/SansNope/MarathonRecomp-Android/blob/main/docs/android/RELEASE-v0.2.0.md#known-issues) remain, including loading-screen and video flicker on Helio G99. The orca-tail softlock reported in [issue #5](https://github.com/SansNope/MarathonRecomp-Android/issues/5) remains under investigation; this release does not include a dedicated fix for it.

---

## Русский

**Три исправления из оригинального MarathonRecomp.** ARM64, Android 10 (API 29) и новее, Vulkan. Ставится поверх v0.2.5 или 0.2.6-test1 с сохранением данных. Версия **0.2.6 (код 12)**, тот же ключ подписи.

### Изменения и авторство оригинального проекта

Оригинальные исправления разработаны в **[sonicnext-dev/MarathonRecomp](https://github.com/sonicnext-dev/MarathonRecomp)** авторами **[squidbus](https://github.com/squidbus)** и **[Hyper (hyperbx)](https://github.com/hyperbx)**. В этом выпуске их работа перенесена в Android-порт; авторство и лицензионные уведомления оригинального кода сохранены.

- **Координаты ввода ImGui:** отложенные события указателя больше не проходят преобразование координат окна повторно между кадрами. Автор **squidbus** — [PR #412](https://github.com/sonicnext-dev/MarathonRecomp/pull/412), [коммит 1248d2d](https://github.com/sonicnext-dev/MarathonRecomp/commit/1248d2d8d71329ccef76f973c4b3a00a977ed571).
- **Рендер-скрипт экрана результатов:** уже активный скрипт не загружается повторно, если пересоздание буферов не требуется. Это устраняет один из источников пауз около экрана результатов. Автор **Hyper** — [PR #374](https://github.com/sonicnext-dev/MarathonRecomp/pull/374), [коммит 397afe2](https://github.com/sonicnext-dev/MarathonRecomp/commit/397afe225cbdf33299afd0bdfc12cd2c3236ed92). Android-адаптация учитывает и отложенное пересоздание буферов порта, сохраняя обязательные перезагрузки. Уменьшение времени ожидания экрана результатов не измерялось.
- **Постобработка после пересоздания буферов:** активный процесс Postprocess сохраняется при перезагрузке рендер-скрипта. Пример исправляемого эффекта в оригинальном проекте — phase shift в End of the World. Автор **Hyper** — [PR #376](https://github.com/sonicnext-dev/MarathonRecomp/pull/376), [коммит d526d93](https://github.com/sonicnext-dev/MarathonRecomp/commit/d526d93902ff8f4f69363932d213b71410830885). Перенесено само функциональное исправление; сопутствующие API-описания из PR для него не понадобились.

### Проверено

- **POCO F5 (Adreno 725, Android 17):** пользователь проверил **0.2.6-test1** и сообщил, что игра работает. В логе примерно за **2 мин 23 с** нет записей о сбоях и ошибках Vulkan/конвейеров. Зафиксированы возврат из фона и поворот на 180° с пересозданием буферов и продолжением вывода.
- **A725 Performance / Turnip sysmem**, масштаб **45%**, лимит **60 FPS**, включены запись производительности и профилирование: **59,6 и 59,9 FPS** в двух периодических сводках. В первом окне есть один кадр 117,5 мс и уход в фон примерно на две секунды; во втором кадров дольше 100 мс нет. Эти замеры не доказывают ускорение относительно v0.2.5.
- Нативная компиляция и линковка прошли. Пройдены десять изолированных проверок условий перезагрузки рендер-скрипта, включая оба Android-флага resize; это не игровые тесты.
- В релизном APK **те же нативные библиотеки и ресурсы**, что в проверенном APK 0.2.6-test1. Конкретный фильтр End of the World отдельно не проверялся; новых проверок Mali и системного драйвера Qualcomm не было.

### Известные проблемы

Сохраняются известные проблемы из [v0.2.1](https://github.com/SansNope/MarathonRecomp-Android/blob/main/docs/android/RELEASE-v0.2.1.md#known-issues) и [v0.2.0](https://github.com/SansNope/MarathonRecomp-Android/blob/main/docs/android/RELEASE-v0.2.0.md#known-issues), включая мерцание загрузочных экранов и роликов на Helio G99. Зависание около хвоста косатки из [issue #5](https://github.com/SansNope/MarathonRecomp-Android/issues/5) ещё разбирается; отдельного исправления этой проблемы в выпуске нет.
