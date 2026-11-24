# MarathonRecomp-Android v0.3.4.1 Hotfix

Restores missing scene graphics reported on the Infinix X6886 with Mali-G57 MC2. The tester confirmed that Sonic's model in Act Trial and the stage scenery were visible again.

- The Android hotfix disables user clip planes in Vulkan using the same mode tested in `0.3.4.1-test1`.
- Resource uploads that arrive between Vulkan command lists are recorded in the next list. Uploads and resource releases keep their order.

The release packages the same native library tested on the Infinix X6886. Android 10 or later, ARM64 and Vulkan are required. Thanks to the tester for checking the menu and gameplay. [Report an issue](https://github.com/SansNope/MarathonRecomp-Android/issues)

[Full source diff](https://github.com/SansNope/MarathonRecomp-Android/compare/v0.3.4...v0.3.4.1)

---

## Русский

Хотфикс восстанавливает пропадавшую графику на Infinix X6886 с Mali-G57 MC2. Тестер подтвердил, что модель Соника в меню «Испытания актов» и окружение на уровне снова отображаются.

- В Vulkan-рендерере Android отключены пользовательские плоскости отсечения. Этот режим проверен в `0.3.4.1-test1`.
- Загрузки ресурсов, поступившие между списками команд Vulkan, выполняются в следующем списке. Порядок загрузок и освобождения ресурсов сохраняется.

В релиз вошла та же нативная библиотека, которую проверили на Infinix X6886. Требуются Android 10 или новее, ARM64 и Vulkan. Спасибо тестеру за проверку меню и игрового процесса. [Сообщить о проблеме](https://github.com/SansNope/MarathonRecomp-Android/issues)

[Полный список изменений](https://github.com/SansNope/MarathonRecomp-Android/compare/v0.3.4...v0.3.4.1)
