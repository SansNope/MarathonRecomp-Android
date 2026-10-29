# MarathonRecomp-Android v0.2.0

**Performance release.** ARM64, Android 10 (API 29) or newer, Vulkan. Game files are imported from your own Xbox 360 copy.

> [!IMPORTANT]
> We do not promise that the port works on every device. Everything below was measured on a few phones; other phones may behave differently. Please report problems in an [issue](https://github.com/SansNope/MarathonRecomp-Android/issues/new/choose) with your phone, GPU and `log.txt`.

## What to enable, and for whom

Press **Advanced → Recommended settings** once after updating. Switches that differ from the recommended settings are highlighted and listed under them, and `log.txt` lists them in a `[switches]` line.

| Who | Settings |
|---|---|
| **Everyone** | Recommended settings; driver **Auto**; FPS cap **60**. |
| **Snapdragon / Adreno** (e.g. POCO F5, Snapdragon 7+ Gen 2) | Render scale **60 %** gives 60 FPS on the POCO F5. Faster chips (Snapdragon 8 Elite) ran at 100–125 % in earlier versions. |
| **Google Tensor G2 / Mali-G710** (Pixel 7 Pro) | Render scale **25–50 %**. Expect 41–57 FPS in levels; 60 FPS is not reached yet. Going below 50 % helps little (see below). |
| **Mali-G57** (Helio G99 and similar) | Render scale **50 %** or lower. Not re-tested on this version. |
| **Graphics artifacts** (flicker, garbage, missing effects) | Turn off **Optimized render passes (Vulkan)** and report it with `log.txt`. |
| **Testers** | **Performance monitor** and **Performance log** on; attach `log.txt` (Advanced → Open logs). **CPU profiler** only when asked. |

**Experiments — leave them off unless you are testing:** *Triple buffering* changes nothing on the tested phones (their drivers already give 4–5 swap-chain images); *Precise frame limiter* and *Main game thread on the strongest cores only* were not measured on their own.

## Changes since v0.1.6

### Performance

- **Game threads on the fast cores.** On the Pixel 7 Pro the main game thread and the render thread ran on the little Cortex-A55 cores most of the time. They are now kept on the big cores together with the game's worker threads; the list of workers was extended (ModelStandard, particle, Havok physics and document workers) after measuring which threads the main thread waits for. Pinning is re-applied after returning from the background and checked every 2 seconds. It is only used on CPUs with little cores (switch: *Keep game threads on fast cores*).
- **Fewer FPU rounding-mode switches.** The game's vector dot products switch the rounding mode before and after every instruction, which is slow on ARM; one function took ~18 % of the game thread's time on the Pixel 7 Pro. A new XenonRecomp option keeps one switch for a whole run of such instructions; the generated code was checked instruction by instruction. Together with the thread pinning, the game thread's CPU time per frame on the Pixel 7 Pro dropped by about half.
- **Tile-friendly Vulkan render passes** (switch: *Optimized render passes (Vulkan)*, on): clears are done when a render pass starts instead of inside it, and barriers wait only for the stages that really use the texture. On the Mali-G710 the GPU time in levels went down by roughly 10–15 % (scenes were not identical). No artifacts on Mali-G710 and Adreno 725.
- **Shader pipeline cache on disk** (switch: *Shader cache*, on): Vulkan pipelines are saved in the game folder (`cache/pipeline_cache.bin`) every 30 seconds and when the game goes to the background, and are reused on later launches. A cache that crashed the driver while loading is deleted automatically.
- **Texture cache for GPUs without BC textures** (Mali and others; switch: *Cache transcoded textures*, on): textures converted to ETC2 are stored in the app's internal storage (up to 1 GB) and read back on later loads — about 1.5 s less CPU work per level load on the Pixel 7 Pro. Phones with BC support (Adreno) never use it.
- **Android performance hints (ADPF)** for the game and render threads where the system supports them (switch, on). In our Pixel 7 Pro logs the system did not raise the CPU limits through them; the POCO F5 does not support them.
- MSAA pipeline variants are no longer compiled on Android (MSAA is off there).

### Fixes

- **The game kept running in the background.** SDL events were never processed on Android, so the game did not pause, sound kept playing and **achievement notifications were never shown**. Now the game and the sound pause in the background, and achievement pop-ups appear.
- **The camera sometimes stopped responding** to the touch stick and the gamepad (after springs, boosters and similar). Upstream fix [sonicnext-dev/MarathonRecomp#317](https://github.com/sonicnext-dev/MarathonRecomp/pull/317) is backported, and the extra camera fields are now initialised.
- **Frame-rate caps below 60 are no longer offered on Android.** At 30 FPS the game itself slowed down (0.3× speed on a Helio G99) and scripted scenes ran slow at 35–50; upstream's frame-rate fixes only cover caps above 60. A lower cap saved by an older version is raised to 60.
- Kernel wait functions with a finite timeout (events, semaphores, waiting for several objects or a thread) now wait until the timeout and return the correct status.
- A swap chain with no upper image limit (`maxImageCount = 0`) is handled correctly.

### Launcher and diagnostics

- **Advanced → Debug** section with the switches above, plus **Recommended settings** and a list of the switches that differ from them.
- **Performance monitor** overlay: FPS, frame time and the worst frame, GPU time and load (when readable), CPU clocks per cluster, game/render thread load, temperature and memory, a frame-time graph.
- **Performance log is opt-in now.** v0.1.6 created `perf_capture.enabled` automatically; it is removed on the first start of this version. Turn it on in the launcher when you want to send a log.
- `log.txt` lines for analysis: `[perf]` (frame, game thread, GPU, display wait, clocks), `[prof]` / `[threads]` / `[cpufreq]` (CPU profiler), `[waits]` / `[wakers]` (what the game thread waits for and who wakes it), `[gpu-passes]` / `[gpu-frame]` (GPU time per render pass), `[texture-cache]`, `[switches]`.
- Version 0.2.0 (code 6), same signing key as v0.1.0–v0.1.6: installs over them and keeps your saves.

## What we found about Mali performance (Pixel 7 Pro)

- In levels the frame takes 18–24 ms: ~10–12 ms of game-thread work plus ~5–6 ms waiting for the next swap-chain image. The GPU needs 12–17 ms.
- About 10 ms of the GPU time does not depend on the render scale: 512×512 shadow maps (8 passes), a chain of ~25 tiny bloom/brightness passes, and four passes at the full screen resolution (HUD and final image, then gamma with screen rotation). That is why a render scale below 50 % gains little.
- Further speed-ups would need bigger changes (merging the tiny passes, a lighter shadow setup, merging gamma into the last game pass). They are not planned for now.

## Tested

| Device | Result |
|---|---|
| **POCO F5** — Snapdragon 7+ Gen 2, Adreno 725, Android 17 | Development builds of this version: 60 FPS in the HUB and in levels at 60 % render scale, GPU ~13 ms per frame, no artifacts. The final APK: launcher and game start checked. |
| **Pixel 7 Pro** — Google Tensor G2, Mali-G710, Android 17 (tester) | Development builds of this version: **45–55 FPS** in levels at 25 % render scale (v0.1.6: 24–31 FPS), **41–57 FPS** at 50 %, 60 FPS in menus; no artifacts, no crashes. |

Not re-tested on this version: realme RMX5090 (Snapdragon 8 Elite), Helio G99 / Mali-G57 MC2, Unisoc T606 / Mali-G57 MP1. Full list: [STATUS](https://github.com/SansNope/MarathonRecomp-Android/blob/main/docs/android/STATUS.md).

## Known issues

- **Pixel 7 Pro / Mali-G710:** 60 FPS is not reached in levels (see above).
- **Loading hitches** — scene transitions drop the frame rate for a few seconds; first loads on Mali are slower (texture conversion; later loads use the texture cache).
- **Adreno 610 (Snapdragon 680)** — broken 2D graphics on the title screen ([#2](https://github.com/SansNope/MarathonRecomp-Android/issues/2)).
- Bifrost Mali GPUs (G31, G51, G52, G71, G72, G76) are not supported.

Thanks to the Pixel 7 Pro and Mali testers for the many logs.

---

## Русский

**Релиз про производительность.** ARM64, Android 10 (API 29) и новее, Vulkan. Файлы игры импортируются из своей копии для Xbox 360.

> [!IMPORTANT]
> Мы не обещаем работу на всех устройствах. Всё ниже измерено на нескольких телефонах; на других результат может отличаться. О проблемах пишите в [issue](https://github.com/SansNope/MarathonRecomp-Android/issues/new/choose) с моделью телефона, GPU и `log.txt`.

### Что и кому включать

После обновления один раз нажмите **«Дополнительно» → «Рекомендуемые настройки»**. Переключатели, которые отличаются от рекомендуемых, подсвечиваются и перечисляются под списком, а в `log.txt` они видны в строке `[switches]`.

| Кому | Настройки |
|---|---|
| **Всем** | Рекомендуемые настройки; драйвер **«Авто»**; лимит FPS **60**. |
| **Snapdragon / Adreno** (например, POCO F5, Snapdragon 7+ Gen 2) | Масштаб рендера **60 %** даёт 60 FPS на POCO F5. Более быстрые чипы (Snapdragon 8 Elite) в прошлых версиях работали на 100–125 %. |
| **Google Tensor G2 / Mali-G710** (Pixel 7 Pro) | Масштаб рендера **25–50 %**. На уровнях 41–57 FPS; до 60 пока не дотягивает. Ниже 50 % выигрыш небольшой (см. ниже). |
| **Mali-G57** (Helio G99 и похожие) | Масштаб рендера **50 %** или меньше. На этой версии не перепроверялось. |
| **Артефакты графики** (мерцание, мусор, пропавшие эффекты) | Выключите **«Оптимизированные проходы рендера (Vulkan)»** и сообщите об этом с `log.txt`. |
| **Тестировщикам** | **«Монитор производительности»** и **«Лог производительности»** вкл.; приложите `log.txt` («Дополнительно» → «Открыть логи»). **«Профайлер CPU»** — только по просьбе. |

**Эксперименты — оставьте выключенными, если не тестируете:** «Тройная буферизация» на проверенных телефонах ничего не меняет (драйверы и так дают 4–5 изображений в цепочке показа); «Точный лимитер кадров» и «Главный игровой поток только на самых сильных ядрах» отдельно не измерялись.

### Изменения с v0.1.6

**Производительность**

- **Игровые потоки на быстрых ядрах.** На Pixel 7 Pro главный игровой поток и поток рендера большую часть времени работали на малых ядрах Cortex-A55. Теперь они закрепляются на больших ядрах вместе с рабочими потоками игры; список рабочих потоков расширен (ModelStandard, частицы, физика Havok, рабочие потоки документа) по замерам того, кого ждёт главный поток. Закрепление восстанавливается после возврата из фона и проверяется раз в 2 секунды. Используется только на процессорах с малыми ядрами (переключатель «Игровые потоки на быстрых ядрах»).
- **Меньше переключений режима округления FPU.** Векторные скалярные произведения игры переключают режим округления до и после каждой инструкции, а на ARM это медленно; одна функция занимала ~18 % времени игрового потока на Pixel 7 Pro. Новая опция XenonRecomp оставляет одно переключение на всю серию таких инструкций; сгенерированный код проверен по инструкциям. Вместе с закреплением потоков время CPU игрового потока на кадр на Pixel 7 Pro сократилось примерно вдвое.
- **Проходы рендера Vulkan под тайловые GPU** (переключатель «Оптимизированные проходы рендера (Vulkan)», вкл.): очистка выполняется при открытии прохода, а не внутри него, а барьеры ждут только те стадии, которые действительно используют текстуру. На Mali-G710 время GPU на уровнях снизилось примерно на 10–15 % (сцены не совпадали). Артефактов на Mali-G710 и Adreno 725 нет.
- **Кэш шейдерных конвейеров на диске** (переключатель «Кэш шейдеров», вкл.): конвейеры Vulkan сохраняются в папку игры (`cache/pipeline_cache.bin`) раз в 30 секунд и при уходе в фон и используются при следующих запусках. Кэш, при загрузке которого упал драйвер, удаляется автоматически.
- **Кэш текстур для GPU без BC** (Mali и другие; переключатель «Кэш перекодированных текстур», вкл.): перекодированные в ETC2 текстуры сохраняются во внутреннюю память приложения (до 1 ГБ) и читаются при следующих загрузках — около 1,5 с работы CPU меньше на каждую загрузку уровня на Pixel 7 Pro. Телефоны с поддержкой BC (Adreno) его не используют.
- **Подсказки производительности Android (ADPF)** для игрового потока и потока рендера, где система их поддерживает (переключатель, вкл.). В наших логах Pixel 7 Pro система через них не поднимала ограничения частоты CPU; POCO F5 их не поддерживает.
- На Android больше не компилируются варианты конвейеров для MSAA (MSAA там выключен).

**Исправления**

- **Игра продолжала работать в фоне.** На Android события SDL не обрабатывались, поэтому игра не вставала на паузу, звук играл, а **уведомления о достижениях не показывались никогда**. Теперь игра и звук в фоне на паузе, уведомления о достижениях появляются.
- **Камера иногда переставала слушаться** сенсорного стика и геймпада (после пружин, ускорителей и т. п.). Перенесено исправление из оригинала [sonicnext-dev/MarathonRecomp#317](https://github.com/sonicnext-dev/MarathonRecomp/pull/317), дополнительные поля камеры теперь инициализируются.
- **Лимит FPS ниже 60 на Android больше не предлагается.** При 30 FPS замедлялась сама игра (скорость 0,3× на Helio G99), а скриптовые сцены тормозили при 35–50; исправления привязки к кадру в оригинале есть только для лимитов выше 60. Меньший лимит, сохранённый старой версией, поднимается до 60.
- Функции ожидания ядра с конечным таймаутом (события, семафоры, ожидание нескольких объектов или потока) теперь ждут до конца таймаута и возвращают правильный результат.
- Правильно обрабатывается цепочка показа без верхнего предела числа изображений (`maxImageCount = 0`).

**Лаунчер и диагностика**

- Раздел **«Дополнительно» → «Отладка»** с переключателями выше, кнопкой **«Рекомендуемые настройки»** и списком переключателей, которые от них отличаются.
- Оверлей **«Монитор производительности»**: FPS, время кадра и худший кадр, время и загрузка GPU (если читается), частоты CPU по кластерам, загрузка игрового потока и потока рендера, температура и память, график времени кадра.
- **Лог производительности теперь включается вручную.** v0.1.6 сама создавала `perf_capture.enabled`; при первом запуске этой версии он удаляется. Включите лог в лаунчере, когда хотите прислать его.
- Строки `log.txt` для разбора: `[perf]` (кадр, игровой поток, GPU, ожидание дисплея, частоты), `[prof]` / `[threads]` / `[cpufreq]` (профайлер CPU), `[waits]` / `[wakers]` (чего ждёт игровой поток и кто его будит), `[gpu-passes]` / `[gpu-frame]` (время GPU по проходам рендера), `[texture-cache]`, `[switches]`.
- Версия 0.2.0 (код 6), тот же ключ подписи, что у v0.1.0–v0.1.6: ставится поверх с сохранением данных.

### Что выяснили о производительности Mali (Pixel 7 Pro)

- На уровнях кадр занимает 18–24 мс: ~10–12 мс работы игрового потока и ~5–6 мс ожидания следующего изображения цепочки показа. GPU нужно 12–17 мс.
- Около 10 мс времени GPU не зависят от масштаба рендера: карты теней 512×512 (8 проходов), цепочка из ~25 крошечных проходов bloom/яркости и четыре прохода в полном разрешении экрана (HUD и итоговая картинка, затем гамма с поворотом экрана). Поэтому масштаб ниже 50 % даёт мало.
- Дальнейшее ускорение требует больших изменений (объединение крошечных проходов, облегчённые тени, гамма в последнем проходе игры). Пока не планируется.

### Проверено

| Устройство | Результат |
|---|---|
| **POCO F5** — Snapdragon 7+ Gen 2, Adreno 725, Android 17 | Тестовые сборки этой версии: 60 FPS в HUB и на уровнях при масштабе 60 %, GPU ~13 мс на кадр, артефактов нет. Итоговый APK: проверены лаунчер и запуск игры. |
| **Pixel 7 Pro** — Google Tensor G2, Mali-G710, Android 17 (тестер) | Тестовые сборки этой версии: **45–55 FPS** на уровнях при масштабе 25 % (v0.1.6: 24–31 FPS), **41–57 FPS** при 50 %, 60 FPS в меню; артефактов и падений нет. |

На этой версии не перепроверялись: realme RMX5090 (Snapdragon 8 Elite), Helio G99 / Mali-G57 MC2, Unisoc T606 / Mali-G57 MP1. Полный список: [STATUS](https://github.com/SansNope/MarathonRecomp-Android/blob/main/docs/android/STATUS.ru.md).

### Известные проблемы

- **Pixel 7 Pro / Mali-G710:** на уровнях 60 FPS не достигается (см. выше).
- **Просадки при загрузках** — переходы между сценами на несколько секунд снижают FPS; первые загрузки на Mali медленнее (перекодирование текстур, дальше — из кэша).
- **Adreno 610 (Snapdragon 680)** — сломанная 2D-графика на титульном экране ([#2](https://github.com/SansNope/MarathonRecomp-Android/issues/2)).
- Mali поколения Bifrost (G31, G51, G52, G71, G72, G76) не поддерживаются.

Спасибо тестировщикам Pixel 7 Pro и Mali за множество логов.
