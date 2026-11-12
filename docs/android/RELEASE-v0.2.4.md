# MarathonRecomp-Android v0.2.4

**A test release: one rendering change for tile-based GPUs and two new log lines.** ARM64, Android 10 (API 29) or newer, Vulkan. Installs over v0.2.3 and keeps your saves.

## Changes

- **The final pass no longer loads the screen image first.** The last pass of every frame (gamma and rotation) draws every pixel of the screen image, so its old contents are not read back any more. This matters on tile-based GPUs: Mali, and Adreno with the stock driver or Turnip in GMEM mode. How much time it saves is **not measured yet** — that is what this release is for. It follows the launcher switch *Optimized render passes (Vulkan)*: with the switch off the image is loaded as before.
- **`[fps]` line in every log.** Once a minute `log.txt` gets a line with the frame rate, the average and the worst frame time, the number of frames longer than 100 ms and 1 s, GPU time, render scale and the FPS cap. *Performance log* does not have to be on for it.
- **`GPU formats` line at startup.** It shows what the GPU can do with the texture and render target formats the game uses (sampling, linear filtering, render target, blending, depth-stencil). A missing letter is a lead for picture differences between GPUs, such as the black sky reported on a Mali-G57 phone.
- **Drawing to the back buffer with a smaller depth buffer.** If the game draws to the full-size back buffer while a smaller depth buffer is bound, the pipeline is now built without depth and stencil tests, matching the render pass. This case has not appeared in any tester's log; the fix removes undefined behaviour in case it does.

Version 0.2.4 (code 10), same signing key.

## For testers

- **Mali phones** (Helio G99, Unisoc T606, Tensor): play a level for a few minutes and send `log.txt` (Advanced → Open logs). The `[fps]` lines are enough; *Performance log* adds per-pass GPU times.
- **If the picture has garbage** at the edges or in the black bars, turn off *Optimized render passes (Vulkan)*, check again, and report both results with `log.txt`.

## Tested

- **POCO F5** (Adreno 725, Turnip in sysmem mode): the 0.2.4 test build works like v0.2.3, 58–60 FPS at the 60 FPS cap over six minutes of play. No speed-up is expected on this phone: in sysmem mode the driver does not read the image back anyway. The release APK has the same native libraries and assets as that test build.
- **Not tested on Mali** or on Adreno with the stock driver.

## Known issues

Unchanged from [v0.2.1](https://github.com/SansNope/MarathonRecomp-Android/blob/main/docs/android/RELEASE-v0.2.1.md#known-issues): loading screens and videos flicker on a Helio G99 phone, and everything listed for [v0.2.0](https://github.com/SansNope/MarathonRecomp-Android/blob/main/docs/android/RELEASE-v0.2.0.md#known-issues).

---

## Русский

**Тестовый релиз: одна правка рендера для тайловых GPU и две новые строки в логе.** ARM64, Android 10 (API 29) и новее, Vulkan. Ставится поверх v0.2.3 с сохранением данных.

### Изменения

- **Финальный проход больше не загружает изображение экрана.** Последний проход каждого кадра (гамма и поворот) рисует каждый пиксель изображения экрана, поэтому его старое содержимое больше не читается. Это важно для тайловых GPU: Mali, а также Adreno с системным драйвером или Turnip в режиме GMEM. Сколько времени это экономит, **пока не измерено** — для этого и нужен релиз. Правка подчиняется переключателю лаунчера «Оптимизированные проходы рендера (Vulkan)»: при выключенном переключателе изображение загружается, как раньше.
- **Строка `[fps]` в любом логе.** Раз в минуту в `log.txt` пишется строка с частотой кадров, средним и худшим временем кадра, числом кадров дольше 100 мс и 1 с, временем GPU, масштабом рендера и лимитом FPS. «Лог производительности» для неё включать не нужно.
- **Строка `GPU formats` при запуске.** Показывает, что GPU умеет с форматами текстур и целей рендера, которые использует игра (выборка, линейная фильтрация, цель рендера, смешивание, depth-stencil). Отсутствующая буква — зацепка для различий картинки между GPU, например для чёрного неба на телефоне с Mali-G57.
- **Рисование в задний буфер с меньшим буфером глубины.** Если игра рисует в полноразмерный задний буфер при привязанном буфере глубины меньшего размера, конвейер теперь строится без тестов глубины и трафарета и соответствует проходу. В логах тестеров этот случай не встречался; правка убирает неопределённое поведение на случай, если он встретится.

Версия 0.2.4 (код 10), тот же ключ подписи.

### Тестировщикам

- **Телефоны с Mali** (Helio G99, Unisoc T606, Tensor): поиграйте на уровне несколько минут и пришлите `log.txt` («Дополнительно» → «Открыть логи»). Достаточно строк `[fps]`; «Лог производительности» добавит время GPU по проходам.
- **Если на картинке мусор** по краям или в чёрных полосах, выключите «Оптимизированные проходы рендера (Vulkan)», проверьте ещё раз и сообщите оба результата вместе с `log.txt`.

### Проверено

- **POCO F5** (Adreno 725, Turnip в режиме sysmem): тестовая сборка 0.2.4 работает так же, как v0.2.3, 58–60 FPS при лимите 60 за шесть минут игры. Ускорения на этом телефоне не ожидается: в режиме sysmem драйвер и так не читает изображение. В релизном APK те же нативные библиотеки и ресурсы, что в этой тестовой сборке.
- **На Mali и на Adreno с системным драйвером не проверено.**

### Известные проблемы

Без изменений с [v0.2.1](https://github.com/SansNope/MarathonRecomp-Android/blob/main/docs/android/RELEASE-v0.2.1.md#known-issues): мерцание загрузочных экранов и роликов на телефоне с Helio G99, а также всё, что перечислено для [v0.2.0](https://github.com/SansNope/MarathonRecomp-Android/blob/main/docs/android/RELEASE-v0.2.0.md#known-issues).
