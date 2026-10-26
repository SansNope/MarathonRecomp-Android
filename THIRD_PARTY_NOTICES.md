# Third-party notices

MarathonRecomp-Android is distributed under the **GNU General Public License v3.0** ([COPYING](COPYING)), like MarathonRecomp. The components below keep their own licenses; their full license texts are in the listed locations (submodules are fetched by `git submodule update --init --recursive`).

## Linked into the APK

| Component | Where | License |
|---|---|---|
| MarathonRecomp | this repository | GPL-3.0 |
| SDL 3 | `thirdparty/SDL` (submodule) | zlib (`LICENSE.txt`) |
| SDL_mixer | `thirdparty/SDL_mixer` (submodule) | zlib (`LICENSE.txt`) |
| Plume | `thirdparty/plume` (submodule) + `android/patches/plume` | MIT (`LICENSE`) |
| FFmpeg 7.1.1 with the XMAFRAMES decoder | built by `Build-FFmpeg-Android.ps1` from vcpkg `7087dcfb` and `thirdparty/ffmpeg-core/ffmpeg.patch` | **LGPL-2.1-or-later** (configured without GPL parts) |
| libadrenotools | `thirdparty/libadrenotools` | BSD-2-Clause (`LICENSE`) |
| miniz | `thirdparty/miniz` | MIT (`LICENSE`) |
| Dear ImGui | `thirdparty/imgui` (submodule) | MIT |
| ImPlot | `thirdparty/implot` (submodule) | MIT |
| nlohmann/json | `thirdparty/json` (submodule) | MIT |
| magic_enum | `thirdparty/magic_enum` (submodule) | MIT |
| moodycamel::ConcurrentQueue | `thirdparty/concurrentqueue` (submodule) | Simplified BSD / Boost (`LICENSE.md`) |
| stb | `thirdparty/stb` (submodule) | MIT or public domain |
| unordered_dense | `thirdparty/unordered_dense` (submodule) | MIT |
| ddspp | `thirdparty/ddspp` (submodule) | MIT |
| msdf-atlas-gen / msdfgen | `thirdparty/msdf-atlas-gen` (submodule) | MIT |
| MarathonRecompResources | `MarathonRecompResources` (submodule) | see the submodule |

FFmpeg is linked statically. The complete source of the application is this repository (GPL-3.0, which LGPL-2.1-or-later code may be combined with), and the scripts in `android/scripts` rebuild FFmpeg and relink the application from source.

## Bundled as assets

| Component | Where | License |
|---|---|---|
| Mesa Turnip Vulkan drivers (4 prebuilt builds) | `android/turnip-drivers` (see its `README.md` for the origin of each build and `SHA256SUMS`) | MIT (Mesa) |
| App icon artwork by **GSNReboot** (GameBanana) | `android-apk/icon-source/`, `android-apk/app/src/main/res/mipmap-*` | Third-party fan artwork, **not** covered by the GPL; credited in the app's Info window. See `android-apk/icon-source/CREDITS.md`. |

## Build tools only (not shipped)

| Component | Where | License |
|---|---|---|
| XenonRecomp | `tools/XenonRecomp` (submodule) | MIT (`LICENSE.md`) |
| XenosRecomp | `tools/XenosRecomp` (submodule) | MIT (`LICENSE.md`) |
| zlib 1.3.2 (host u8extract) | downloaded by `Prepare-Dependencies.ps1` | zlib |
| vcpkg | `thirdparty/vcpkg` (submodule) and `out/deps/vcpkg-ffmpeg` | MIT |

## Game content

*Sonic the Hedgehog* and all related content are property of SEGA. This repository contains **no game files**; you must provide them from your own legally acquired copy of the Xbox 360 game. This project is not affiliated with or endorsed by SEGA or the MarathonRecomp team.
