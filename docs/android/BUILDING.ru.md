# Сборка MarathonRecomp-Android

[English version](BUILDING.md)

Сборка выполняется на **Windows 10/11 x64**. Linux и macOS скрипты пока не поддерживают: инструменты генерации кода собираются как программы для Windows и запускаются во время сборки под Android.

Результат — отладочный APK для **arm64-v8a, Android 10 (API 29) и новее**.

## 1. Что нужно

| Инструмент | Версия, на которой собирались проверенные сборки | Примечание |
|---|---|---|
| Git | 2.54 | с поддержкой submodule |
| Android SDK | платформа `android-35`, build-tools 34.0.0 | Android Studio или command-line tools |
| Android NDK | **29.0.14206865** | именно эта версия (`ndkVersion` в `android-apk/app/build.gradle`) |
| JDK | 17 (17.0.20.1) | для Gradle и `apksigner` |
| CMake | 4.3.3 | нужна 3.20 или новее |
| Ninja | 1.13.2 | |
| LLVM для Windows | 19.1.7 | `clang-cl.exe` и `lld-link.exe`; собирают инструменты хоста и host-порты vcpkg |
| MSVC Build Tools | 14.44.35207 | только заголовки и библиотеки (Visual Studio Build Tools, «Разработка классических приложений на C++»); сама Visual Studio не нужна |
| Windows SDK | 10.0.26100.0 | ставится вместе с Build Tools |
| PowerShell | 5.1 (встроен в Windows) | |

Нужно несколько гигабайт свободного места: папки сборки, vcpkg, FFmpeg и кэши Gradle лежат в `out/`.

### Файлы игры

Из **своей** копии игры для Xbox 360 (см. [docs/DUMPING-en.md](../DUMPING-en.md) и оригинальный [docs/BUILDING.md](../BUILDING.md)) скопируйте в `MarathonRecompLib/private/`:

- `default.xex` — из корня папки игры;
- `shader.arc` и `shader_lt.arc` — из `xenon/archives/`.

Они нужны только во время сборки, чтобы перекомпилировать код и шейдеры игры. Папка `MarathonRecompLib/private/` игнорируется git, поэтому в коммит эти файлы не попадут. **Не выкладывайте APK, собранные из ваших игровых файлов.**

## 2. Получить исходники

```powershell
git clone --recurse-submodules https://github.com/<владелец>/MarathonRecomp-Android.git
cd MarathonRecomp-Android
```

Если клонировали без `--recurse-submodules`, submodule инициализирует `Prepare-Dependencies.ps1`.

## 3. Указать пути к инструментам

```powershell
copy android\local.paths.example.json android\local.paths.json
notepad android\local.paths.json
```

Укажите все пути для своего компьютера. Файл `android/local.paths.json` игнорируется git.

Если PowerShell не даёт запускать скрипты, разрешите локальные скрипты для своей учётной записи (`Set-ExecutionPolicy -Scope CurrentUser RemoteSigned`) или разблокируйте файлы после скачивания ZIP (`Get-ChildItem -Recurse android\scripts | Unblock-File`).

## 4. Подготовить зависимости (один раз)

```powershell
.\android\scripts\Prepare-Dependencies.ps1
```

Скрипт:

1. инициализирует git submodule;
2. применяет `android/patches/plume/0001-marathon-android.patch` к `thirdparty/plume` и `android/patches/xenonrecomp/0001-merge-rounding-overrides.patch` к `tools/XenonRecomp` (если инструменты хоста собраны до v0.2.0, после этого шага запустите `Build-HostTools.ps1` ещё раз);
3. скачивает zlib 1.3.2 (проверка SHA-512) для инструментов хоста;
4. получает vcpkg на коммите `7087dcfb` и применяет официальный `thirdparty/ffmpeg-core/ffmpeg.patch`, который добавляет в FFmpeg декодер XMA от Xbox 360 (XMAFRAMES). На Windows вложенный патч может получить переводы строк CRLF; скрипт переводит его в LF и сверяет SHA-256.

Загрузки кладутся в `out/downloads`, исходники — в `out/deps`. При повторном запуске выполненные шаги пропускаются.

## 5. Проверить окружение

```powershell
.\android\scripts\Check-Environment.ps1
```

Выводит таблицу: инструменты и версии, submodule, патч Plume, зависимости, файлы игры. Ничего не собирает и не меняет.

## 6. Собрать

Всё сразу:

```powershell
.\android\scripts\Build-All.ps1
```

Или по шагам:

| Шаг | Скрипт | Результат |
|---|---|---|
| Инструменты хоста: XenonRecomp, XenosRecomp, file_to_c, u8extract (Windows, clang-cl) | `Build-HostTools.ps1` | `out/build/host-tools`, отчёт `out/logs/host-tools.json` |
| FFmpeg 7.1.1 с XMAFRAMES для Android arm64 | `Build-FFmpeg-Android.ps1` | `out/ffmpeg-android/installed/arm64-android-29-release` |
| Перекомпиляция игры и сборка `libmain.so` | `Configure-Android.ps1 -Build` | `out/build/android-arm64` |
| Упаковка APK | `Build-Apk.ps1` | `out/artifacts/MarathonRecomp-Android-debug.apk` + `.sha256` |

