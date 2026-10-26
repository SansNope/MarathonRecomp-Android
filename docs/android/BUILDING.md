# Building MarathonRecomp-Android

[Русская версия](BUILDING.ru.md)

The build runs on **Windows 10/11 x64**. Linux and macOS are not supported by the scripts yet: the code-generation tools are built as Windows executables and run during the Android build.

The result is a debug APK for **arm64-v8a, Android 10 (API 29) or newer**.

## 1. Requirements

| Tool | Version used for the tested builds | Notes |
|---|---|---|
| Git | 2.54 | with submodule support |
| Android SDK | platform `android-35`, build-tools 34.0.0 | Android Studio or the command-line tools |
| Android NDK | **29.0.14206865** | exactly this version (`ndkVersion` in `android-apk/app/build.gradle`) |
| JDK | 17 (17.0.20.1) | for Gradle and `apksigner` |
| CMake | 4.3.3 | 3.20 or newer is required |
| Ninja | 1.13.2 | |
| LLVM for Windows | 19.1.7 | `clang-cl.exe` and `lld-link.exe`; builds the host tools and the vcpkg host ports |
| MSVC build tools | 14.44.35207 | headers and libraries only (Visual Studio Build Tools, "Desktop development with C++"); the Visual Studio IDE is not needed |
| Windows SDK | 10.0.26100.0 | installed with the Build Tools |
| PowerShell | 5.1 (built into Windows) | |

Plan for several GB of free disk space: the build directories, vcpkg, FFmpeg and Gradle caches all live in `out/`.

### Game files

From **your own** copy of the Xbox 360 game (see [docs/DUMPING-en.md](../DUMPING-en.md) and the upstream [docs/BUILDING.md](../BUILDING.md)) copy into `MarathonRecompLib/private/`:

- `default.xex` — from the root of the game folder;
- `shader.arc` and `shader_lt.arc` — from `xenon/archives/`.

They are used only at build time to recompile the game code and shaders. `MarathonRecompLib/private/` is ignored by git, so these files never end up in a commit. **Do not publish APKs built from your game files.**

## 2. Get the sources

```powershell
git clone --recurse-submodules https://github.com/<owner>/MarathonRecomp-Android.git
cd MarathonRecomp-Android
```

If you cloned without `--recurse-submodules`, `Prepare-Dependencies.ps1` initialises the submodules for you.

## 3. Configure the tool paths

```powershell
copy android\local.paths.example.json android\local.paths.json
notepad android\local.paths.json
```

Set every path for your machine. `android/local.paths.json` is ignored by git.

If PowerShell refuses to run the scripts, allow local scripts for your user account (`Set-ExecutionPolicy -Scope CurrentUser RemoteSigned`), or unblock the files after downloading a ZIP (`Get-ChildItem -Recurse android\scripts | Unblock-File`).

## 4. Prepare the dependencies (once)

```powershell
.\android\scripts\Prepare-Dependencies.ps1
```

This script:

1. initialises the git submodules;
2. applies `android/patches/plume/0001-marathon-android.patch` to `thirdparty/plume`;
3. downloads zlib 1.3.2 (checked by SHA-512) for the host tools;
4. checks out vcpkg at `7087dcfb` and applies the official `thirdparty/ffmpeg-core/ffmpeg.patch`, which adds the Xbox 360 XMA decoder (XMAFRAMES) to FFmpeg. On Windows the nested patch can get CRLF line endings; the script converts it to LF and checks its SHA-256.

Downloads go to `out/downloads`, sources to `out/deps`. Finished steps are skipped when the script is run again.

## 5. Check the environment

```powershell
.\android\scripts\Check-Environment.ps1
```

Prints a table of tools, versions, submodules, the Plume patch, dependencies and game files. Nothing is built or changed.

## 6. Build

Everything in one go:

```powershell
.\android\scripts\Build-All.ps1
```

Or step by step:

| Step | Script | Output |
|---|---|---|
| Host tools: XenonRecomp, XenosRecomp, file_to_c, u8extract (Windows, clang-cl) | `Build-HostTools.ps1` | `out/build/host-tools`, report `out/logs/host-tools.json` |
| FFmpeg 7.1.1 with XMAFRAMES for Android arm64 | `Build-FFmpeg-Android.ps1` | `out/ffmpeg-android/installed/arm64-android-29-release` |
| Recompile the game, build `libmain.so` | `Configure-Android.ps1 -Build` | `out/build/android-arm64` |
| Package the APK | `Build-Apk.ps1` | `out/artifacts/MarathonRecomp-Android-debug.apk` + `.sha256` |

