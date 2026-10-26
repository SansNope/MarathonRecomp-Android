# Dot-source this file: . .\android\scripts\Enter-Environment.ps1
# Selects the toolchain from android/local.paths.json for this PowerShell process only.
# Copy android/local.paths.example.json to android/local.paths.json and edit the paths first.
$ErrorActionPreference = 'Stop'
$marathonRepo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$marathonPathsFile = Join-Path $marathonRepo 'android\local.paths.json'
if (-not (Test-Path -LiteralPath $marathonPathsFile)) {
    throw "Missing $marathonPathsFile - copy android\local.paths.example.json and set your tool paths."
}
$marathonPaths = Get-Content -Raw -LiteralPath $marathonPathsFile | ConvertFrom-Json
foreach ($marathonItem in @('android_sdk','android_ndk','jdk17','cmake_bin','ninja_bin','host_llvm_bin','msvc_root','windows_sdk_root')) {
    if (-not (Test-Path -LiteralPath $marathonPaths.$marathonItem -PathType Container)) {
        throw "Missing configured tool path: $marathonItem = $($marathonPaths.$marathonItem)"
    }
}
$env:JAVA_HOME = $marathonPaths.jdk17
$env:ANDROID_HOME = $marathonPaths.android_sdk
$env:ANDROID_SDK_ROOT = $marathonPaths.android_sdk
$env:ANDROID_NDK_HOME = $marathonPaths.android_ndk
$env:ANDROID_NDK_ROOT = $marathonPaths.android_ndk
$marathonToolBins = @(
    (Join-Path $marathonPaths.jdk17 'bin'),
    $marathonPaths.cmake_bin,
    $marathonPaths.ninja_bin,
    $marathonPaths.host_llvm_bin,
    (Join-Path $marathonPaths.msvc_root 'bin\Hostx64\x64'),
    (Join-Path $marathonPaths.windows_sdk_root "bin\$($marathonPaths.windows_sdk_version)\x64")
)
$env:PATH = ($marathonToolBins -join ';') + ';' + $env:PATH
# clang-cl builds the Windows host tools against the MSVC headers/libraries and the Windows SDK.
$env:INCLUDE = @(
    (Join-Path $marathonPaths.msvc_root 'include'),
    (Join-Path $marathonPaths.windows_sdk_root "Include\$($marathonPaths.windows_sdk_version)\ucrt"),
    (Join-Path $marathonPaths.windows_sdk_root "Include\$($marathonPaths.windows_sdk_version)\shared"),
    (Join-Path $marathonPaths.windows_sdk_root "Include\$($marathonPaths.windows_sdk_version)\um")
) -join ';'
$env:LIB = @(
    (Join-Path $marathonPaths.msvc_root 'lib\x64'),
    (Join-Path $marathonPaths.windows_sdk_root "Lib\$($marathonPaths.windows_sdk_version)\ucrt\x64"),
    (Join-Path $marathonPaths.windows_sdk_root "Lib\$($marathonPaths.windows_sdk_version)\um\x64")
) -join ';'
$env:VCPKG_ROOT = Join-Path $marathonRepo 'thirdparty\vcpkg'
# Used by the x64-windows-llvm-host vcpkg triplet (vcpkg sanitizes PATH for port builds).
$env:MARATHON_HOST_LLVM_BIN = $marathonPaths.host_llvm_bin
$env:GRADLE_USER_HOME = Join-Path $marathonRepo 'out\gradle-cache'
$env:VCPKG_DEFAULT_BINARY_CACHE = Join-Path $marathonRepo 'out\vcpkg-cache'
New-Item -ItemType Directory -Force $env:VCPKG_DEFAULT_BINARY_CACHE | Out-Null
Write-Output "Marathon Android environment selected for this process. Repository: $marathonRepo"
