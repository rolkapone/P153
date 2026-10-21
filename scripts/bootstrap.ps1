<#
.SYNOPSIS
  Installe vcpkg (au commit epingle) et prepare les dependances d'Enhance (ADR-023/024).
  PHASE B : necessite un acces reseau et l'accord du PO. Ne lance rien sans -Run.
.DESCRIPTION
  1. git clone https://github.com/microsoft/vcpkg dans -VcpkgRoot, checkout du commit -Baseline
     (a defaut : "builtin-baseline" de vcpkg.json ; refuse le marqueur TODO-VCPKG-BASELINE-SHA)
  2. bootstrap-vcpkg.bat -disableMetrics
  3. variables d'environnement utilisateur : VCPKG_ROOT, VCPKG_DEFAULT_BINARY_CACHE, VCPKG_DISABLE_METRICS
  4. (-Install) configure le preset dev : vcpkg installe le manifeste (Qt compile ~1 h 20 la 1re fois)
.EXAMPLE
  scripts\bootstrap.ps1 -Baseline <sha-40-hex> -Run -Install
#>
[CmdletBinding()]
param(
    [string]$VcpkgRoot = 'C:\Tools\vcpkg',
    [string]$Baseline = '',
    [string]$BinaryCache = 'F:\vcpkg-cache',
    [string]$BuildTrees = 'C:\vb',
    [switch]$Install,
    [switch]$Run
)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot\common.ps1"

$manifestPath = Join-Path $script:RepoRoot 'vcpkg.json'
$manifest = Get-Content $manifestPath -Raw | ConvertFrom-Json
if (-not $Baseline) { $Baseline = $manifest.'builtin-baseline' }
if ($Baseline -notmatch '^[0-9a-f]{40}$') {
    throw "Baseline vcpkg invalide ('$Baseline'). Fournir -Baseline <sha de 40 caracteres hexadecimaux> (a epingler aussi dans vcpkg.json)."
}

Write-Host "vcpkg : $VcpkgRoot @ $Baseline"
Write-Host "Cache binaire : $BinaryCache ; buildtrees : $BuildTrees"
if (-not $Run) {
    Write-Host 'Simulation (aucune action). Relancer avec -Run pour cloner, compiler vcpkg et installer.' -ForegroundColor Yellow
    return
}

if (-not (Test-Path (Join-Path $VcpkgRoot '.git'))) {
    New-Item -ItemType Directory -Force -Path (Split-Path $VcpkgRoot) | Out-Null
    & git clone https://github.com/microsoft/vcpkg $VcpkgRoot
    if ($LASTEXITCODE -ne 0) { throw 'git clone vcpkg echoue.' }
}
& git -C $VcpkgRoot checkout $Baseline
if ($LASTEXITCODE -ne 0) { throw "git checkout $Baseline echoue." }
& (Join-Path $VcpkgRoot 'bootstrap-vcpkg.bat') -disableMetrics
if ($LASTEXITCODE -ne 0) { throw 'bootstrap-vcpkg echoue.' }

New-Item -ItemType Directory -Force -Path $BinaryCache | Out-Null
[Environment]::SetEnvironmentVariable('VCPKG_ROOT', $VcpkgRoot, 'User')
[Environment]::SetEnvironmentVariable('VCPKG_DEFAULT_BINARY_CACHE', $BinaryCache, 'User')
[Environment]::SetEnvironmentVariable('VCPKG_DISABLE_METRICS', '1', 'User')
$env:VCPKG_ROOT = $VcpkgRoot
$env:VCPKG_DEFAULT_BINARY_CACHE = $BinaryCache
$env:VCPKG_DISABLE_METRICS = '1'
Write-Host 'vcpkg pret. Ouvrir un nouveau terminal pour voir les variables utilisateur.' -ForegroundColor Green

if ($Install) {
    $tools = Enter-EnhVsEnv
    Push-Location $script:RepoRoot
    try {
        & $tools.CMake --preset dev "-DVCPKG_INSTALL_OPTIONS=--x-buildtrees-root=$BuildTrees"
        if ($LASTEXITCODE -ne 0) { throw 'Configure (et installation vcpkg) echoue.' }
    } finally { Pop-Location }
}
