# MarathonRecomp-Android v0.2.1

**Small fix release on top of [v0.2.0](RELEASE-v0.2.0.md).** ARM64, Android 10 (API 29) or newer, Vulkan. Installs over v0.2.0 and keeps your saves.

## Changes

- **Render thread crash while a level loads fixed.** On a Helio G99 (Mali-G57) the game once drew a shape while no vertex declaration was set, and building the graphics pipeline for that state crashed the render thread. Such draws are now skipped and reported once in `log.txt` (`DrawPrimitiveUP skipped: no vertex declaration is set …`). The crash happened only once in our tests, so the fix has not been seen in action yet; if you see that line, please send us the log.
- **`log.txt` now starts with the app version and the library build ID** (`[app] version=0.2.1 code=7 build_id=…`), so a log can be matched with the exact build.
- Version 0.2.1 (code 7), same signing key.

## Recommendations (in addition to the [v0.2.0 table](RELEASE-v0.2.0.md#what-to-enable-and-for-whom))

- **Mali-G57 (Helio G99 and similar): set *Shadow Resolution* to 512** in the game's graphics options. On a TECNO KJ7 (Helio G99) the 1024 shadow maps took ~7 ms of a ~26 ms frame; with 512 the median frame rate in levels went from 42 to 54 FPS (scenes differed). Keep the render scale at 25–50 % and the FPS cap at 60.

## Tested

- **POCO F5** (Adreno 725): the 0.2.1 test build — game starts and plays, 60 FPS in levels, the `[app]` line is correct.
- **TECNO KJ7** (Helio G99, Mali-G57 MC2, tester): the 0.2.1 test build — played without a crash (the crash did not happen again, so the skipped-draw line did not appear either).

## Known issues

- **Helio G99: loading screens and videos flicker** since v0.2.0 (reported by a tester, also with a 60 FPS cap). Not found yet; we are looking into it.
- **Graphics artifacts were reported on an Adreno 830 phone** with v0.2.0; details are not known yet.
- Everything listed for [v0.2.0](RELEASE-v0.2.0.md#known-issues).

---

## Русский

**Небольшой исправляющий релиз поверх [v0.2.0](RELEASE-v0.2.0.md).** ARM64, Android 10 (API 29) и новее, Vulkan. Ставится поверх v0.2.0 с сохранением данных.

### Изменения

- **Исправлено падение потока рендера при загрузке уровня.** На Helio G99 (Mali-G57) игра однажды попыталась нарисовать фигуру без описания вершин, и создание графического конвейера для такого состояния роняло поток рендера. Теперь такие отрисовки пропускаются, а в `log.txt` один раз пишется строка `DrawPrimitiveUP skipped: no vertex declaration is set …`. В наших тестах падение случилось только один раз, поэтому срабатывание исправления ещё не видели; если заметите эту строку в логе — пришлите его нам.
- **`log.txt` теперь начинается с версии приложения и Build ID библиотеки** (`[app] version=0.2.1 code=7 build_id=…`) — по ним лог однозначно сопоставляется со сборкой.
- Версия 0.2.1 (код 7), тот же ключ подписи.

### Рекомендации (дополнение к [таблице v0.2.0](RELEASE-v0.2.0.md#что-и-кому-включать))

- **Mali-G57 (Helio G99 и похожие): поставьте *Shadow Resolution* (разрешение теней) 512** в настройках графики игры. На TECNO KJ7 (Helio G99) тени 1024 занимали ~7 мс из ~26 мс кадра; с 512 медиана FPS на уровнях выросла с 42 до 54 (сцены отличались). Масштаб рендера — 25–50 %, лимит FPS — 60.

### Проверено

- **POCO F5** (Adreno 725): тестовая сборка 0.2.1 — игра запускается и играется, 60 FPS на уровнях, строка `[app]` правильная.
- **TECNO KJ7** (Helio G99, Mali-G57 MC2, тестировщик): тестовая сборка 0.2.1 — игра без падений (падение не повторилось, поэтому и строки о пропуске отрисовки в логе нет).

### Известные проблемы

- **Helio G99: мерцают загрузочные экраны и ролики** с v0.2.0 (сообщил тестировщик, в том числе при лимите 60 FPS). Причина пока не найдена, разбираемся.
- **Сообщают об артефактах графики на телефоне с Adreno 830** на v0.2.0; подробностей пока нет.
- Всё, что перечислено для [v0.2.0](RELEASE-v0.2.0.md#known-issues).
