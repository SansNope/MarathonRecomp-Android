# Prepares everything the Android build needs besides the toolchain and your game files:
#   1. git submodules (as pinned by this repository);
#   2. the Android patches for thirdparty/plume (android/patches/plume) and tools/XenonRecomp
#      (android/patches/xenonrecomp);
#   3. zlib 1.3.2 source for the Windows host tools (out/deps/zlib-1.3.2);
#   4. a vcpkg checkout with the FFmpeg XMAFRAMES patch (out/deps/vcpkg-ffmpeg), the same way
#      sonicnext-dev/ffmpeg-core builds its desktop prebuilts.
# Downloads go to out/downloads. Safe to re-run: finished steps are detected and skipped.
# Never deletes anything.
param(
    [switch]$SkipSubmodules,
    [switch]$SkipPlume,
    # Only for testing: an ffmpeg-core ffmpeg.patch outside this checkout.
    [string]$FfmpegCorePatch = ''
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$deps = Join-Path $repo 'out\deps'
$downloads = Join-Path $repo 'out\downloads'
New-Item -ItemType Directory -Force $deps, $downloads | Out-Null

# Windows PowerShell turns native stderr lines into errors; only the exit code decides failure.
function Invoke-Native([string]$exe, [string[]]$arguments) {
    $ErrorActionPreference = 'Continue'
    & $exe @arguments 2>&1 | ForEach-Object { if ($_ -is [System.Management.Automation.ErrorRecord]) { $_.Exception.Message } else { "$_" } } | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "$exe $($arguments -join ' ') failed with exit code $LASTEXITCODE" }
}

function Test-Native([string]$exe, [string[]]$arguments) {
    $ErrorActionPreference = 'Continue'
    & $exe @arguments 2>&1 | Out-Null
    return $LASTEXITCODE -eq 0
}

# 1. Submodules.
if (-not $SkipSubmodules) {
    Write-Output '== git submodules'
    Invoke-Native git @('-C', $repo, 'submodule', 'update', '--init', '--recursive')
}

# 2. Plume: swap-chain pre-rotation, Android surface recreation, custom Vulkan loader hook.
if (-not $SkipPlume) {
    Write-Output '== thirdparty/plume Android patch'
    $plume = Join-Path $repo 'thirdparty\plume'
    $patch = Join-Path $repo 'android\patches\plume\0001-marathon-android.patch'
    if (Test-Native git @('-C', $plume, 'apply', '--reverse', '--check', $patch)) {
        Write-Output 'already applied'
    } else {
        Invoke-Native git @('-C', $plume, 'apply', '--whitespace=nowarn', $patch)
        Write-Output 'applied'
    }

    # XenonRecomp: the merge_rounding_overrides option (MARATHON_RECOMP_PPC_MERGE_ROUNDING).
    # Build-HostTools.ps1 has to be run again after this patch is applied for the first time.
    Write-Output '== tools/XenonRecomp Android patch'
    $xenon = Join-Path $repo 'tools\XenonRecomp'
    $patch = Join-Path $repo 'android\patches\xenonrecomp\0001-merge-rounding-overrides.patch'
    if (Test-Native git @('-C', $xenon, 'apply', '--reverse', '--check', $patch)) {
        Write-Output 'already applied'
    } else {
        Invoke-Native git @('-C', $xenon, 'apply', '--whitespace=nowarn', $patch)
        Write-Output 'applied'
    }
}

# 3. zlib for u8extract (the host tools are built without vcpkg).
Write-Output '== zlib 1.3.2'
$zlibDir = Join-Path $deps 'zlib-1.3.2'
if (Test-Path -LiteralPath (Join-Path $zlibDir 'zlib.h')) {
    Write-Output 'already present'
} else {
    $zlibArchive = Join-Path $downloads 'zlib-v1.3.2.tar.gz'
    # Same SHA-512 as thirdparty/vcpkg/ports/zlib/portfile.cmake.
    $zlibSha512 = '16fea4df307a68cf0035858abe2fd550250618a97590e202037acd18a666f57afc10f8836cbbd472d54a0e76539d0e558cb26f059d53de52ff90634bbf4f47d4'
    if (-not (Test-Path -LiteralPath $zlibArchive)) {
        Invoke-WebRequest -UseBasicParsing -Uri 'https://github.com/madler/zlib/archive/v1.3.2.tar.gz' -OutFile $zlibArchive
    }
    if ((Get-FileHash -LiteralPath $zlibArchive -Algorithm SHA512).Hash.ToLower() -ne $zlibSha512) {
        throw "SHA-512 mismatch for $zlibArchive; delete it and run again."
    }
    Invoke-Native tar @('-xzf', $zlibArchive, '-C', $deps)
    Write-Output "extracted to $zlibDir"
}

# 4. vcpkg + FFmpeg XMAFRAMES (Xbox 360 XMA audio) for Android.
Write-Output '== vcpkg for FFmpeg (7087dcfb + ffmpeg-core patch)'
$vcpkgCommit = '7087dcfba66c8da4acb50b09ea680b4e9050a4e9'
$vcpkgDir = Join-Path $deps 'vcpkg-ffmpeg'
if (-not (Test-Path -LiteralPath (Join-Path $vcpkgDir '.git'))) {
    New-Item -ItemType Directory -Force $vcpkgDir | Out-Null
    Invoke-Native git @('-C', $vcpkgDir, 'init', '-q')
    Invoke-Native git @('-C', $vcpkgDir, 'remote', 'add', 'origin', 'https://github.com/microsoft/vcpkg.git')
    Invoke-Native git @('-C', $vcpkgDir, 'fetch', '--depth', '1', 'origin', $vcpkgCommit)
    Invoke-Native git @('-C', $vcpkgDir, '-c', 'advice.detachedHead=false', 'checkout', '--detach', 'FETCH_HEAD')
}
if ((git -C $vcpkgDir rev-parse HEAD) -ne $vcpkgCommit) { throw "$vcpkgDir is not at $vcpkgCommit" }

$nestedPatch = Join-Path $vcpkgDir 'ports\ffmpeg\0008-xmaframes.patch'
if (-not (Test-Path -LiteralPath $nestedPatch)) {
    if (-not $FfmpegCorePatch) { $FfmpegCorePatch = Join-Path $repo 'thirdparty\ffmpeg-core\ffmpeg.patch' }
    if (-not (Test-Path -LiteralPath $FfmpegCorePatch)) { throw "Missing $FfmpegCorePatch (run without -SkipSubmodules)" }
    Invoke-Native git @('-C', $vcpkgDir, 'apply', '--ignore-space-change', '--ignore-whitespace', $FfmpegCorePatch)
}

# A Windows checkout with core.autocrlf turns the patch-inside-the-patch into CRLF, which the
# FFmpeg build then fails to apply. Normalise it to LF and check it against the known copy.
$nestedText = [System.IO.File]::ReadAllText($nestedPatch)
if ($nestedText.Contains("`r")) {
    [System.IO.File]::WriteAllText($nestedPatch, $nestedText.Replace("`r`n", "`n").Replace("`r", "`n"), (New-Object System.Text.UTF8Encoding($false)))
}
$expectedNested = '4abad6afbb008a5468050d7c4db9d9d6f0e52ac914974b4a0799aae66b3ff5f3'
if ((Get-FileHash -LiteralPath $nestedPatch -Algorithm SHA256).Hash.ToLower() -ne $expectedNested) {
    throw "Unexpected $nestedPatch (SHA-256 should be $expectedNested)"
}
Write-Output 'vcpkg ready (XMAFRAMES patch verified)'
Write-Output 'Dependencies prepared.'
