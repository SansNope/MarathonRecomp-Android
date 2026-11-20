# MarathonRecomp-Android v0.3.1

**World shadow controls, bloom toggle, and 12 additional options in Options → Game Settings (37 total).** Android 10 (API 29) or newer, ARM64 and Vulkan. Version **0.3.1 (code 17)** uses the existing Android package and release signing key.

## Launcher graphics settings

Four optional controls are available under **Graphics** in the Android launcher. Select **Launch game** to save them and fully restart the game to apply changes.

- **World shadow quality** (`Auto`, `256 × 256` to `8192 × 8192`) overrides the cascaded world shadow map resolution (`CSMTextureSize`). **Auto** keeps the in-game setting, or `512 × 512` when Low graphics is enabled.
- **Limit full object shadows to 2 cascades** (off by default) caps `GenerateCSMObjects` at two cascades instead of drawing full objects into every cascade.
- **Shorter shadow distance (experimental)** (off by default) scales each shadow cascade's camera extent to 65% of stock distance.
- **Disable bloom** (off by default) skips `MakeBloom` / `ApplyBloom` in the stock gameplay and preview render scripts while keeping the rest of post-processing and the HUD intact.

## 12 additional Game Settings

Open **Options → Game Settings** from the main menu or pause menu. All 12 new switches default to off, include names and descriptions in all six in-game menu languages, and are saved under `[Codes]` when leaving Options.

- **Character and snowboard movement:** **Steer During Bounce Attack** (Sonic), **Steer During Spin Kick**, **Steer During Hammer Attack** (Amy), **Disable Airborne Spinning Claw** (Blaze), **Full Jump Height in Sand and Water**, and **Full Snowboard Steering**.
- **Gameplay and UI:** **Keep Lives on Death**, **Show Mission Text on Pause**, **Faster Title Screen Input**, **Restore Chain Jump Flips**, **Episode HUD Colours** (re-enter the stage to apply or revert), and **Disable Kingdom Valley Mist** (updated with the upstream early-return fix).

## Credits and original patch authors

