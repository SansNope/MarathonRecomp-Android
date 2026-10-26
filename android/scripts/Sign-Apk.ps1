# Signs a release APK with external private keys; never installs or publishes it.
param(
    [Parameter(Mandatory)][string]$UnsignedApk,
    [Parameter(Mandatory)][string]$OutputApk,
    [string]$SigningConfig = $env:MARATHON_RELEASE_SIGNING_CONFIG
)
$ErrorActionPreference = 'Stop'
if (-not $SigningConfig -or -not (Test-Path -LiteralPath $SigningConfig -PathType Leaf)) {
    throw 'Set MARATHON_RELEASE_SIGNING_CONFIG to a private signing JSON file (see BUILDING.md).'
}
if (-not $env:JAVA_HOME -or -not $env:ANDROID_HOME) { throw 'Select the Android environment before signing.' }
$javaTask = Join-Path $env:JAVA_HOME 'bin\java.exe'
$buildToolsTask = Get-ChildItem -LiteralPath (Join-Path $env:ANDROID_HOME 'build-tools') -Directory |
    Where-Object { $_.Name -match '^\d+\.\d+\.\d+$' -and (Test-Path -LiteralPath (Join-Path $_.FullName 'lib\apksigner.jar')) } |
    Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
if (-not $buildToolsTask) { throw 'Android SDK Build Tools with apksigner are required.' }
$apksignerTask = Join-Path $buildToolsTask.FullName 'lib\apksigner.jar'
$zipalignTask = Join-Path $buildToolsTask.FullName 'zipalign.exe'
$signingTask = Get-Content -Raw -LiteralPath $SigningConfig | ConvertFrom-Json
if (@($signingTask.signers).Count -lt 1) { throw 'The private signing JSON must list at least one signer.' }
$unsignedTask = (Resolve-Path -LiteralPath $UnsignedApk).Path
$outputTask = [IO.Path]::GetFullPath($OutputApk)
if ($unsignedTask -eq $outputTask) { throw 'The signed APK must have a different path from the input APK.' }
New-Item -ItemType Directory -Force (Split-Path -Parent $outputTask) | Out-Null
$workTask = Join-Path (Split-Path -Parent $outputTask) ('signing-work\' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force $workTask | Out-Null
$alignedTask = Join-Path $workTask 'aligned-unsigned.apk'
& $zipalignTask -f 4 $unsignedTask $alignedTask
if ($LASTEXITCODE -ne 0) { throw 'zipalign failed.' }
$argumentsTask = [Collections.Generic.List[string]]::new()
foreach ($argTask in @('-jar', $apksignerTask, 'sign', '--v1-signing-enabled', 'false', '--v2-signing-enabled', 'true', '--v3-signing-enabled', 'true', '--v4-signing-enabled', 'false')) {
    $argumentsTask.Add($argTask)
}
if ($signingTask.lineageFile) {
    if (-not (Test-Path -LiteralPath $signingTask.lineageFile -PathType Leaf)) { throw 'Signing lineage file is missing.' }
    $argumentsTask.Add('--lineage'); $argumentsTask.Add([string]$signingTask.lineageFile)
    $argumentsTask.Add('--rotation-min-sdk-version'); $argumentsTask.Add('28')
}
$firstSignerTask = $true
foreach ($signerTask in @($signingTask.signers)) {
    foreach ($fieldTask in @('keystore', 'storePasswordFile', 'keyPasswordFile')) {
        if (-not (Test-Path -LiteralPath $signerTask.$fieldTask -PathType Leaf)) { throw "Missing private signer file: $fieldTask" }
    }
    if (-not $signerTask.alias) { throw 'Signing key alias is required.' }
    if (-not $firstSignerTask) { $argumentsTask.Add('--next-signer') }
    $firstSignerTask = $false
    foreach ($argTask in @('--ks', [string]$signerTask.keystore, '--ks-key-alias', [string]$signerTask.alias,
        '--ks-pass', "file:$($signerTask.storePasswordFile)", '--key-pass', "file:$($signerTask.keyPasswordFile)")) {
        $argumentsTask.Add($argTask)
    }
}
$argumentsTask.Add('--out'); $argumentsTask.Add($outputTask); $argumentsTask.Add($alignedTask)
& $javaTask @argumentsTask
if ($LASTEXITCODE -ne 0) { throw 'apksigner sign failed.' }
& $javaTask -jar $apksignerTask verify --verbose --print-certs --min-sdk-version 29 $outputTask
if ($LASTEXITCODE -ne 0) { throw 'Release signature verification failed.' }
& $zipalignTask -c 4 $outputTask
if ($LASTEXITCODE -ne 0) { throw 'Signed release alignment verification failed.' }
Write-Output "Signed release APK: $outputTask"
