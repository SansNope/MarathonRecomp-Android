# MarathonRecomp-Android v0.2.5

**Five fixes ported from the original MarathonRecomp.** ARM64, Android 10 (API 29) or newer, Vulkan. Installs over v0.2.4 or 0.2.5-test1 and keeps your saves. Version **0.2.5 (code 11)**, same signing key.

## Changes and upstream credits

These fixes were originally developed in **[sonicnext-dev/MarathonRecomp](https://github.com/sonicnext-dev/MarathonRecomp)** by the MarathonRecomp team and contributors. Credit for the original fixes belongs to **[squidbus](https://github.com/squidbus)** and **[Hyper (hyperbx)](https://github.com/hyperbx)**; this release brings their work to the Android port.

- **Basic Xbox profile settings:** return the profile data queried by the game, including the default Y-axis inversion value, instead of letting the game read beyond the result buffer. By **squidbus** — [PR #369](https://github.com/sonicnext-dev/MarathonRecomp/pull/369), [commit 038caf9](https://github.com/sonicnext-dev/MarathonRecomp/commit/038caf95d089581f80cbcbbfcfe159827423716f).
- **Audio client cleanup:** stop the audio thread and free its callback parameter when the game unregisters the audio client. By **squidbus** — [PR #370](https://github.com/sonicnext-dev/MarathonRecomp/pull/370), [commit d1155c9](https://github.com/sonicnext-dev/MarathonRecomp/commit/d1155c945c2953b66ac905d45c4352edffac1082).
- **Disabled anisotropic filtering:** set the sampler's maximum anisotropy to 1 instead of 16 when anisotropic filtering is off. A visible picture change is not expected. By **squidbus** — [PR #349](https://github.com/sonicnext-dev/MarathonRecomp/pull/349), [commit 2bb8017](https://github.com/sonicnext-dev/MarathonRecomp/commit/2bb8017ac50029aea3568f7f02cd2cfc624896d2).
- **Render-thread ownership:** wait for the next command list before another thread takes over rendering. Upstream describes this as a potential fix for random crashes on the first frame of loading screens. By **Hyper**, with **squidbus** credited as co-author — [PR #394](https://github.com/sonicnext-dev/MarathonRecomp/pull/394), [commit 9b19b00](https://github.com/sonicnext-dev/MarathonRecomp/commit/9b19b0070fae6118a9da48d56a4d33ea94537467). The upstream fix reimplements an earlier [UnleashedRecomp change](https://github.com/hedge-dev/UnleashedRecomp/pull/1486).
- **Remove redundant fullscreen vertices:** draw three vertices instead of six in the final gamma/rotation pass and the color-copy pass, avoiding a redundant triangle. By **Hyper** — [PR #396](https://github.com/sonicnext-dev/MarathonRecomp/pull/396), [commit 7b6cf34](https://github.com/sonicnext-dev/MarathonRecomp/commit/7b6cf348c2e39da70af61716d2428e472d47882c). **A performance improvement has not been established by a controlled before/after test.**

PR #369 was applied unchanged. The other fixes were adapted to this port's existing code: in particular, #370 uses the port's `std::thread` audio implementation and guards channel-configuration changes when no audio client is registered; #394 includes the corresponding regenerated PPC hook. The original authorship and license notices are preserved.

## Tested

- **POCO F5 (Adreno 725, Android 17):** the user tested 0.2.5-test1; its 4 min 52 s log contains no crash records or Vulkan/pipeline errors. A 180-degree rotation and return from the background were recorded, with rendering continuing afterward.
- Driver **A725 Performance / Turnip sysmem**, render scale **45%**, cap **60 FPS**, performance capture and sampling enabled: **59.4 / 59.8 / 59.6 / 59.9 FPS** in the four periodic summaries. Short drops and game-thread pauses around likely loading transitions remain; this is not a claim of uninterrupted 60 FPS or a speed-up over v0.2.4.
- The release APK has the **same native libraries and assets** as the tested 0.2.5-test1 APK. No new Mali or stock Qualcomm-driver validation was performed for this release.

## Known issues

The known issues from [v0.2.1](https://github.com/SansNope/MarathonRecomp-Android/blob/main/docs/android/RELEASE-v0.2.1.md#known-issues) and [v0.2.0](https://github.com/SansNope/MarathonRecomp-Android/blob/main/docs/android/RELEASE-v0.2.0.md#known-issues) remain, including loading-screen and video flicker reported on Helio G99. This release does not claim to fix those GPU-specific issues.

---

## Русский

**Пять исправлений из оригинального MarathonRecomp.** ARM64, Android 10 (API 29) и новее, Vulkan. Ставится поверх v0.2.4 или 0.2.5-test1 с сохранением данных. Версия **0.2.5 (код 11)**, тот же ключ подписи.

### Изменения и авторство оригинального проекта

Эти исправления изначально разработаны в **[sonicnext-dev/MarathonRecomp](https://github.com/sonicnext-dev/MarathonRecomp)** командой MarathonRecomp и участниками проекта. Авторы оригинальных исправлений — **[squidbus](https://github.com/squidbus)** и **[Hyper (hyperbx)](https://github.com/hyperbx)**; в этом выпуске их работа перенесена в Android-порт.

- **Базовые настройки профиля Xbox:** игра получает корректные данные профиля, включая значение инверсии оси Y по умолчанию, вместо чтения за границей буфера результата. Автор **squidbus** — [PR #369](https://github.com/sonicnext-dev/MarathonRecomp/pull/369), [коммит 038caf9](https://github.com/sonicnext-dev/MarathonRecomp/commit/038caf95d089581f80cbcbbfcfe159827423716f).
- **Освобождение аудиоклиента:** при его отключении игра останавливает аудиопоток и освобождает параметр обратного вызова. Автор **squidbus** — [PR #370](https://github.com/sonicnext-dev/MarathonRecomp/pull/370), [коммит d1155c9](https://github.com/sonicnext-dev/MarathonRecomp/commit/d1155c945c2953b66ac905d45c4352edffac1082).
- **Выключенная анизотропная фильтрация:** максимальная анизотропия сэмплера равна 1 вместо 16, когда фильтрация отключена. Видимых изменений картинки не ожидается. Автор **squidbus** — [PR #349](https://github.com/sonicnext-dev/MarathonRecomp/pull/349), [коммит 2bb8017](https://github.com/sonicnext-dev/MarathonRecomp/commit/2bb8017ac50029aea3568f7f02cd2cfc624896d2).
- **Смена потока, владеющего рендером:** другой поток ожидает начала следующего списка команд перед получением управления рендером. В оригинальном проекте это описано как возможное исправление случайных сбоев на первом кадре загрузочных экранов. Автор **Hyper**, **squidbus** указан соавтором — [PR #394](https://github.com/sonicnext-dev/MarathonRecomp/pull/394), [коммит 9b19b00](https://github.com/sonicnext-dev/MarathonRecomp/commit/9b19b0070fae6118a9da48d56a4d33ea94537467). Оригинальное исправление повторяет подход из [UnleashedRecomp](https://github.com/hedge-dev/UnleashedRecomp/pull/1486).
- **Убраны лишние вершины полноэкранных проходов:** в финальном проходе гаммы/поворота и при копировании цвета рисуются три вершины вместо шести, без лишнего треугольника. Автор **Hyper** — [PR #396](https://github.com/sonicnext-dev/MarathonRecomp/pull/396), [коммит 7b6cf34](https://github.com/sonicnext-dev/MarathonRecomp/commit/7b6cf348c2e39da70af61716d2428e472d47882c). **Прирост производительности одинаковым тестом до/после пока не подтверждён.**

PR #369 перенесён без изменений. Остальные исправления адаптированы к существующему коду порта: в частности, #370 использует нашу реализацию аудиопотока на `std::thread` и учитывает отсутствие аудиоклиента при смене конфигурации каналов; для #394 перегенерирован соответствующий PPC-хук. Авторство оригинального кода и лицензионные уведомления сохранены.

### Проверено

- **POCO F5 (Adreno 725, Android 17):** пользователь проверил 0.2.5-test1; в логе за 4 мин 52 с нет записей о сбоях или ошибках Vulkan/конвейеров. Зафиксированы поворот на 180° и возврат из фона с продолжением вывода.
- Драйвер **A725 Performance / Turnip sysmem**, масштаб **45%**, лимит **60 FPS**, включены запись производительности и профилирование: **59,4 / 59,8 / 59,6 / 59,9 FPS** в четырёх периодических сводках. Короткие просадки и паузы игрового потока на вероятных загрузочных переходах сохраняются; это не обещание непрерывных 60 FPS или ускорения относительно v0.2.4.
- В релизном APK **те же нативные библиотеки и ресурсы**, что в проверенном APK 0.2.5-test1. Новых проверок на Mali и системном драйвере Qualcomm для этого выпуска не проводилось.

### Известные проблемы

Сохраняются известные проблемы из [v0.2.1](https://github.com/SansNope/MarathonRecomp-Android/blob/main/docs/android/RELEASE-v0.2.1.md#known-issues) и [v0.2.0](https://github.com/SansNope/MarathonRecomp-Android/blob/main/docs/android/RELEASE-v0.2.0.md#known-issues), включая мерцание загрузочных экранов и роликов на Helio G99. Этот выпуск не заявляет об исправлении этих проблем отдельных GPU.
