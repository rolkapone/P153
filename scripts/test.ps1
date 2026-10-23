<#
.SYNOPSIS
  Build puis tests unitaires (label "unit", sans GPU) et tests de l interface (label "ui", si Qt).
.EXAMPLE
  scripts\test.ps1                  # preset dev-nogpu
  scripts\test.ps1 -Preset ci
  scripts\test.ps1 -Coverage        # + couverture OpenCppCoverage (outil local) : build\coverage\index.html
#>
[CmdletBinding()]
param(
    [ValidateSet('dev-nogpu', 'dev', 'ci')]
    [string]$Preset = 'dev-nogpu',
    [switch]$NoBuild,
    [switch]$Coverage
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

# Tests de l'interface (label ui, ADR-028) : presents seulement si Qt est compile (presets dev*).
& $tools.Ctest --test-dir (Join-Path $script:RepoRoot "build\$Preset") -L ui --output-on-failure --no-tests=ignore
if ($LASTEXITCODE -ne 0) { throw 'Tests de l interface (label ui) en echec.' }
Write-Host 'Tests de l interface OK' -ForegroundColor Green

if ($Coverage) {
    $occ = Get-Command OpenCppCoverage -ErrorAction SilentlyContinue
    if (-not $occ) { $c = "$env:ProgramFiles\OpenCppCoverage\OpenCppCoverage.exe"; if (Test-Path $c) { $occ = Get-Item $c } }
    if (-not $occ) { throw 'OpenCppCoverage introuvable (winget install OpenCppCoverage.OpenCppCoverage).' }
    $exe = Get-ChildItem (Join-Path $script:RepoRoot "build\$Preset\tests") -Recurse -Filter enh_unit_tests.exe | Select-Object -First 1
    if (-not $exe) { throw 'enh_unit_tests.exe introuvable.' }
    $out = Join-Path $script:RepoRoot 'build\coverage'
    $occExe = if ($occ -is [IO.FileInfo]) { $occ.FullName } else { $occ.Source }
    & $occExe --quiet --sources (Join-Path $script:RepoRoot 'src') --export_type "html:$out" -- $exe.FullName
    if ($LASTEXITCODE -ne 0) { throw 'Couverture en echec.' }
    Write-Host "Rapport de couverture : build\coverage\index.html" -ForegroundColor Green
}