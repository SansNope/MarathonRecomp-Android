# Packages the APK from android-apk/ with the libmain.so built by Configure-Android.ps1 -Build
# and the bundled Turnip drivers (android/turnip-drivers, verified against SHA256SUMS).
# Does not install anything. The debug APK is signed with your local Android debug key;
# the result contains no game data, but do not redistribute APKs built from your game.
param(
    [string]$NativeBuildDir = '',
    [string]$Variant = 'Debug',
    [string]$ArtifactName = ''
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
. (Join-Path $PSScriptRoot 'Enter-Environment.ps1') | Out-Null

if (-not $NativeBuildDir) { $NativeBuildDir = Join-Path $repo 'out\build\android-arm64' }
if (-not $ArtifactName) { $ArtifactName = "MarathonRecomp-Android-$($Variant.ToLower())" }
$apkProject = Join-Path $repo 'android-apk'
$jniLibs = Join-Path $repo 'out\apk-jnilibs'
$artifacts = Join-Path $repo 'out\artifacts'
$logDir = Join-Path $repo 'out\logs'
$log = Join-Path $logDir "apk-build-$ArtifactName.log"
New-Item -ItemType Directory -Force (Join-Path $jniLibs 'arm64-v8a'), $artifacts, $logDir | Out-Null

$libmain = Join-Path $NativeBuildDir 'MarathonRecomp\libmain.so'
if (-not (Test-Path -LiteralPath $libmain)) { throw "libmain.so not found: $libmain (run Configure-Android.ps1 -Build)" }
Copy-Item -LiteralPath $libmain -Destination (Join-Path $jniLibs 'arm64-v8a\libmain.so') -Force

# libadrenotools hook libraries (custom Vulkan driver loading).
foreach ($hook in 'libmain_hook.so', 'libfile_redirect_hook.so', 'libgsl_alloc_hook.so', 'libhook_impl.so') {
    $path = Get-ChildItem -LiteralPath $NativeBuildDir -Recurse -Filter $hook | Select-Object -First 1
    if (-not $path) { throw "adrenotools hook library $hook was not produced" }
    Copy-Item -LiteralPath $path.FullName -Destination (Join-Path $jniLibs "arm64-v8a\$hook") -Force
}

# Bundled Turnip drivers (assets/bundled_driver/*.so), verified against their manifest.
$drivers = Join-Path $repo 'android\turnip-drivers'
foreach ($line in Get-Content -LiteralPath (Join-Path $drivers 'SHA256SUMS')) {
    $hash, $name = $line -split '\s+', 2
    $actual = (Get-FileHash -LiteralPath (Join-Path $drivers "assets\bundled_driver\$name") -Algorithm SHA256).Hash.ToLower()
    if ($actual -ne $hash) { throw "Driver checksum mismatch: $name" }
}

"APK build started $(Get-Date -Format o)" | Out-File -Encoding utf8 $log
$ErrorActionPreference = 'Continue'
& (Join-Path $apkProject 'gradlew.bat') -p $apkProject --no-daemon "assemble$Variant" "-PmarathonJniLibs=$($jniLibs -replace '\\','/')" "-PmarathonAssets=$("$drivers\assets" -replace '\\','/')" 2>&1 |
    ForEach-Object { if ($_ -is [System.Management.Automation.ErrorRecord]) { $_.Exception.Message } else { "$_" } } |
    Tee-Object -Append -FilePath $log | Out-Host
if ($LASTEXITCODE -ne 0) { throw "Gradle failed with exit code $LASTEXITCODE (see $log)" }
$ErrorActionPreference = 'Stop'

$apk = Get-ChildItem -LiteralPath (Join-Path $apkProject "app\build\outputs\apk\$($Variant.ToLower())") -Filter '*.apk' | Select-Object -First 1
if (-not $apk) { throw "No APK produced" }
$destination = Join-Path $artifacts "$ArtifactName.apk"
Copy-Item -LiteralPath $apk.FullName -Destination $destination -Force
$hash = (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash.ToLower()
"$hash  $(Split-Path -Leaf $destination)" | Out-File -Encoding ascii "$destination.sha256"
Write-Output "APK: $destination"
Write-Output "SHA-256: $hash"