- **Shadow hook sites adapted from the original MarathonRecomp:** **Hyper** ([PR #400](https://github.com/sonicnext-dev/MarathonRecomp/pull/400) · [commit `612ffc5`](https://github.com/sonicnext-dev/MarathonRecomp/commit/612ffc50780c8ee2610502e5161c408545c2ceba)).
- **Four movement patches and reversible HUD colours fix backported from the original MarathonRecomp:** **Branch of Hourai {&}** and **Hyper** ([PR #217](https://github.com/sonicnext-dev/MarathonRecomp/pull/217) · [commit `03c94ac`](https://github.com/sonicnext-dev/MarathonRecomp/commit/03c94ac62f139776cb12a2b5f3d85843fb85cb56)). Original Xbox 360 patches for *Steer During Hammer Attack*, *Full Jump Height in Sand and Water*, *Disable Airborne Spinning Claw* and *Full Snowboard Steering* by **Hyper**.
- **Disable Kingdom Valley Mist fix backported from the original MarathonRecomp:** **squidbus** ([PR #366](https://github.com/sonicnext-dev/MarathonRecomp/pull/366) · [commit `55b3e69`](https://github.com/sonicnext-dev/MarathonRecomp/commit/55b3e6967aebdfba57804c5416e5b7d4e09a3240)).
- **Eight existing patches exposed in Game Settings:**
  - *Steer During Bounce Attack* — original Xbox 360 patch by **Desko**, MarathonRecomp by **Isaac Marovitz** ([PR #142](https://github.com/sonicnext-dev/MarathonRecomp/pull/142)).
  - *Steer During Spin Kick* — original Xbox 360 patch by **Nonami**, MarathonRecomp by **Isaac Marovitz** ([PR #141](https://github.com/sonicnext-dev/MarathonRecomp/pull/141)).
  - *Disable Kingdom Valley Mist* — original Xbox 360 patch by **Hyper**, MarathonRecomp by **Isaac Marovitz** ([PR #139](https://github.com/sonicnext-dev/MarathonRecomp/pull/139)), fixed by **squidbus** ([PR #366](https://github.com/sonicnext-dev/MarathonRecomp/pull/366)).
  - *Faster Title Screen Input* — original Xbox 360 patch by **Hyper** and **VolcanoTheBat**, MarathonRecomp by **Hyper** ([PR #144](https://github.com/sonicnext-dev/MarathonRecomp/pull/144)).
  - *Keep Lives on Death* — **Isaac Marovitz** ([PR #176](https://github.com/sonicnext-dev/MarathonRecomp/pull/176)).
  - *Restore Chain Jump Flips* — original Xbox 360 patch by **Desko** and **Chara**, MarathonRecomp by **Isaac Marovitz** ([PR #148](https://github.com/sonicnext-dev/MarathonRecomp/pull/148)).
  - *Episode HUD Colours* — original Xbox 360 patch by **Dunker**, **Reimous**, **Hyper** and **Chara**, MarathonRecomp by **Hyper** ([PR #145](https://github.com/sonicnext-dev/MarathonRecomp/pull/145)), reversible toggle adapted from [PR #217](https://github.com/sonicnext-dev/MarathonRecomp/pull/217).
  - *Show Mission Text on Pause* — original Xbox 360 patch by **Desko** and **Hyper**, MarathonRecomp by **Isaac Marovitz** ([PR #137](https://github.com/sonicnext-dev/MarathonRecomp/pull/137)).

Original Xbox 360 patch reference archive: [Sonic '06 Mod Manager Patches](https://github.com/Knuxfan24/Sonic-06-Mod-Manager-Patches/tree/52ac81009e2580d6e93a52d87c7d1e4296977760). Based on [MarathonRecomp by SonicNext Dev and contributors](https://github.com/sonicnext-dev/MarathonRecomp). [Full source diff](https://github.com/SansNope/MarathonRecomp-Android/compare/v0.3.0...v0.3.1)

## Tested

**0.3.1-test2** was tested in gameplay on **POCO F5 (Adreno 725)** with `256 × 256` world shadows, 2 object shadow cascades, shorter shadow distance, disabled bloom, Low graphics and 75% output (`1800×810` output, `1080×486` scene at 60% scale). The 10-minute session log confirmed `CSMTextureSize = 256`, `GenerateCSMObjects = 2`, disabled bloom and real-time reflections with no crashes or Vulkan errors, and the tester confirmed the new options work. This release packages the exact same tested native library (`libmain.so`). Performance impact on Mali GPUs still needs A/B testing on devices.

---

## Русский

**Настройки теней мира, отключение свечения (bloom) и 12 дополнительных пунктов в Options → Game Settings (всего 37).** Требуются Android 10 (API 29) или новее, ARM64 и Vulkan. Версия **0.3.1 (код 17)** использует прежний идентификатор Android-приложения и релизный ключ подписи.

### Настройки графики в лаунчере

В разделе **«Графика»** Android-лаунчера появились четыре дополнительные настройки. Нажмите **«Запустить игру»** для сохранения и полностью перезапустите игру, чтобы изменения вступили в силу.

- **Качество теней мира** (`Авто`, от `256 × 256` до `8192 × 8192`) — задаёт разрешение каскадных карт теней мира (`CSMTextureSize`). Режим **«Авто»** использует игровую настройку или `512 × 512` при включённой облегчённой графике.
- **Полные тени от объектов только в двух каскадах** (выключено по умолчанию) — ограничивает `GenerateCSMObjects` двумя каскадами вместо отрисовки полного набора объектов во всех каскадах.
- **Сократить дальность теней (экспериментально)** (выключено по умолчанию) — уменьшает размер камеры каждого каскада теней до 65% от штатной дальности.
- **Отключить свечение (bloom)** (выключено по умолчанию) — пропускает вызовы `MakeBloom` / `ApplyBloom` в штатных render-скриптах игры и предпросмотра, сохраняя остальную постобработку и интерфейс.

### 12 дополнительных опций в Game Settings

Откройте **Options → Game Settings** из главного меню или паузы. Все 12 новых переключателей выключены по умолчанию, имеют названия и описания на всех шести языках игрового меню и сохраняются в секции `[Codes]` при выходе из Options.

- **Управление персонажами и сноубордом:** управление во время Bounce Attack (Соник), управление во время Spin Kick, управление молотом Эми, отключение воздушного Spinning Claw у Блейз, полная высота прыжка в песке и воде, полное руление сноубордом.
- **Геймплей и интерфейс:** сохранение жизней при обычной гибели, показ цели миссии в паузе, более ранний ввод на титульном экране, сальто в цепных прыжках, цвета HUD по эпизоду (для применения или возврата нужно заново войти в уровень) и отключение тумана Kingdom Valley (с перенесённым исправлением из апстрима).

### Авторы и происхождение патчей

- **Точки хуков теней адаптированы из оригинального MarathonRecomp:** **Hyper** ([PR #400](https://github.com/sonicnext-dev/MarathonRecomp/pull/400) · [коммит `612ffc5`](https://github.com/sonicnext-dev/MarathonRecomp/commit/612ffc50780c8ee2610502e5161c408545c2ceba)).
- **Четыре патча управления и обратимое переключение цветов HUD перенесены из оригинального MarathonRecomp:** **Branch of Hourai {&}** и **Hyper** ([PR #217](https://github.com/sonicnext-dev/MarathonRecomp/pull/217) · [коммит `03c94ac`](https://github.com/sonicnext-dev/MarathonRecomp/commit/03c94ac62f139776cb12a2b5f3d85843fb85cb56)). Автор оригинальных патчей для Xbox 360 (*управление молотом Эми*, *прыжок в песке и воде*, *отключение воздушного Spinning Claw*, *полное руление сноубордом*) — **Hyper**.
- **Исправление отключения тумана Kingdom Valley перенесено из оригинального MarathonRecomp:** **squidbus** ([PR #366](https://github.com/sonicnext-dev/MarathonRecomp/pull/366) · [коммит `55b3e69`](https://github.com/sonicnext-dev/MarathonRecomp/commit/55b3e6967aebdfba57804c5416e5b7d4e09a3240)).
- **Восемь уже присутствовавших патчей выведены в Game Settings:**
  - *Управление во время Bounce Attack* — оригинальный патч Xbox 360: **Desko**, реализация в MarathonRecomp: **Isaac Marovitz** ([PR #142](https://github.com/sonicnext-dev/MarathonRecomp/pull/142)).
  - *Управление во время Spin Kick* — оригинальный патч Xbox 360: **Nonami**, реализация в MarathonRecomp: **Isaac Marovitz** ([PR #141](https://github.com/sonicnext-dev/MarathonRecomp/pull/141)).
  - *Отключение тумана Kingdom Valley* — оригинальный патч Xbox 360: **Hyper**, реализация в MarathonRecomp: **Isaac Marovitz** ([PR #139](https://github.com/sonicnext-dev/MarathonRecomp/pull/139)), исправление: **squidbus** ([PR #366](https://github.com/sonicnext-dev/MarathonRecomp/pull/366)).
  - *Более ранний ввод на титульном экране* — оригинальный патч Xbox 360: **Hyper** и **VolcanoTheBat**, реализация в MarathonRecomp: **Hyper** ([PR #144](https://github.com/sonicnext-dev/MarathonRecomp/pull/144)).
  - *Сохранение жизней при обычной гибели* — **Isaac Marovitz** ([PR #176](https://github.com/sonicnext-dev/MarathonRecomp/pull/176)).
  - *Сальто в цепных прыжках* — оригинальный патч Xbox 360: **Desko** и **Chara**, реализация в MarathonRecomp: **Isaac Marovitz** ([PR #148](https://github.com/sonicnext-dev/MarathonRecomp/pull/148)).
  - *Цвета HUD по эпизоду* — оригинальный патч Xbox 360: **Dunker**, **Reimous**, **Hyper** и **Chara**, реализация в MarathonRecomp: **Hyper** ([PR #145](https://github.com/sonicnext-dev/MarathonRecomp/pull/145)), обратимое переключение адаптировано из [PR #217](https://github.com/sonicnext-dev/MarathonRecomp/pull/217).
  - *Цель миссии в паузе* — оригинальный патч Xbox 360: **Desko** и **Hyper**, реализация в MarathonRecomp: **Isaac Marovitz** ([PR #137](https://github.com/sonicnext-dev/MarathonRecomp/pull/137)).

Архив оригинальных патчей Xbox 360: [Sonic '06 Mod Manager Patches](https://github.com/Knuxfan24/Sonic-06-Mod-Manager-Patches/tree/52ac81009e2580d6e93a52d87c7d1e4296977760). В основе порта [MarathonRecomp команды SonicNext Dev и участников проекта](https://github.com/sonicnext-dev/MarathonRecomp). [Полный список изменений в исходниках](https://github.com/SansNope/MarathonRecomp-Android/compare/v0.3.0...v0.3.1)

### Проверено

Сборка **0.3.1-test2** проверена в игре на **POCO F5 (Adreno 725)** с тенями мира `256 × 256`, лимитом в 2 каскада для объектов, сокращённой дальностью теней, отключённым bloom, облегчённой графикой и выводом 75% (`1800×810`, сцена `1080×486` при масштабе 60%). Десятиминутный лог подтвердил `CSMTextureSize = 256`, `GenerateCSMObjects = 2`, отключение bloom и динамических отражений без крашей и ошибок Vulkan; пользователь подтвердил работу новых опций. В релиз вошла побайтно та же нативная библиотека (`libmain.so`). Влияние новых настроек графики на Mali ещё требует A/B-замеров на устройствах.
