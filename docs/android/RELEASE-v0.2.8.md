# MarathonRecomp-Android v0.2.8

**New in-game Game Settings menu with two gameplay patches.** Android 10 (API 29) or newer, ARM64 and Vulkan. Version **0.2.8 (code 14)** uses the regular Android package and signing key, so it can update v0.2.7 in place.

## What we changed in the Android port

- Added **Options → Game Settings** to the main menu and pause menu. Both patches have independent switches, localised in the six languages supported by the in-game options menu.
- Enabled both options by default. Existing settings are read from the same keys in `config.toml`. Changes take effect without restarting and are saved when you leave Options.
- Exposed the existing **Max Speed Air Control** patch and backported **Wall Jump Fix** from the original MarathonRecomp.

## Gameplay patches and credits

- **Max Speed Air Control.** Steer Sonic with the left stick after takeoff in Max Speed sections. Original patch by **Desko and Dunker**, implemented in MarathonRecomp by **Isaac Marovitz**. [Original patch](https://github.com/Knuxfan24/Sonic-06-Mod-Manager-Patches/blob/master/MidairControlForMachSpeed.mlua) · [MarathonRecomp commit](https://github.com/sonicnext-dev/MarathonRecomp/commit/359a9f20edd76cd58f7fbd10c52b3bd82728ab25)
- **Wall Jump Fix.** Prevents Knuckles and Rouge from automatically grabbing a wall during a normal jump, including immediately after jumping away from it. They can still grab walls while gliding. Original patch by **Nonami**, implemented in MarathonRecomp by **Hyper**. [Original patch](https://github.com/Knuxfan24/Sonic-06-Mod-Manager-Patches/blob/master/DisableClimbFromJump.mlua) · [MarathonRecomp commit](https://github.com/sonicnext-dev/MarathonRecomp/commit/bd87f24669947bbabf00b1114b8739db5ea279f2) · [PR #217](https://github.com/sonicnext-dev/MarathonRecomp/pull/217)

## Tested

Both patches were confirmed working on **POCO F5** in **0.2.7-modded-test1**. This release uses the same native library and packages it as an update for the regular Android app.

---

## Русский

**Новый раздел Game Settings в игре и два игровых патча.** Android 10 (API 29) и новее, ARM64 и Vulkan. Версия **0.2.8 (код 14)** использует идентификатор и ключ подписи обычного приложения и устанавливается поверх v0.2.7.

### Что мы сделали в Android-порте

- Добавили **Options → Game Settings** в главное меню и меню паузы. У каждого патча свой переключатель. Подписи переведены на шесть языков игрового меню.
- Включили оба патча по умолчанию. Прежние значения читаются по тем же ключам в `config.toml`. Изменения действуют без перезапуска и сохраняются при выходе из Options.
- Вывели в меню уже присутствовавший **Max Speed Air Control** и перенесли **Wall Jump Fix** из оригинального MarathonRecomp.

### Игровые патчи и авторы

- **Max Speed Air Control.** На скоростных участках направление прыжка Соника можно менять левым стиком после отрыва от земли. Авторы оригинального патча — **Desko и Dunker**. Реализацию для MarathonRecomp сделал **Isaac Marovitz**. [Оригинальный патч](https://github.com/Knuxfan24/Sonic-06-Mod-Manager-Patches/blob/master/MidairControlForMachSpeed.mlua) · [Коммит MarathonRecomp](https://github.com/sonicnext-dev/MarathonRecomp/commit/359a9f20edd76cd58f7fbd10c52b3bd82728ab25)
- **Wall Jump Fix.** Наклз и Руж перестают автоматически цепляться за стены во время обычного прыжка, в том числе сразу после прыжка со стены. Захват стены при планировании доступен. Автор оригинального патча — **Nonami**. Реализацию для MarathonRecomp сделал **Hyper**. [Оригинальный патч](https://github.com/Knuxfan24/Sonic-06-Mod-Manager-Patches/blob/master/DisableClimbFromJump.mlua) · [Коммит MarathonRecomp](https://github.com/sonicnext-dev/MarathonRecomp/commit/bd87f24669947bbabf00b1114b8739db5ea279f2) · [PR #217](https://github.com/sonicnext-dev/MarathonRecomp/pull/217)

### Проверено

Пользователь подтвердил работу обоих патчей на **POCO F5** в сборке **0.2.7-modded-test1**. Релиз использует ту же нативную библиотеку и упакован как обновление обычного Android-приложения.
