# MarathonRecomp-Android v0.2.7

**Separate on-screen D-pad and three fixes from the original MarathonRecomp.** ARM64, Android 10 (API 29) or newer, Vulkan. Version **0.2.7 (code 13)** uses the same signing key and can update v0.2.6 or 0.2.7-test1 in place.

## Changes

- **Separate D-pad.** The on-screen D-pad works alongside the left analog stick, so you can move and switch Sonic’s gems at the same time. Move or resize it under **Controls → Arrange touch controls**. Existing layouts keep their saved positions and get the D-pad in its default position. Thanks to **foxyexe143** for reporting the missing separate control.
- **Radical Train progression.** The mirror can open when Sonic already has the Bounce Bracelet but the mission’s shop-purchase flag is missing. The Android port limits the check to the mirror’s Lua event. Original fix by **Isaac Marovitz (IsaacMarovitz)**. [PR #462](https://github.com/sonicnext-dev/MarathonRecomp/pull/462) · [commit b91b317](https://github.com/sonicnext-dev/MarathonRecomp/commit/b91b317372e47c3bf477ef723b41200d1954fb78)
- **Gamepad connection.** A controller that SDL fails to open no longer occupies a player slot. The log records the device ID and SDL error. Original fix by **Sajid (Sajidur78)**. [PR #444](https://github.com/sonicnext-dev/MarathonRecomp/pull/444) · [commit 755abf2](https://github.com/sonicnext-dev/MarathonRecomp/commit/755abf2642e675aafcec7169d73a14d5f037a210)
- **DLC loading.** The Android port registers a DLC folder only if it contains a regular `download.arc` file, preventing incomplete folders from blocking loading. Original fix by **squidbus**. [PR #437](https://github.com/sonicnext-dev/MarathonRecomp/pull/437) · [commit 53b7989](https://github.com/sonicnext-dev/MarathonRecomp/commit/53b79896ce82161279c59971e8e9a09a25210bcb)

## Tested

The separate D-pad was confirmed working on **POCO F5** with **0.2.7-test1**. The release APK ships the same code, libraries and assets. The three upstream fixes still await separate in-game verification on Android.

---

## Русский

**Отдельная экранная крестовина и три исправления из оригинального MarathonRecomp.** ARM64, Android 10 (API 29) и новее, Vulkan. Версия **0.2.7 (код 13)** подписана тем же ключом и устанавливается поверх v0.2.6 или 0.2.7-test1.

### Изменения

- **Отдельная крестовина.** Экранная крестовина работает вместе с левым аналоговым стиком. Можно двигаться и одновременно переключать камни Соника. Положение и размер настраиваются в меню **Управление → Расставить экранные кнопки**. В старой раскладке крестовина появляется в стандартном месте, позиции остальных кнопок сохраняются. Спасибо **foxyexe143** за замечание.
- **Проход в Radical Train.** Зеркало может открыться, если у Соника уже есть Bounce Bracelet, а флаг покупки в этой миссии отсутствует. В Android-порте проверка ограничена Lua-событием зеркала. Автор оригинального исправления — **Isaac Marovitz (IsaacMarovitz)**. [PR #462](https://github.com/sonicnext-dev/MarathonRecomp/pull/462) · [коммит b91b317](https://github.com/sonicnext-dev/MarathonRecomp/commit/b91b317372e47c3bf477ef723b41200d1954fb78)
- **Подключение геймпадов.** Если SDL не удалось открыть контроллер, он не занимает слот игрока. В журнал записываются ID устройства и ошибка SDL. Автор оригинального исправления — **Sajid (Sajidur78)**. [PR #444](https://github.com/sonicnext-dev/MarathonRecomp/pull/444) · [коммит 755abf2](https://github.com/sonicnext-dev/MarathonRecomp/commit/755abf2642e675aafcec7169d73a14d5f037a210)
- **Загрузка DLC.** Android-порт регистрирует папку DLC, только если в ней есть обычный файл `download.arc`. Это устраняет зависание при загрузке из-за неполных папок. Автор оригинального исправления — **squidbus**. [PR #437](https://github.com/sonicnext-dev/MarathonRecomp/pull/437) · [коммит 53b7989](https://github.com/sonicnext-dev/MarathonRecomp/commit/53b79896ce82161279c59971e8e9a09a25210bcb)

### Проверено

Пользователь подтвердил работу отдельной крестовины на **POCO F5** в **0.2.7-test1**. Код, библиотеки и ресурсы в релизном APK совпадают с тестовой сборкой. Для трёх исправлений из оригинала ещё предстоит отдельная проверка в игре на Android.
