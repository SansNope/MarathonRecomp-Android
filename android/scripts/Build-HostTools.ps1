# Builds the Windows x64 code-generation tools (XenonRecomp, XenosRecomp, file_to_c, u8extract)
# in a MARATHON_RECOMP_HOST_TOOLS_ONLY pass with clang-cl. The Android build runs them to
# recompile your default.xex and shaders. Needs Prepare-Dependencies.ps1 (zlib) first.
# Never deletes anything: an existing build directory is reconfigured in place.
param(
    [string]$BuildDir = '',
    [switch]$ConfigureOnly
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
. (Join-Path $PSScriptRoot 'Enter-Environment.ps1') | Out-Null

if (-not $BuildDir) { $BuildDir = Join-Path $repo 'out\build\host-tools' }
$zlib = Join-Path $repo 'out\deps\zlib-1.3.2'
$logDir = Join-Path $repo 'out\logs'
$log = Join-Path $logDir 'host-tools-build.log'
New-Item -ItemType Directory -Force $logDir | Out-Null
if (-not (Test-Path -LiteralPath (Join-Path $zlib 'zlib.h'))) { throw "zlib source not found: $zlib (run Prepare-Dependencies.ps1)" }

$tools = @('XenonRecomp', 'XenosRecomp', 'file_to_c', 'u8extract')

function Invoke-Logged([string]$exe, [string[]]$arguments) {
    "`n> $exe $($arguments -join ' ')" | Out-File -Append -Encoding utf8 $log
    # Windows PowerShell turns native stderr lines into errors; only the exit code decides failure.
    $ErrorActionPreference = 'Continue'
    & $exe @arguments 2>&1 | ForEach-Object { if ($_ -is [System.Management.Automation.ErrorRecord]) { $_.Exception.Message -replace '^System\.Management\.Automation\.RemoteException$', '' } else { "$_" } } | Tee-Object -Append -FilePath $log | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "$exe failed with exit code $LASTEXITCODE (see $log)" }
}

"Host tools build started $(Get-Date -Format o)" | Out-File -Encoding utf8 $log
Invoke-Logged 'cmake' @(
    '-S', $repo, '-B', $BuildDir, '-G', 'Ninja',
    '-DCMAKE_BUILD_TYPE=Release',
    '-DCMAKE_C_COMPILER=clang-cl.exe',
    '-DCMAKE_CXX_COMPILER=clang-cl.exe',
    '-DCMAKE_LINKER=lld-link.exe',
    '-DMARATHON_RECOMP_HOST_TOOLS_ONLY=ON',
    # Android only uses the SPIR-V shader cache; DXIL would only add dead data to libmain.so.
    '-DXENOS_RECOMP_DXIL=OFF',
    "-DMARATHON_RECOMP_HOST_ZLIB_SOURCE=$($zlib -replace '\\','/')"
)
if ($ConfigureOnly) { Write-Output "Configured only: $BuildDir"; return }

Invoke-Logged 'cmake' (@('--build', $BuildDir, '--target') + $tools)

$report = [ordered]@{
    built_at = (Get-Date -Format o)
    source_head = (git -C $repo rev-parse HEAD)
    build_dir = $BuildDir
    tools = [ordered]@{}
}
foreach ($tool in $tools) {
    $exe = Get-ChildItem -LiteralPath $BuildDir -Recurse -Filter "$tool.exe" | Select-Object -First 1
    if (-not $exe) { throw "Host tool $tool was not produced" }
    $report.tools[$tool] = [ordered]@{
        path = $exe.FullName
        size = $exe.Length
        sha256 = (Get-FileHash -LiteralPath $exe.FullName -Algorithm SHA256).Hash.ToLower()
    }
}
$reportPath = Join-Path $logDir 'host-tools.json'
$report | ConvertTo-Json -Depth 4 | Out-File -Encoding utf8 $reportPath
Write-Output "Host tools built; report: $reportPath"
