# Read-only checks before a build: tools and versions, submodules, the Plume patch,
# prepared dependencies and your game files. Builds nothing and changes nothing.
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
. (Join-Path $PSScriptRoot 'Enter-Environment.ps1') | Out-Null
$paths = Get-Content -Raw -LiteralPath (Join-Path $repo 'android\local.paths.json') | ConvertFrom-Json

# Windows PowerShell turns native stderr lines (java -version writes only there) into errors;
# only the exit code decides failure.
function Get-NativeVersion([string]$exe, [string]$argument) {
    $ErrorActionPreference = 'Continue'
    $output = & $exe $argument 2>&1 | ForEach-Object { if ($_ -is [System.Management.Automation.ErrorRecord]) { $_.Exception.Message } else { "$_" } }
    if ($LASTEXITCODE -ne 0) { return $null }
    ($output | Select-Object -First 1 | Out-String).Trim()
}

$checks = New-Object System.Collections.Generic.List[object]
function Add-Check([string]$name, [bool]$ok, [string]$detail) {
    $checks.Add([pscustomobject][ordered]@{ check = $name; ok = $ok; detail = $detail })
}

$tools = [ordered]@{
    git = @((Get-Command git -ErrorAction SilentlyContinue).Source, '--version')
    cmake = @((Join-Path $paths.cmake_bin 'cmake.exe'), '--version')
    ninja = @((Join-Path $paths.ninja_bin 'ninja.exe'), '--version')
    java17 = @((Join-Path $paths.jdk17 'bin\java.exe'), '-version')
    clang_cl = @((Join-Path $paths.host_llvm_bin 'clang-cl.exe'), '--version')
    lld_link = @((Join-Path $paths.host_llvm_bin 'lld-link.exe'), '--version')
    ndk_clang = @((Join-Path $paths.android_ndk 'toolchains\llvm\prebuilt\windows-x86_64\bin\clang.exe'), '--version')
}
foreach ($name in $tools.Keys) {
    $exe, $argument = $tools[$name]
    $exists = $exe -and (Test-Path -LiteralPath $exe -PathType Leaf)
    Add-Check $name $exists $(if ($exists) { Get-NativeVersion $exe $argument } else { "not found: $exe" })
}
$ndkVersion = Split-Path -Leaf $paths.android_ndk
Add-Check 'ndk 29.0.14206865' ($ndkVersion -eq '29.0.14206865') "configured: $ndkVersion (other NDK versions are untested)"
$platform = Join-Path $paths.android_sdk 'platforms\android-35\android.jar'
Add-Check 'android-35 platform' (Test-Path -LiteralPath $platform) $platform
Add-Check 'msvc include' (Test-Path -LiteralPath (Join-Path $paths.msvc_root 'include\vcruntime.h')) $paths.msvc_root
Add-Check 'windows sdk' (Test-Path -LiteralPath (Join-Path $paths.windows_sdk_root "Include\$($paths.windows_sdk_version)\ucrt")) "$($paths.windows_sdk_root) $($paths.windows_sdk_version)"

foreach ($submodule in 'thirdparty\SDL', 'thirdparty\plume', 'thirdparty\vcpkg', 'thirdparty\ffmpeg-core', 'tools\XenonRecomp', 'tools\XenosRecomp', 'MarathonRecompResources') {
    $full = Join-Path $repo $submodule
    Add-Check "submodule $submodule" ((Get-ChildItem -LiteralPath $full -Force -ErrorAction SilentlyContinue | Measure-Object).Count -gt 0) $full
}

$ErrorActionPreference = 'Continue'
git -C (Join-Path $repo 'thirdparty\plume') apply --reverse --check (Join-Path $repo 'android\patches\plume\0001-marathon-android.patch') 2>&1 | Out-Null
$plumePatched = $LASTEXITCODE -eq 0
$ErrorActionPreference = 'Stop'
Add-Check 'plume android patch applied' $plumePatched 'run Prepare-Dependencies.ps1 if not'
Add-Check 'zlib 1.3.2 (out/deps)' (Test-Path -LiteralPath (Join-Path $repo 'out\deps\zlib-1.3.2\zlib.h')) 'Prepare-Dependencies.ps1'
Add-Check 'vcpkg-ffmpeg (out/deps)' (Test-Path -LiteralPath (Join-Path $repo 'out\deps\vcpkg-ffmpeg\ports\ffmpeg\0008-xmaframes.patch')) 'Prepare-Dependencies.ps1'

foreach ($gameFile in 'default.xex', 'shader.arc', 'shader_lt.arc') {
    $full = Join-Path $repo "MarathonRecompLib\private\$gameFile"
    Add-Check "game file $gameFile" (Test-Path -LiteralPath $full) 'copy from your own copy of the game (docs/android/BUILDING.md)'
}

$checks | Format-Table -AutoSize -Wrap | Out-String -Width 200 | Write-Output
if ($checks | Where-Object { -not $_.ok }) { throw 'Some checks failed; see the table above.' }
Write-Output 'Environment OK.'
