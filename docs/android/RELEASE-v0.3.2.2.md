# MarathonRecomp-Android v0.3.2.2

**Shared save folder.** You can now store your progress and achievements in a shared folder for tools such as Syncthing. By default, the game continues to use the saves in your existing installation.

1. Fully close the game and reopen the launcher. Go to **Game files → Choose save location → Shared folder (Android/media)**.
2. Choose **Copy current saves** to bring your progress and achievements with you. The launcher keeps the originals and stops the copy if it finds a different save in the destination. Choose **Use this folder** if it already contains the saves you want to play with.
3. Point your sync app to the path shown in the launcher. On primary storage it is `/sdcard/Android/media/io.github.sansnope.marathonrecomp.android/saves`. Close the game on one device and wait for sync to finish before opening it on the other.

Copying saves, loading existing progress, and writing a new save to the shared folder were verified on **POCO F5**. Version **0.3.2.2 (code 20)** requires Android 10 (API 29) or later, ARM64, and Vulkan.

[Full source diff](https://github.com/SansNope/MarathonRecomp-Android/compare/v0.3.2.1...v0.3.2.2)

---

## Русский

**Общая папка сохранений.** Теперь прогресс и достижения можно хранить в общей папке для синхронизации через Syncthing или другое приложение. По умолчанию игра использует сохранения из существующей установки.

1. Полностью закройте игру и снова откройте лаунчер. Выберите **Файлы игры → Выбрать папку сохранений → В общей папке (Android/media)**.
2. Нажмите **Копировать сохранения**, чтобы перенести прогресс и достижения. Исходные файлы сохраняются. Если в папке назначения уже лежит другая копия, перенос остановится. Чтобы играть с сейвами из этой папки, нажмите **Использовать эту папку**.
3. Укажите в приложении синхронизации путь из лаунчера. На основном накопителе это `/sdcard/Android/media/io.github.sansnope.marathonrecomp.android/saves`. Перед переходом на другое устройство закройте игру и дождитесь завершения синхронизации.

Копирование сейвов, загрузка прежнего прогресса и запись нового сохранения в общую папку проверены на **POCO F5**. Версия **0.3.2.2 (код 20)** требует Android 10 (API 29) или новее, ARM64 и Vulkan.

[Полный список изменений](https://github.com/SansNope/MarathonRecomp-Android/compare/v0.3.2.1...v0.3.2.2)
