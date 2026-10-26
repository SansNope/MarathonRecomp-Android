# Host triplet for building vcpkg host dependencies (pkgconf, meson helpers, ...) on a
# Windows machine without a Visual Studio installation: clang-cl/lld-link and the MSVC/SDK
# headers and libraries come from INCLUDE/LIB/PATH (see scripts/Enter-Environment.ps1).
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_BUILD_TYPE release)

# A chainloaded toolchain disables vcvars, so vcpkg does not look for Visual Studio.
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/x64-windows-llvm-host.toolchain.cmake")
set(VCPKG_LOAD_VCVARS_ENV OFF)
# vcpkg sanitizes PATH, so the toolchain locates LLVM through MARATHON_HOST_LLVM_BIN.
set(VCPKG_ENV_PASSTHROUGH_UNTRACKED "INCLUDE;LIB;LIBPATH;MARATHON_HOST_LLVM_BIN")

# Meson-based ports look up clang-cl/lld-link by name, so put LLVM back on the port's PATH.
if (DEFINED ENV{MARATHON_HOST_LLVM_BIN})
    set(ENV{PATH} "$ENV{MARATHON_HOST_LLVM_BIN};$ENV{PATH}")
endif()

set(VCPKG_POLICY_SKIP_DUMPBIN_CHECKS enabled)
