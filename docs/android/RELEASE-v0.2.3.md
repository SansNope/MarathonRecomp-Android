# MarathonRecomp-Android v0.2.3

**More fixes from the original MarathonRecomp.** ARM64, Android 10 (API 29) or newer, Vulkan. Installs over v0.2.2 and keeps your saves.

## Changes

These fixes were **ported from the original [MarathonRecomp](https://github.com/sonicnext-dev/MarathonRecomp) repository** by the MarathonRecomp team; all credit goes to them:

- **[#339](https://github.com/sonicnext-dev/MarathonRecomp/pull/339)** — connecting a gamepad during play no longer runs into undefined behaviour in the input code.
- **[#342](https://github.com/sonicnext-dev/MarathonRecomp/pull/342)** — waiting for the GPU uses its own command list. On Android this wait runs when the game returns from the background.
- **[#340](https://github.com/sonicnext-dev/MarathonRecomp/pull/340)** — button icons in multi-page texts show the right buttons on the second and later pages; with Controller Icons set to Auto, the icons are chosen at the main menu and no longer change in the middle of a stage.

#339 is unchanged. #342 and #340 were applied by hand to this port's code, with the same behaviour as upstream.

Version 0.2.3 (code 9), same signing key.

## Tested

- **POCO F5** (Adreno 725): the 0.2.3 test build works without problems. The release APK has the same native library and assets as that test build.

## Known issues

Unchanged from [v0.2.1](https://github.com/SansNope/MarathonRecomp-Android/blob/main/docs/android/RELEASE-v0.2.1.md#known-issues): loading screens and videos flicker on a Helio G99 phone, and everything listed for [v0.2.0](https://github.com/SansNope/MarathonRecomp-Android/blob/main/docs/android/RELEASE-v0.2.0.md#known-issues).

---

## Русский

**Новые исправления из оригинального MarathonRecomp.** ARM64, Android 10 (API 29) и новее, Vulkan. Ставится поверх v0.2.2 с сохранением данных.

### Изменения

Эти исправления **перенесены из оригинального репозитория [MarathonRecomp](https://github.com/sonicnext-dev/MarathonRecomp)**; их авторы — команда MarathonRecomp:

- **[#339](https://github.com/sonicnext-dev/MarathonRecomp/pull/339)** — подключение геймпада во время игры больше не приводит к неопределённому поведению в коде ввода.
- **[#342](https://github.com/sonicnext-dev/MarathonRecomp/pull/342)** — ожидание GPU выполняется через отдельный список команд. На Android это ожидание срабатывает при возврате игры из фона.
- **[#340](https://github.com/sonicnext-dev/MarathonRecomp/pull/340)** — иконки кнопок в многостраничных текстах показывают правильные кнопки на второй и следующих страницах; при значении «Авто» для иконок контроллера они выбираются в главном меню и больше не меняются посреди уровня.

#339 перенесён без изменений. #342 и #340 внесены в код порта вручную, поведение то же, что в оригинале.

Версия 0.2.3 (код 9), тот же ключ подписи.

### Проверено

- **POCO F5** (Adreno 725): тестовая сборка 0.2.3 работает без проблем. В релизном APK та же нативная библиотека и те же ресурсы, что в этой тестовой сборке.

### Известные проблемы

Без изменений с [v0.2.1](https://github.com/SansNope/MarathonRecomp-Android/blob/main/docs/android/RELEASE-v0.2.1.md#known-issues): мерцание загрузочных экранов и роликов на телефоне с Helio G99, а также всё, что перечислено для [v0.2.0](https://github.com/SansNope/MarathonRecomp-Android/blob/main/docs/android/RELEASE-v0.2.0.md#known-issues).
