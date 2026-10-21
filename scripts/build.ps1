<#
.SYNOPSIS
  Configure et compile Enhance (CMake + Ninja + MSVC v145).
.EXAMPLE
  scripts\build.ps1                      # preset dev
  scripts\build.ps1 -Preset ci -Clean
#>
[CmdletBinding()]
param(
    [ValidateSet('dev', 'dev-nogpu', 'local-gpu', 'release', 'ci', 'ci-ui')]
    [string]$Preset = 'dev',
    [switch]$Clean,
    [switch]$ConfigureOnly
)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot\common.ps1"

$tools = Enter-EnhVsEnv
$env:VCPKG_DISABLE_METRICS = '1'
Push-Location $script:RepoRoot
try {
    if ($Clean) {
        $dir = Join-Path $script:RepoRoot "build\$Preset"
        if (Test-Path $dir) { Remove-Item -Recurse -Force $dir }
    }
    if (-not $env:VCPKG_ROOT) {
        Write-Warning 'VCPKG_ROOT non defini : dependances vcpkg indisponibles (tests/UI ignores). Voir scripts\bootstrap.ps1.'
    }
    & $tools.CMake --preset $Preset
    if ($LASTEXITCODE -ne 0) { throw "Configure echoue (preset $Preset)." }
    if ($ConfigureOnly) { return }
    & $tools.CMake --build --preset $Preset
    if ($LASTEXITCODE -ne 0) { throw "Build echoue (preset $Preset)." }
    Write-Host "Build OK : build\$Preset" -ForegroundColor Green
}
finally {
    Pop-Location
}
