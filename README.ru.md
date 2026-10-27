# MarathonRecomp-Android

**Неофициальный порт [MarathonRecomp](https://github.com/sonicnext-dev/MarathonRecomp) на Android** — статической рекомпиляции версии *Sonic the Hedgehog* (2006) для Xbox 360. Игра работает нативно на arm64-смартфонах: исходный код PowerPC и шейдеры Xenos перекомпилированы в C++ и SPIR-V, вывод идёт через Vulkan.

[English version](README.md) · [Сборка](docs/android/BUILDING.ru.md) · [Что и зачем сделано](docs/android/TECHNICAL.ru.md) · [Что проверено](docs/android/STATUS.ru.md) · [Info / благодарности](INFO.md)

> [!CAUTION]
> - **Это не официальный проект.** Это порт на Android другого проекта с GitHub — MarathonRecomp. Он не связан ни с командой MarathonRecomp (sonicnext-dev), ни с SEGA. Пожалуйста, не отправляйте проблемы Android-версии в оригинальный проект.
> - **Игровые файлы не входят в репозиторий.** Для сборки и для игры нужна ваша собственная законно приобретённая копия игры для Xbox 360.
> - **Мы не обещаем работу на всех устройствах.** Порт проверен на двух телефонах (см. ниже). На других телефонах, GPU и версиях Android он может не работать или работать хуже.
> - Сам MarathonRecomp ещё в активной разработке и пока не предназначен для широкого использования; этот порт наследует это состояние.

## Что проверено

| Устройство | SoC / GPU | Android | Результат |
|---|---|---|---|
| POCO F5 | Snapdragon 7+ Gen 2 (SM7475) / Adreno 725 | 17 | Меню, HUB с событиями, босс, Wave Ocean (пройден), Dusty Desert. Звук, сенсорное управление и сохранения работают. **60 FPS** в HUB и на уровнях при разрешении рендера 60 %; короткие просадки во время загрузки сцен. |
| realme RMX5090 | Snapdragon 8 Elite (SM8750) / Adreno 830 | 16 | HUB с событиями и уровень при разрешении рендера **125 %**, без ошибок; производительность по оценке тестировщика отличная (не замерялась). |

**GPU Mali: с v0.1.6 игра запускается и играется** на Mali-G57 MC2 (Helio G99) и Mali-G710 (Pixel 7 Pro), но производительность на Mali пока плохая — работаем над этим (см. [STATUS](docs/android/STATUS.ru.md)).

Не проверено: другие поколения Adreno, GPU Mali, Xclipse и PowerVR, Android 10–15, DLC, полное прохождение, физические геймпады, долгая игра от батареи. Подробности и известные проблемы: [docs/android/STATUS.ru.md](docs/android/STATUS.ru.md).

## Что добавляет порт

- **Сборка MarathonRecomp под Android** через NDK (arm64-v8a, Android 10+): SDL3, FFmpeg с декодером XMA от Xbox 360, Vulkan через Plume.
- **Лаунчер** в стиле меню 2006 года: установка игры из ZIP или папки, выбор хранилища, драйвера Vulkan и режима рендера, настройки и редактор сенсорного управления, доступ к логам и сохранениям, окно [Info](INFO.md). Русский и английский язык (по языку системы).
- **Драйверы Vulkan:** встроенные сборки Mesa Turnip для Adreno (выбираются автоматически) и импорт своего драйвера (`.so` или `.zip` AdrenoTools/ExynosTools), автоматический откат после неудачного запуска драйвера.
- **Экспериментальная поддержка Mali / PowerVR** (из UnleashedRecomp-Android): системный драйвер Vulkan, текстуры BC перекодируются в ETC2/EAC при загрузке.
- **Сенсорное управление** на экране с редактором раскладки.
- **Оптимизации под Android:** pre-rotation swapchain (система не тратит GPU на поворот кадра), рабочие потоки уводятся с малых ядер процессора, если они есть, проверенный набор оптимизаций XenonRecomp, масштаб разрешения, при котором HUD остаётся в родном разрешении.
- **Диагностика:** `log.txt` со сведениями об устройстве и отчётом о сбое, опциональные замеры времени кадра.

Как и зачем сделана каждая часть: [docs/android/TECHNICAL.ru.md](docs/android/TECHNICAL.ru.md).

## Как играть

1. Скачайте APK из [GitHub Releases](https://github.com/SansNope/MarathonRecomp-Android/releases/latest) или [соберите самостоятельно](docs/android/BUILDING.ru.md) и установите его. Описание релизов: [v0.1.6](docs/android/RELEASE-v0.1.6.md), [v0.1.5](docs/android/RELEASE-v0.1.5.md), [v0.1.0](docs/android/RELEASE-v0.1.0.md).
2. Скопируйте папку игры из своей копии для Xbox 360 на телефон и выберите в лаунчере **«Файлы игры» → «Установить игру / DLC (.zip / папка)»**. Лаунчер покажет, что игра готова, и сколько DLC найдено.
3. Нажмите **«Запустить игру»**. На Adreno оставьте драйвер **«Авто (рекомендуется)»**, если не знаете точно, что нужен другой.

## Сборка

Windows 10/11 x64 с PowerShell, Android SDK/NDK 29, JDK 17, CMake, Ninja, LLVM (clang-cl) и MSVC Build Tools. Коротко:

```powershell
git clone --recurse-submodules <этот репозиторий>
copy android\local.paths.example.json android\local.paths.json   # затем укажите свои пути
# скопируйте default.xex, shader.arc и shader_lt.arc из своей игры в MarathonRecompLib\private\
.\android\scripts\Prepare-Dependencies.ps1
.\android\scripts\Build-All.ps1
```

Готовый APK появится в `out\artifacts\`. Полная инструкция, требования и решение проблем: [docs/android/BUILDING.ru.md](docs/android/BUILDING.ru.md).

## Структура репозитория

| Путь | Что там |
|---|---|
| `MarathonRecomp/`, `MarathonRecompLib/`, `thirdparty/`, `tools/` | Исходники MarathonRecomp с изменениями для Android (Android-код в основном в `MarathonRecomp/os/android/` и `MarathonRecomp/ui/touch_controls.*`). |
| `android-apk/` | Gradle-проект: лаунчер, игровая activity, провайдер файлов, иконка. |
| `android/scripts/` | Скрипты сборки (PowerShell). |
| `android/patches/plume/` | Изменения для submodule `thirdparty/plume`, применяются `Prepare-Dependencies.ps1`. |
| `android/turnip-drivers/` | Встроенные драйверы Vulkan (готовые сборки Mesa Turnip, см. README и `SHA256SUMS`). |
| `android/vcpkg-triplets/` | Триплеты vcpkg для Android и для хоста с clang-cl. |
| `docs/android/` | Сборка, технические заметки, статус проверки. |
| `docs/MarathonRecomp-README.md` | Оригинальный README MarathonRecomp (авторы команды MarathonRecomp). |

## Info и благодарности

См. [INFO.md](INFO.md) — тот же текст, что в окне «Инфо» лаунчера, на русском и английском — и [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

MarathonRecomp создан командой [sonicnext-dev](https://github.com/sonicnext-dev) и участниками; см. [docs/MarathonRecomp-README.md](docs/MarathonRecomp-README.md). Сенсорное управление, лаунчер, загрузчик драйверов и код хранилища взяты из [UnleashedRecomp-Android](https://github.com/SansNope/UnleashedRecomp-Android).

## Лицензия

MarathonRecomp распространяется по лицензии GNU General Public License v3.0 ([COPYING](COPYING)); порт на Android — по той же лицензии. Сторонние компоненты сохраняют свои лицензии ([THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)). *Sonic the Hedgehog* — товарный знак SEGA; игровых материалов в репозитории нет.
