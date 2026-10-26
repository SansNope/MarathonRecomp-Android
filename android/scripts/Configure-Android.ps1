# Configures (and with -Build, builds) the arm64-v8a libmain.so with the Android NDK.
# Needs the host tools (Build-HostTools.ps1), FFmpeg (Build-FFmpeg-Android.ps1) and your game
# files in MarathonRecompLib/private (see docs/android/BUILDING.md).
# Never deletes anything: an existing build directory is reconfigured in place.
param(
    [string]$BuildDir = '',
    [string]$BuildType = 'Release',
    # XenonRecomp optimizations. The default is the tested set (group 1, keeps CTR/XER/CR and the
    # reservation in locals). Pass -CMakeArgs @() for the conservative upstream set (all OFF).
    # Changing the set regenerates all recompiled code; use a separate -BuildDir per set.
    [string[]]$CMakeArgs = @(
        '-DMARATHON_RECOMP_PPC_CTR_AS_LOCAL=ON',
        '-DMARATHON_RECOMP_PPC_XER_AS_LOCAL=ON',
        '-DMARATHON_RECOMP_PPC_RESERVED_AS_LOCAL=ON',
        '-DMARATHON_RECOMP_PPC_CR_AS_LOCAL=ON'
    ),
    [switch]$Build
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
. (Join-Path $PSScriptRoot 'Enter-Environment.ps1') | Out-Null

if (-not $BuildDir) { $BuildDir = Join-Path $repo 'out\build\android-arm64' }
$triplet = 'arm64-android-29-release'
$overlay = Join-Path $repo 'android\vcpkg-triplets'
$ffmpeg = Join-Path $repo "out\ffmpeg-android\installed\$triplet"
$vcpkgWork = Join-Path $repo 'out\vcpkg-android'
$downloads = Join-Path $repo 'out\downloads\vcpkg'
$logDir = Join-Path $repo 'out\logs'
$logSuffix = if ((Split-Path -Leaf $BuildDir) -eq 'android-arm64') { '' } else { '-' + (Split-Path -Leaf $BuildDir) }
$log = Join-Path $logDir "android-configure$logSuffix.log"
New-Item -ItemType Directory -Force $logDir, $vcpkgWork, $downloads | Out-Null

$hostToolsReport = Join-Path $logDir 'host-tools.json'
if (-not (Test-Path -LiteralPath $hostToolsReport)) { throw "Missing $hostToolsReport; run Build-HostTools.ps1" }
$hostTools = Get-Content -Raw -LiteralPath $hostToolsReport | ConvertFrom-Json
foreach ($tool in 'XenonRecomp', 'XenosRecomp', 'file_to_c', 'u8extract') {
    if (-not (Test-Path -LiteralPath $hostTools.tools.$tool.path)) { throw "Host tool $tool missing; run Build-HostTools.ps1" }
}
if (-not (Test-Path -LiteralPath (Join-Path $ffmpeg 'lib\libavcodec.a'))) { throw "FFmpeg for Android not found in $ffmpeg; run Build-FFmpeg-Android.ps1" }
foreach ($gameFile in 'default.xex', 'shader.arc', 'shader_lt.arc') {
    if (-not (Test-Path -LiteralPath (Join-Path $repo "MarathonRecompLib\private\$gameFile"))) {
        throw "Missing MarathonRecompLib\private\$gameFile - copy it from your own copy of the game (docs/android/BUILDING.md)"
    }
}

function ConvertTo-CMakePath([string]$path) { $path -replace '\\', '/' }

function Invoke-Logged([string]$exe, [string[]]$arguments) {
    "`n> $exe $($arguments -join ' ')" | Out-File -Append -Encoding utf8 $log
    # Windows PowerShell turns native stderr lines into errors; only the exit code decides failure.
    $ErrorActionPreference = 'Continue'
    & $exe @arguments 2>&1 | ForEach-Object { if ($_ -is [System.Management.Automation.ErrorRecord]) { $_.Exception.Message } else { "$_" } } | Tee-Object -Append -FilePath $log | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "$exe failed with exit code $LASTEXITCODE (see $log)" }
}

$vcpkgInstallOptions = @(
    "--x-buildtrees-root=$(ConvertTo-CMakePath "$vcpkgWork\buildtrees")",
    "--x-packages-root=$(ConvertTo-CMakePath "$vcpkgWork\packages")",
    "--downloads-root=$(ConvertTo-CMakePath $downloads)",
    '--disable-metrics'
) -join ';'

"Android configure started $(Get-Date -Format o)" | Out-File -Encoding utf8 $log
Invoke-Logged 'cmake' (@(
    '-S', $repo, '-B', $BuildDir, '-G', 'Ninja',
    "-DCMAKE_BUILD_TYPE=$BuildType",
    "-DCMAKE_TOOLCHAIN_FILE=$(ConvertTo-CMakePath "$repo\thirdparty\vcpkg\scripts\buildsystems\vcpkg.cmake")",
    "-DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=$(ConvertTo-CMakePath "$env:ANDROID_NDK_HOME\build\cmake\android.toolchain.cmake")",
    "-DVCPKG_TARGET_TRIPLET=$triplet",
    # vcpkg host ports use clang-cl via a chainloaded toolchain (no Visual Studio IDE needed).
    '-DVCPKG_HOST_TRIPLET=x64-windows-llvm-host',
    "-DVCPKG_OVERLAY_TRIPLETS=$(ConvertTo-CMakePath $overlay)",
    "-DVCPKG_INSTALL_OPTIONS=$vcpkgInstallOptions",
    '-DANDROID_ABI=arm64-v8a',
    '-DANDROID_PLATFORM=android-29',
    "-DMARATHON_RECOMP_HOST_XENON_RECOMP=$(ConvertTo-CMakePath $hostTools.tools.XenonRecomp.path)",
    "-DMARATHON_RECOMP_HOST_XENOS_RECOMP=$(ConvertTo-CMakePath $hostTools.tools.XenosRecomp.path)",
    "-DMARATHON_RECOMP_HOST_FILE_TO_C=$(ConvertTo-CMakePath $hostTools.tools.file_to_c.path)",
    "-DMARATHON_RECOMP_HOST_U8EXTRACT=$(ConvertTo-CMakePath $hostTools.tools.u8extract.path)",
    "-DMARATHON_RECOMP_ANDROID_FFMPEG_ROOT=$(ConvertTo-CMakePath $ffmpeg)"
) + $CMakeArgs)

if ($Build) {
    # libadrenotools hook libraries are loaded at runtime from the APK's native lib dir.
    Invoke-Logged 'cmake' @('--build', $BuildDir, '--target', 'MarathonRecomp', 'main_hook', 'file_redirect_hook', 'gsl_alloc_hook', 'hook_impl')
}
Write-Output "Android build directory: $BuildDir"
