# One-shot build: environment check, host tools, FFmpeg (skipped when already built),
# libmain.so and the APK. Run Prepare-Dependencies.ps1 once before the first build.
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)

& (Join-Path $PSScriptRoot 'Check-Environment.ps1')
& (Join-Path $PSScriptRoot 'Build-HostTools.ps1')
if (-not (Test-Path -LiteralPath (Join-Path $repo 'out\ffmpeg-android\installed\arm64-android-29-release\lib\libavcodec.a'))) {
    & (Join-Path $PSScriptRoot 'Build-FFmpeg-Android.ps1')
}
& (Join-Path $PSScriptRoot 'Configure-Android.ps1') -Build
& (Join-Path $PSScriptRoot 'Build-Apk.ps1')
