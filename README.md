# MarathonRecomp-Android

**An unofficial Android port of [MarathonRecomp](https://github.com/sonicnext-dev/MarathonRecomp)** — the static recompilation of the Xbox 360 version of *Sonic the Hedgehog* (2006). The game runs natively on arm64 Android phones: the original PowerPC code and Xenos shaders are recompiled to C++ and SPIR-V and rendered with Vulkan.

[Русская версия](README.ru.md) · [Building](docs/android/BUILDING.md) · [What was done and why](docs/android/TECHNICAL.md) · [Test status](docs/android/STATUS.md) · [Info / credits](INFO.md)

> [!CAUTION]
> - **This is not an official project.** It is an Android port of another project from GitHub, MarathonRecomp. It is not affiliated with the MarathonRecomp team (sonicnext-dev) or with SEGA. Please do not report Android problems to the upstream project.
> - **No game files are included.** You need your own legally acquired copy of the Xbox 360 game, both to build and to play.
> - **We do not promise that it works on every device.** It has been tested on two phones (see below). Other phones, GPUs and Android versions may not work, or may work worse.
> - MarathonRecomp itself is still in active development and is not meant for public use yet; this port inherits that state.

## Status

| Device | SoC / GPU | Android | Result |
|---|---|---|---|
| POCO F5 | Snapdragon 7+ Gen 2 (SM7475) / Adreno 725 | 17 | Menus, HUB with events, a boss, Wave Ocean (completed), Dusty Desert. Sound, touch controls and saves work. **60 FPS** in the HUB and in levels at 60 % render scale; short drops while scenes load. |
| realme RMX5090 | Snapdragon 8 Elite (SM8750) / Adreno 830 | 16 | HUB with events and a level at **125 %** render scale, no errors; the tester describes the performance as excellent (not measured). |

Not tested: other Adreno generations, Mali, Xclipse and PowerVR GPUs, Android 10–15, DLC, a full playthrough, physical gamepads, long sessions on battery. Details and known issues: [docs/android/STATUS.md](docs/android/STATUS.md).

## What the port adds

- **Android build** of MarathonRecomp with the NDK (arm64-v8a, Android 10+), SDL3, FFmpeg with the Xbox 360 XMA decoder, and Vulkan through Plume.
- **Launcher** in the style of the 2006 menus: installs the game from a ZIP or a folder, picks the storage, Vulkan driver and render mode, touch-control options and layout editor, opens logs and saves, and shows the [Info](INFO.md) window. English and Russian (follows the system language).
- **Vulkan drivers:** bundled Mesa Turnip builds for Adreno (selected automatically) and import of your own driver (`.so` or AdrenoTools/ExynosTools `.zip`), with automatic fallback after a failed driver start.
- **On-screen touch controls** with a layout editor.
- **Android-specific performance work:** swap-chain pre-rotation (no extra GPU composition by the system), worker threads kept off the little CPU cores when the SoC has them, a tested set of XenonRecomp optimizations, render-scale handling that keeps the HUD at native resolution.
- **Diagnostics:** `log.txt` with device info and a crash report, optional per-frame timing.

How and why each part was done: [docs/android/TECHNICAL.md](docs/android/TECHNICAL.md).

## Playing

1. Download the APK from [GitHub Releases](https://github.com/SansNope/MarathonRecomp-Android/releases/latest), or [build it yourself](docs/android/BUILDING.md), and install it. See the [v0.1.0 release notes](docs/android/RELEASE-v0.1.0.md).
2. Copy the game folder of your Xbox 360 copy to the phone and choose **Game files → Install game / DLC (.zip / folder)** in the launcher. The launcher shows when the game is ready and how many DLC packs were found.
3. Press **Launch game**. On Adreno GPUs keep the driver on **Auto (recommended)** unless you know you need another one.

## Building

Windows 10/11 x64 with PowerShell, Android SDK/NDK 29, JDK 17, CMake, Ninja, LLVM (clang-cl) and the MSVC build tools. In short:

```powershell
git clone --recurse-submodules <this repository>
copy android\local.paths.example.json android\local.paths.json   # then edit the paths
# copy default.xex, shader.arc and shader_lt.arc from your game into MarathonRecompLib\private\
.\android\scripts\Prepare-Dependencies.ps1
.\android\scripts\Build-All.ps1
```

The APK ends up in `out\artifacts\`. Full instructions, requirements and troubleshooting: [docs/android/BUILDING.md](docs/android/BUILDING.md).

## Repository layout

| Path | Contents |
|---|---|
| `MarathonRecomp/`, `MarathonRecompLib/`, `thirdparty/`, `tools/` | MarathonRecomp sources with the Android changes (Android code mostly in `MarathonRecomp/os/android/` and `MarathonRecomp/ui/touch_controls.*`). |
| `android-apk/` | Gradle project: launcher, game activity, documents provider, icon. |
| `android/scripts/` | Build scripts (PowerShell). |
| `android/patches/plume/` | Android changes for the `thirdparty/plume` submodule, applied by `Prepare-Dependencies.ps1`. |
| `android/turnip-drivers/` | Bundled Vulkan drivers (prebuilt Mesa Turnip, see its README and `SHA256SUMS`). |
| `android/vcpkg-triplets/` | vcpkg triplets for Android and the clang-cl host. |
| `docs/android/` | Building, technical notes, test status. |
| `docs/MarathonRecomp-README.md` | The original MarathonRecomp README (credits of the MarathonRecomp team). |

## Info and credits

See [INFO.md](INFO.md) — the same text as the launcher's Info window, in English and Russian — and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

MarathonRecomp is made by the [sonicnext-dev](https://github.com/sonicnext-dev) team and contributors; see [docs/MarathonRecomp-README.md](docs/MarathonRecomp-README.md). The touch controls, launcher, driver loader and storage code come from [UnleashedRecomp-Android](https://github.com/SansNope/UnleashedRecomp-Android).

## License

MarathonRecomp is licensed under the GNU General Public License v3.0 ([COPYING](COPYING)); the Android port is distributed under the same license. Third-party components keep their own licenses ([THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)). *Sonic the Hedgehog* is a trademark of SEGA; no game content is included in this repository.
