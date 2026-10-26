# What was done for Android, how and why

[Русская версия](TECHNICAL.ru.md)

MarathonRecomp turns the Xbox 360 executable and shaders into C++ and shader code at build time and runs them through its own kernel, audio and graphics layers. The port keeps that design and adds what Android needs. Base: MarathonRecomp `a66d4fb` ("Rewritten HID backend").

## 1. Build system

| What | How | Why |
|---|---|---|
| Host/target split | `MARATHON_RECOMP_HOST_TOOLS_ONLY` builds XenonRecomp, XenosRecomp, file_to_c and u8extract for Windows; the NDK build gets them through `MARATHON_RECOMP_HOST_*` (root `CMakeLists.txt`, `MarathonRecompLib/CMakeLists.txt`). | The code generators must run on the build machine, while everything else is compiled for arm64. |
| No Visual Studio IDE | vcpkg triplets in `android/vcpkg-triplets`: `arm64-android-29-release` for the device and `x64-windows-llvm-host` (clang-cl + lld-link, chainloaded toolchain) for host ports. | Builds with the MSVC Build Tools headers only. |
| FFmpeg with XMAFRAMES | vcpkg `7087dcfb` + the official `ffmpeg-core` patch, static libraries (`Build-FFmpeg-Android.ps1`). | The game's music and voices are XMA streams. |
| SDL3 | Linked statically into `libmain.so`; its Java glue comes unmodified from the `thirdparty/SDL` submodule. | Window, input, audio and app lifecycle on Android. |
| Vulkan driver loading | `thirdparty/libadrenotools` (vendored, BSD-2) and `thirdparty/miniz` for driver packages. | Lets the game use Mesa Turnip instead of the vendor driver. |
| XenonRecomp option switches | `Marathon.toml.in` takes the 8 optimization flags from `MARATHON_RECOMP_PPC_*` CMake options (all OFF by default; the Android scripts enable group 1). | Upstream keeps them off; group 1 was verified on device (see section 5). |

## 2. App layer (`MarathonRecomp/os/android/`)

- **`storage_android`** — the game root is asked from the launcher over JNI (`MarathonActivity.getGameStoragePath()`), defaulting to `Android/data/<package>/files/MarathonRecomp`; config and saves live in `<root>/.config/MarathonRecomp`. Android has no meaningful home directory for an app.
- **`logger_android`** — logcat plus `log.txt` next to the game folder (the previous run is kept as `log_prev.txt`), stderr redirected into the log (Plume and Vulkan errors), a header with device, SoC and kernel. Fatal signals append a short report (signal, fault address, PC/LR resolved to library+offset), then the normal system tombstone runs. Each long-lived thread gets its own alternate signal stack, so a stack overflow is still reported. Testers often have no `adb`, so the file is copied over USB/MTP.
- **Utility logging off** (`os/logger.h`) — the game logs every file request twice and every stub call. On Android each line cost a logcat write and a file write; thousands of them per scene transition. These lines are now skipped before formatting; `log_utility.enabled` turns them back on for debugging.
- **`process`, `version`, `user`, `media`, `thread`** — Android implementations of the small OS interfaces.

## 3. Rendering (`MarathonRecomp/gpu/video.cpp`, `thirdparty/plume`)

- **RGBA back buffer** — Android swap chains only offer RGBA formats; the desktop default is BGRA.
- **Surface lifecycle** — Android replaces the `ANativeWindow` when the app goes to the background. The swap chain compares the window every frame and recreates the Vulkan surface (`VulkanSwapChain::recreateSurface`), with a 2-second acquire timeout instead of waiting forever on a dead window.
- **Frame-rate vote** — `ANativeWindow_setFrameRate` with the FPS option, so high-refresh panels do not hold the game at the panel default.
- **Swap-chain pre-rotation** — phones have portrait panels. With an identity swap chain SurfaceFlinger rotated every landscape frame with GPU composition, on the same GPU as the game. The swap chain now uses `preTransform = currentTransform`, and the final gamma pass writes the image already rotated (`gamma_correction_ps.hlsl`). The display then scans it out directly. `prerotation.disabled` restores the old path.
- **Resolution scale** — the scene render targets are scaled, while the back buffer and the HUD keep the full screen size: the game's own "Rasterize" pass stretches the scene. Upstream scaled everything, which cropped the image or enlarged the HUD on phones. Verified at 60 % and 125 %.
- **Landscape only** — SDL replaced the manifest's `sensorLandscape` with "any orientation" for the resizable window; `SDL_HINT_ORIENTATIONS = "LandscapeLeft LandscapeRight"` fixes that.
- **GPUs without BC textures (Mali, PowerVR; v0.1.5, ported from UnleashedRecomp-Android 1.5.0)** — the game's textures are BC (DXT) compressed, which stock Mali/PowerVR drivers cannot sample. When the device reports no BC support, `LoadTexture` decodes the blocks with [bcdec](https://github.com/iOrange/bcdec) and re-encodes them as ETC2/EAC with [etcpak](https://github.com/wolfpld/etcpak) at load time (same memory size, except transparent BC1); without ETC2 the textures are decoded to RGBA. BC textures the game creates itself are created as RGBA8. The texture/sampler descriptor sets are clamped to the device limits. `bc_fallback.enabled` forces this path on Adreno for testing (with the word `decode` inside: RGBA instead of ETC2). **Not tested on a Mali device yet.**
- **Vulkan 1.2 feature path in Plume (v0.1.5)** — on 1.2+ drivers descriptor indexing, scalar block layout, buffer device address and mirror-clamp sampling are enabled through `VkPhysicalDeviceVulkan12Features`, because drivers such as stock Mali do not have to list the old extension names. `MIRROR_ONCE` falls back to `MIRRORED_REPEAT` when unsupported, device functions are loaded with `volkLoadDevice`, and a GPU without scalar block layout or buffer device address now fails with a clear message in `log.txt`. Turnip (Adreno) also takes this path; checked on Adreno 725.

