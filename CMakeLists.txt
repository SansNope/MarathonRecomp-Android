cmake_minimum_required (VERSION 3.20)

option(MARATHON_RECOMP_HOST_TOOLS_ONLY "Only build the host code-generation tools (XenonRecomp, XenosRecomp, file_to_c, u8extract), skipping the game executable and its platform-specific dependencies. Used to produce host binaries for cross-compiling to another target platform." OFF)

# Optional zlib source tree for u8extract in a MARATHON_RECOMP_HOST_TOOLS_ONLY pass,
# so the host tools can be built without vcpkg (which needs a full Visual Studio on Windows).
set(MARATHON_RECOMP_HOST_ZLIB_SOURCE "" CACHE PATH "Path to a zlib source tree to build statically for the host tools")

if(NOT MARATHON_RECOMP_HOST_TOOLS_ONLY AND NOT DEFINED ENV{VCPKG_ROOT})
    message(FATAL_ERROR "VCPKG_ROOT is not defined!")
endif()

# When cross-compiling (e.g. to Android), these point at prebuilt host executables
# produced by a MARATHON_RECOMP_HOST_TOOLS_ONLY pass, since the in-tree tool targets
# would otherwise be built for the (unrunnable, on the host) target architecture.
set(MARATHON_RECOMP_HOST_XENON_RECOMP "" CACHE FILEPATH "Path to a prebuilt host XenonRecomp executable")
set(MARATHON_RECOMP_HOST_XENOS_RECOMP "" CACHE FILEPATH "Path to a prebuilt host XenosRecomp executable")
set(MARATHON_RECOMP_HOST_FILE_TO_C "" CACHE FILEPATH "Path to a prebuilt host file_to_c executable")
set(MARATHON_RECOMP_HOST_U8EXTRACT "" CACHE FILEPATH "Path to a prebuilt host u8extract executable")
set(MARATHON_RECOMP_HOST_DXC "" CACHE FILEPATH "Path to a host dxc executable, defaults to the bundled one for the build host when cross-compiling")

set(MARATHON_RECOMP_THIRDPARTY_ROOT ${CMAKE_SOURCE_DIR}/thirdparty)
set(MARATHON_RECOMP_TOOLS_ROOT ${CMAKE_SOURCE_DIR}/tools)
set(CMAKE_CXX_STANDARD 20)
set(BUILD_SHARED_LIBS OFF)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project("MarathonRecomp-ALL")

if (APPLE)
    enable_language(OBJC OBJCXX)
endif()

if (CMAKE_OSX_ARCHITECTURES)
    set(MARATHON_RECOMP_ARCHITECTURE ${CMAKE_OSX_ARCHITECTURES})
elseif(CMAKE_SYSTEM_PROCESSOR)
    set(MARATHON_RECOMP_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
else()
    set(MARATHON_RECOMP_ARCHITECTURE ${CMAKE_HOST_SYSTEM_PROCESSOR})
endif()
string(TOLOWER "${MARATHON_RECOMP_ARCHITECTURE}" MARATHON_RECOMP_ARCHITECTURE)
message(STATUS "Detected architecture: ${MARATHON_RECOMP_ARCHITECTURE}")

if (MARATHON_RECOMP_ARCHITECTURE STREQUAL "x86_64" OR MARATHON_RECOMP_ARCHITECTURE STREQUAL "amd64")
    # Target Sandy Bridge for all projects
    add_compile_options(
        -march=sandybridge
    )
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Normally only defined by Visual Studio, added for consistency
    add_compile_definitions(_DEBUG)
endif()

if (MARATHON_RECOMP_HOST_XENON_RECOMP AND MARATHON_RECOMP_HOST_XENOS_RECOMP AND MARATHON_RECOMP_HOST_FILE_TO_C AND MARATHON_RECOMP_HOST_U8EXTRACT)
    set(MARATHON_RECOMP_USE_PREBUILT_HOST_TOOLS ON)
else()
    set(MARATHON_RECOMP_USE_PREBUILT_HOST_TOOLS OFF)
endif()

if (CMAKE_CROSSCOMPILING AND NOT MARATHON_RECOMP_HOST_TOOLS_ONLY AND NOT MARATHON_RECOMP_USE_PREBUILT_HOST_TOOLS)
    message(FATAL_ERROR "Cross-compiling requires prebuilt host tools: set MARATHON_RECOMP_HOST_XENON_RECOMP, MARATHON_RECOMP_HOST_XENOS_RECOMP, MARATHON_RECOMP_HOST_FILE_TO_C and MARATHON_RECOMP_HOST_U8EXTRACT.")
endif()

if (MARATHON_RECOMP_HOST_TOOLS_ONLY AND MARATHON_RECOMP_HOST_ZLIB_SOURCE)
    file(GLOB MARATHON_RECOMP_HOST_ZLIB_SOURCES "${MARATHON_RECOMP_HOST_ZLIB_SOURCE}/*.c")
    add_library(host_zlib STATIC ${MARATHON_RECOMP_HOST_ZLIB_SOURCES})
    target_include_directories(host_zlib PUBLIC "${MARATHON_RECOMP_HOST_ZLIB_SOURCE}")
    if (WIN32)
        target_compile_definitions(host_zlib PRIVATE _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE)
    endif()
    add_library(ZLIB::ZLIB ALIAS host_zlib)

    # Satisfies find_package(ZLIB) in tools/u8extract; FindZLIB keeps the existing ZLIB::ZLIB target.
    set(ZLIB_INCLUDE_DIR "${MARATHON_RECOMP_HOST_ZLIB_SOURCE}")
    set(ZLIB_LIBRARY host_zlib)
endif()

if (NOT MARATHON_RECOMP_HOST_TOOLS_ONLY)
    add_subdirectory(${MARATHON_RECOMP_THIRDPARTY_ROOT})
endif()

# tools/ also provides shared thirdparty libraries (fmt, zstd, xxHash, tomlplusplus, XenonUtils)
# that MarathonRecomp links against directly, so it's always added even when the recompiler
# executables themselves are swapped out for prebuilt host binaries.
add_subdirectory(${MARATHON_RECOMP_TOOLS_ROOT})
target_compile_definitions(XenosRecomp PRIVATE MARATHON_RECOMP)

if (MARATHON_RECOMP_HOST_DXC)
    set(DIRECTX_DXC_TOOL "${MARATHON_RECOMP_HOST_DXC}" CACHE INTERNAL "")
elseif (CMAKE_CROSSCOMPILING)
    # dxc-bin picks the binary for the target platform, which can't run on the build host.
    set(MARATHON_RECOMP_DXC_BIN_ROOT "${MARATHON_RECOMP_TOOLS_ROOT}/XenosRecomp/thirdparty/dxc-bin")
    if (CMAKE_HOST_WIN32)
        set(DIRECTX_DXC_TOOL "${MARATHON_RECOMP_DXC_BIN_ROOT}/bin/x64/dxc.exe" CACHE INTERNAL "")
    else()
        set(DIRECTX_DXC_TOOL "LD_LIBRARY_PATH=${MARATHON_RECOMP_DXC_BIN_ROOT}/lib/x64" "${MARATHON_RECOMP_DXC_BIN_ROOT}/bin/x64/dxc-linux" CACHE INTERNAL "")
    endif()
endif()

if (NOT MARATHON_RECOMP_HOST_TOOLS_ONLY)
    # Include sub-projects.
    add_subdirectory("MarathonRecompLib")
    add_subdirectory("MarathonRecomp")
endif()
