# Cross-builds FFmpeg 7.1.1 with the XMAFRAMES decoder (Xbox 360 XMA audio) for Android arm64,
# the same way sonicnext-dev/ffmpeg-core builds its desktop prebuilts: vcpkg at 7087dcfb plus
# the official ffmpeg.patch. Needs Prepare-Dependencies.ps1 first. Writes only under out/.
param(
    [string]$Triplet = 'arm64-android-29-release'
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
. (Join-Path $PSScriptRoot 'Enter-Environment.ps1') | Out-Null

$vcpkgRoot = Join-Path $repo 'out\deps\vcpkg-ffmpeg'
$overlay = Join-Path $repo 'android\vcpkg-triplets'
$work = Join-Path $repo 'out\ffmpeg-android'
$downloads = Join-Path $repo 'out\downloads\vcpkg'
$logDir = Join-Path $repo 'out\logs'
$log = Join-Path $logDir 'ffmpeg-android-build.log'
New-Item -ItemType Directory -Force $work, $downloads, $logDir | Out-Null

$expectedPatch = '4abad6afbb008a5468050d7c4db9d9d6f0e52ac914974b4a0799aae66b3ff5f3'
$patch = Join-Path $vcpkgRoot 'ports\ffmpeg\0008-xmaframes.patch'
if ((git -C $vcpkgRoot rev-parse HEAD) -ne '7087dcfba66c8da4acb50b09ea680b4e9050a4e9') { throw "out/deps/vcpkg-ffmpeg is not at 7087dcfb (run Prepare-Dependencies.ps1)" }
if (-not (Test-Path -LiteralPath $patch) -or (Get-FileHash -LiteralPath $patch -Algorithm SHA256).Hash.ToLower() -ne $expectedPatch) { throw "XMAFRAMES patch missing or not the verified LF copy: $patch" }

# The environment points VCPKG_ROOT at the main vcpkg submodule; this build uses its own checkout.
$env:VCPKG_ROOT = $vcpkgRoot
$vcpkg = Join-Path $vcpkgRoot 'vcpkg.exe'

function Invoke-Logged([string]$exe, [string[]]$arguments) {
    "`n> $exe $($arguments -join ' ')" | Out-File -Append -Encoding utf8 $log
    # Windows PowerShell turns native stderr lines into errors; only the exit code decides failure.
    $ErrorActionPreference = 'Continue'
    & $exe @arguments 2>&1 | ForEach-Object { if ($_ -is [System.Management.Automation.ErrorRecord]) { $_.Exception.Message } else { "$_" } } | Tee-Object -Append -FilePath $log | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "$exe failed with exit code $LASTEXITCODE (see $log)" }
}

"FFmpeg Android build started $(Get-Date -Format o)" | Out-File -Encoding utf8 $log
if (-not (Test-Path -LiteralPath $vcpkg)) {
    Invoke-Logged (Join-Path $vcpkgRoot 'bootstrap-vcpkg.bat') @('-disableMetrics')
}

Push-Location $vcpkgRoot
try {
    Invoke-Logged $vcpkg @(
        'install', "ffmpeg[avcodec,avfilter,avdevice,avformat,swresample,swscale]:$Triplet",
        "--overlay-triplets=$overlay",
        # Host ports are built with clang-cl through a chainloaded toolchain (no Visual Studio IDE needed).
        '--host-triplet=x64-windows-llvm-host',
        "--x-install-root=$work\installed",
        "--x-buildtrees-root=$work\buildtrees",
        "--x-packages-root=$work\packages",
        "--downloads-root=$downloads",
        '--disable-metrics'
    )
}
finally {
    Pop-Location
}

$prefix = Join-Path $work "installed\$Triplet"
$libs = Get-ChildItem -LiteralPath (Join-Path $prefix 'lib') -Filter '*.a'
$report = [ordered]@{
    built_at = (Get-Date -Format o)
    vcpkg_commit = '7087dcfba66c8da4acb50b09ea680b4e9050a4e9'
    xmaframes_patch_sha256 = $expectedPatch
    triplet = $Triplet
    prefix = $prefix
    libs = @($libs | ForEach-Object { [ordered]@{ name = $_.Name; size = $_.Length; sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLower() } })
}
$report | ConvertTo-Json -Depth 4 | Out-File -Encoding utf8 (Join-Path $logDir 'ffmpeg-android.json')
Write-Output "FFmpeg for Android installed at $prefix"