## 4. Vulkan drivers (`os/android/vulkan_driver_android.cpp`, `android/turnip-drivers`)

- Four bundled Mesa Turnip builds (see `android/turnip-drivers/README.md`). **Auto** picks by device: Snapdragon 8 Elite (SM8750, Adreno 830) → the Mesa 26.3 build with the gralloc UBWC fix; other Adreno → the bundled default build; non-Adreno GPUs → the system driver; a previously imported driver stays selected.
- Users can import a driver (`.so` or an AdrenoTools/ExynosTools `.zip`) from the launcher or by dropping it into `driver_import/`.
- **Boot recovery** — a marker is written before a custom driver is loaded and removed only after the device and a usable swap chain exist. If the game dies in between, the next launch uses the system driver.
- The bundled drivers are copied to internal storage once per APK install (a stamp with the APK's library path and the sizes), not compared byte by byte on every launch.
- Ported from UnleashedRecomp-Android 0.6.1 and moved to SDL3.

## 5. Performance

Measured on a POCO F5 (SM7475) in the HUB, with `perf_capture.enabled` and Perfetto traces:

- **Worker threads and little cores.** The main thread waited ~6.4 ms per frame for the game's FrustumCulling, UpdateResource and FrameSynchronizer workers, which the scheduler ran on the little Cortex-A510 cores at ~1.1 GHz. Pinning them to the bigger cores (`os/android/thread_hints_android.cpp`) took the HUB from **46–50 to 60 FPS** without any system boost: the load on the big cluster also raised its frequency, so the main thread ran at ~2.46 GHz instead of ~1.5 GHz.
  - The decision comes from `cpu_capacity`: a core is little when it is below 50 % of the strongest one; pinning is enabled only if little cores exist and at least 3 other cores remain. On the Snapdragon 8 Elite (no little cores) it stays off. The mask is re-applied after returning from the background. `thread_affinity.disabled` turns it off; `cpus=` in `thread_affinity.enabled` forces a set.
  - Per-thread `uclamp_min` was tried first; the tested firmware rejects it for apps (EPERM). It remains as an opt-in (`thread_uclamp.enabled`).
- **XenonRecomp group 1** (CTR, XER, CR and the reservation as locals): machine code −13.5 % (41.97 → 36.29 MB `.text`). No FPS change was measurable on its own at the time (that state was GPU-bound), but it reduces CPU work per frame. Upstream's reason for keeping the options off was looked into: they are the defaults recommended for bring-up; mid-asm hooks and the fork's new instructions are compatible with local registers.
- **Pre-rotation** — removes SurfaceFlinger's GPU composition (see section 3).
- **Remaining limits on the POCO F5:** in the HUB the GPU needs ~15.5 of the 16.7 ms frame budget at 60 % scale, and scene loads cause short drops.

## 6. Input

- **On-screen touch controls** (`ui/touch_controls.*`, ported from UnleashedRecomp-Android) with a layout editor started from the launcher, and an `HID::Touch` peripheral attached with the keyboard and mouse of user 1. Touches do not generate mouse events (`SDL_HINT_TOUCH_MOUSE_EVENTS = 0`). Physical gamepad input hides the overlay in Auto mode.
- Not adapted yet: a D-pad in menus and a single Skip button in cutscenes (the hooks exist, the Sonic 2006 call sites are not wired).

## 7. Launcher and Android app (`android-apk/`)

- **`MarathonLauncherActivity`** — game status (game ready, DLC count), install from ZIP or folder through the Storage Access Framework, storage choice and migration, Vulkan driver and render mode, touch options, skip intro, logs, the **Info** window. The theme is drawn in code (`LauncherStyle.java`) in the style of the 2006 menus. Strings: English (default) and Russian.
- **`MarathonActivity`** — the SDL activity that starts `libmain.so`.
- **`MarathonDocumentsProvider`** — exposes the game folder, the saves and the transfer folder to the system Files app (Android 11+ hides `Android/data` from file managers).
- Separate application ID `io.github.sansnope.marathonrecomp.android`, so it installs next to other ports.

## 8. Switches (files in the game folder)

| File | Effect |
|---|---|
| `perf_capture.enabled` | `[perf]` timing line in `log.txt` every 2 seconds |
| `log_utility.enabled` | Utility log lines (file loads, stub calls) |
| `prerotation.disabled` | swap chain without pre-rotation |
| `thread_affinity.disabled` | no worker affinity |
| `thread_affinity.enabled` | `cpus=4-7`, `workers=…` overrides |
| `thread_uclamp.enabled` | per-thread uclamp (often rejected by firmware) |
| `bc_fallback.enabled` | test the BC→ETC2 texture fallback on a GPU with BC support (`decode` inside: BC→RGBA) |

The game folder is `Android/data/io.github.sansnope.marathonrecomp.android/files/MarathonRecomp` unless another storage was chosen in the launcher.