Logs of every step are in `out/logs/`. The first native build regenerates all recompiled code (about 150 large C++ files) and takes a while; later builds are incremental.

### XenonRecomp optimizations

`Configure-Android.ps1` enables the tested group of XenonRecomp optimizations by default: CTR, XER, CR and the reservation register as C++ locals (`-DMARATHON_RECOMP_PPC_*_AS_LOCAL=ON`). Upstream MarathonRecomp keeps all of them off. To build the conservative set:

```powershell
.\android\scripts\Configure-Android.ps1 -BuildDir out\build\android-arm64-upstream -CMakeArgs @() -Build
.\android\scripts\Build-Apk.ps1 -NativeBuildDir out\build\android-arm64-upstream -ArtifactName MarathonRecomp-Android-upstream-debug
```

Changing the set changes the layout of `PPCContext` and regenerates all recompiled code, so always use a separate build directory for each set. The other options (`NON_ARGUMENT_AS_LOCAL`, `NON_VOLATILE_AS_LOCAL`, `SKIP_LR`, `SKIP_MSR`) are untested.

## 7. Install and run

```powershell
adb install -r out\artifacts\MarathonRecomp-Android-debug.apk
```

Then open **Marathon Recompiled**, go to **Game files → Install game / DLC (.zip / folder)** and pick a ZIP archive or the folder of your Xbox 360 copy. The launcher shows **Game ready** when `game/default.xex` and `game/xenon/archives` are in place.

Alternatively copy the files over USB with `adb push` into `/sdcard/Android/data/io.github.sansnope.marathonrecomp.android/files/MarathonRecomp/game`. **Open the launcher once before that**, so the app creates its `MarathonRecomp` folder itself: a folder created by `adb` can be unwritable for the app.

### Signing

The debug APK is signed with the Android debug key of the machine that built it (`%USERPROFILE%\.android\debug.keystore`). An APK built on another machine cannot update yours. Android then requires uninstalling first, and **uninstalling deletes the game, the settings and the saves stored in `Android/data`**. Copy your saves first (launcher → Game files → Open saves folder).

### Release APK

The official v0.1.0 APK is a non-debuggable Release build (versionCode3), signed with the project's dedicated key. A certificate lineage preserves its update path from the maintainer's development builds. The private keys/configuration are kept outside Git; public verification material and a configuration example are in [android/signing](../../android/signing/README.md).

After the native build, package and sign with:

```powershell
.\android\scripts\Build-Apk.ps1 -Variant Release -ArtifactName MarathonRecomp-Android-v0.1.0-arm64-v8a -SigningConfig C:\private\signing.json
```

`MARATHON_RELEASE_SIGNING_CONFIG` can supply the configuration path instead. Release packaging fails if signing is not configured; it does not silently copy an unsigned APK. Keep independent backups of the keystore, passwords and lineage. APKs signed with independently generated keys cannot update official releases.

## 8. Troubleshooting

| Problem | What to do |
|---|---|
| `Missing configured tool path` | A path in `android/local.paths.json` does not exist. |
| vcpkg cannot find Visual Studio | The host ports use the `x64-windows-llvm-host` triplet with clang-cl; make sure `host_llvm_bin`, `msvc_root` and the Windows SDK are set. Always run the scripts, not plain `cmake`, so `Enter-Environment.ps1` sets the variables. |
| `XMAFRAMES patch missing or not the verified LF copy` | Run `Prepare-Dependencies.ps1` again; it normalises the line endings. |
| The Plume patch does not apply | `thirdparty/plume` is not at the pinned commit: `git submodule update --init --recursive`, then `Prepare-Dependencies.ps1`. |
| `Missing MarathonRecompLib\private\default.xex` | Copy the game files (section 1). |
| The game does not start on a non-Adreno GPU | The bundled drivers are Adreno-only; the game uses the system Vulkan driver there. Non-Adreno devices are untested. |
| Black screen or a driver crash after changing the driver | The next launch falls back to the system driver automatically; choose **Auto (recommended)** under Graphics in the launcher. |

On the phone the log is `Android/data/io.github.sansnope.marathonrecomp.android/files/log.txt` (launcher → Advanced → Open logs). Attach it when you report a problem.
