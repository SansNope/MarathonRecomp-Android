cmake_policy(SET CMP0077 NEW)

set(MSDF_ATLAS_BUILD_STANDALONE OFF)
set(MSDF_ATLAS_USE_SKIA OFF)
set(MSDF_ATLAS_NO_ARTERY_FONT ON)
set(MSDFGEN_DISABLE_PNG ON)

set(SDLMIXER_DEPS_SHARED OFF)
set(SDLMIXER_VENDORED ON)
set(SDLMIXER_FLAC OFF)
set(SDLMIXER_MOD OFF)
set(SDLMIXER_MP3 OFF)
set(SDLMIXER_MIDI OFF)
set(SDLMIXER_OPUS OFF)
set(SDLMIXER_VORBIS_STB OFF)
set(SDLMIXER_WAVPACK OFF)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(PLUME_SDL3_VULKAN_ENABLED ON CACHE BOOL "")
endif()

if (WIN32)
    set(PLUME_D3D12_AGILITY_SDK_ENABLED ON CACHE BOOL "")
endif()

if (APPLE AND CMAKE_OSX_ARCHITECTURES)
    set(BASE_ARCHITECTURE "${CMAKE_OSX_ARCHITECTURES}")
elseif (CMAKE_SYSTEM_PROCESSOR)
    set(BASE_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
else()
    set(BASE_ARCHITECTURE "${CMAKE_HOST_SYSTEM_PROCESSOR}")
endif()

# Next, match common architecture strings down to a known common value.
if (BASE_ARCHITECTURE MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
    set(ARCHITECTURE "x86_64")
elseif (BASE_ARCHITECTURE MATCHES "(aarch64)|(AARCH64)|(arm64)|(ARM64)")
    set(ARCHITECTURE "arm64")
else()
    message(FATAL_ERROR "Unsupported CPU architecture: ${BASE_ARCHITECTURE}")
endif()

add_subdirectory("${MARATHON_RECOMP_THIRDPARTY_ROOT}/msdf-atlas-gen")

# The native file picker is not used on Android; game data is imported by the Java launcher.
if (NOT ANDROID)
    add_subdirectory("${MARATHON_RECOMP_THIRDPARTY_ROOT}/nativefiledialog-extended")
endif()

add_subdirectory("${MARATHON_RECOMP_THIRDPARTY_ROOT}/o1heap")
add_subdirectory("${MARATHON_RECOMP_THIRDPARTY_ROOT}/SDL")
add_subdirectory("${MARATHON_RECOMP_THIRDPARTY_ROOT}/SDL_mixer")
add_subdirectory("${MARATHON_RECOMP_THIRDPARTY_ROOT}/plume")

if (ANDROID)
    # Loads a custom Vulkan driver (Mesa Turnip) instead of the vendor driver, see
    # MarathonRecomp/os/android/vulkan_driver_android.cpp. Vendored from UnleashedRecomp-Android.
    # Its hook libraries (main_hook, file_redirect_hook, gsl_alloc_hook, hook_impl) ship in the APK.
    add_subdirectory("${MARATHON_RECOMP_THIRDPARTY_ROOT}/libadrenotools")

    # ffmpeg-core only ships desktop prebuilts (its "linux" build is glibc), so Android links
    # a separately cross-built FFmpeg with the same XMAFRAMES patch (see scripts/Build-FFmpeg-Android.ps1).
    set(MARATHON_RECOMP_ANDROID_FFMPEG_ROOT "" CACHE PATH "Install prefix of FFmpeg built for Android with the XMAFRAMES decoder")
    if (NOT EXISTS "${MARATHON_RECOMP_ANDROID_FFMPEG_ROOT}/include/libavcodec/avcodec.h")
        message(FATAL_ERROR "MARATHON_RECOMP_ANDROID_FFMPEG_ROOT must point at an Android FFmpeg install prefix (got '${MARATHON_RECOMP_ANDROID_FFMPEG_ROOT}').")
    endif()

    add_library(ffmpeg INTERFACE)
    target_include_directories(ffmpeg INTERFACE "${MARATHON_RECOMP_ANDROID_FFMPEG_ROOT}/include")
    target_link_libraries(ffmpeg INTERFACE
        "${MARATHON_RECOMP_ANDROID_FFMPEG_ROOT}/lib/libavformat.a"
        "${MARATHON_RECOMP_ANDROID_FFMPEG_ROOT}/lib/libavcodec.a"
        "${MARATHON_RECOMP_ANDROID_FFMPEG_ROOT}/lib/libswscale.a"
        "${MARATHON_RECOMP_ANDROID_FFMPEG_ROOT}/lib/libavutil.a"
        "${MARATHON_RECOMP_ANDROID_FFMPEG_ROOT}/lib/libavfilter.a"
        "${MARATHON_RECOMP_ANDROID_FFMPEG_ROOT}/lib/libswresample.a")
else()
    add_subdirectory("${MARATHON_RECOMP_THIRDPARTY_ROOT}/ffmpeg-core")
endif()
