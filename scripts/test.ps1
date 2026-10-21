<#
.SYNOPSIS
  Build puis tests unitaires (label "unit", sans GPU).
.EXAMPLE
  scripts\test.ps1                  # preset dev-nogpu
  scripts\test.ps1 -Preset ci
#>
[CmdletBinding()]
param(
    [ValidateSet('dev-nogpu', 'ci')]
    [string]$Preset = 'dev-nogpu',
    [switch]$NoBuild
)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot\common.ps1"

if (-not $NoBuild) {
    & "$PSScriptRoot\build.ps1" -Preset $Preset
}
$tools = Enter-EnhVsEnv
& $tools.Ctest --test-dir (Join-Path $script:RepoRoot "build\$Preset") -L unit --output-on-failure --no-tests=error
if ($LASTEXITCODE -ne 0) { throw "Tests unitaires en echec (ou aucun test : GoogleTest absent, voir scripts\bootstrap.ps1)." }
Write-Host 'Tests unitaires OK' -ForegroundColor Green