Логи каждого шага — в `out/logs/`. Первая сборка заново генерирует весь перекомпилированный код (около 150 больших файлов C++) и идёт долго; следующие сборки инкрементальные.

### Оптимизации XenonRecomp

`Configure-Android.ps1` по умолчанию включает проверенную группу оптимизаций XenonRecomp: регистры CTR, XER, CR и регистр резервирования как локальные переменные C++ (`-DMARATHON_RECOMP_PPC_*_AS_LOCAL=ON`), а с v0.2.0 ещё `-DMARATHON_RECOMP_PPC_MERGE_ROUNDING=ON`. Последняя опция добавляется патчем XenonRecomp из этого репозитория: векторные скалярные произведения игры переключают режим округления FPU до и после каждой инструкции (`msr fpcr`, на ARM это медленно); с опцией одно переключение покрывает серию произведений и инструкций, не зависящих от округления, и снимается перед всем, что от него зависит. Сгенерированный код проверен по инструкциям (ни одна не выполняется с другим режимом округления). В оригинальном MarathonRecomp они все выключены. Собрать консервативный вариант:

```powershell
.\android\scripts\Configure-Android.ps1 -BuildDir out\build\android-arm64-upstream -CMakeArgs @() -Build
.\android\scripts\Build-Apk.ps1 -NativeBuildDir out\build\android-arm64-upstream -ArtifactName MarathonRecomp-Android-upstream-debug
```

Смена набора меняет раскладку `PPCContext` и перегенерирует весь перекомпилированный код, поэтому для каждого набора нужна своя папка сборки. Остальные опции (`NON_ARGUMENT_AS_LOCAL`, `NON_VOLATILE_AS_LOCAL`, `SKIP_LR`, `SKIP_MSR`) не проверялись.

## 7. Установить и запустить

```powershell
adb install -r out\artifacts\MarathonRecomp-Android-debug.apk
```

Затем откройте **Marathon Recompiled**, перейдите в **«Файлы игры» → «Установить игру / DLC (.zip / папка)»** и выберите ZIP-архив или папку своей копии для Xbox 360. Когда `game/default.xex` и `game/xenon/archives` на месте, лаунчер показывает **«Игра готова»**.

Можно и скопировать файлы по USB через `adb push` в `/sdcard/Android/data/io.github.sansnope.marathonrecomp.android/files/MarathonRecomp/game`. **Перед этим один раз откройте лаунчер**, чтобы приложение само создало папку `MarathonRecomp`: папка, созданная через `adb`, может оказаться недоступной приложению для записи.

### Подпись

Отладочный APK подписывается отладочным ключом Android того компьютера, на котором он собран (`%USERPROFILE%\.android\debug.keystore`). APK, собранный на другом компьютере, не сможет обновить ваш. Android тогда потребует сначала удалить приложение, а **удаление стирает игру, настройки и сохранения в `Android/data`**. Сначала скопируйте сохранения (лаунчер → «Файлы игры» → «Открыть папку сохранений»).

### Release APK

Официальный v0.1.0 — сборка Release с выключенной отладкой, versionCode3 и отдельным ключом проекта. Цепочка сертификатов сохраняет путь обновления от прежних сборок владельца. Приватные ключи и настройки остаются вне Git; публичный сертификат, lineage и пример конфигурации — в [android/signing](../../android/signing/README.md).

После сборки native:

```powershell
.\android\scripts\Build-Apk.ps1 -Variant Release -ArtifactName MarathonRecomp-Android-v0.1.0-arm64-v8a -SigningConfig C:\private\signing.json
```

Путь можно задать через `MARATHON_RELEASE_SIGNING_CONFIG`. Без конфигурации подписи release-упаковка завершится ошибкой и не выдаст неподписанный APK за готовый релиз. Храните независимые копии keystore, паролей и lineage. Сборка с самостоятельно созданным другим ключом не обновит официальный релиз.

## 8. Если что-то не так

| Проблема | Что делать |
|---|---|
| `Missing configured tool path` | Путь из `android/local.paths.json` не существует. |
| vcpkg не находит Visual Studio | Host-порты собираются триплетом `x64-windows-llvm-host` через clang-cl; проверьте `host_llvm_bin`, `msvc_root` и Windows SDK. Запускайте сборку только скриптами, а не голым `cmake`, — переменные выставляет `Enter-Environment.ps1`. |
| `XMAFRAMES patch missing or not the verified LF copy` | Запустите `Prepare-Dependencies.ps1` ещё раз: он исправляет переводы строк. |
| Патч Plume не применяется | `thirdparty/plume` не на закреплённом коммите: `git submodule update --init --recursive`, затем `Prepare-Dependencies.ps1`. |
| `Missing MarathonRecompLib\private\default.xex` | Скопируйте файлы игры (раздел 1). |
| Игра не запускается на GPU не от Adreno | Встроенные драйверы только для Adreno; на других GPU используется системный драйвер Vulkan. Такие устройства не проверялись. |
| Чёрный экран или сбой драйвера после смены драйвера | Следующий запуск автоматически откатится на системный драйвер; выберите в лаунчере «Графика» → **«Авто (рекомендуется)»**. |

На телефоне лог лежит в `Android/data/io.github.sansnope.marathonrecomp.android/files/log.txt` (лаунчер → «Дополнительно» → «Открыть логи»). Прикладывайте его к сообщению о проблеме.
