<#
.SYNOPSIS
  cppcheck local, memes version et options que le job `sast` de .github/workflows/ci.yml (ADR-029/030).
  A lancer avant tout merge. Code de sortie 0 = aucun constat ; 1 = constats ; 2 = cppcheck absent ou mauvaise version.
.EXAMPLE
  scripts\sast.ps1
#>
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot\common.ps1"

$expected = '2.22.0'   # a garder identique a ci.yml
$cpp = "$env:ProgramFiles\Cppcheck\cppcheck.exe"
if (-not (Test-Path $cpp)) { $c = Get-Command cppcheck -ErrorAction SilentlyContinue; if ($c) { $cpp = $c.Source } }
if (-not (Test-Path $cpp)) {
    Write-Host "cppcheck $expected introuvable. Installation epinglee : MSI de la release officielle (voir l'etape 'cppcheck' de ci.yml pour l'URL et le SHA-256)." -ForegroundColor Red
    exit 2
}
$ver = (& $cpp --version) -replace '^Cppcheck\s+', ''
if ($ver.Trim() -ne $expected) { Write-Host "cppcheck $ver trouve, $expected attendu (version de la CI)." -ForegroundColor Red; exit 2 }
Push-Location $script:RepoRoot
try {
    & $cpp --platform=win64 --std=c++20 --enable=warning,performance,portability --suppress=unknownMacro --inline-suppr --error-exitcode=1 -I src src
    if ($LASTEXITCODE -ne 0) { Write-Host 'cppcheck : constats.' -ForegroundColor Red; exit 1 }
    Write-Host 'cppcheck : 0 constat.' -ForegroundColor Green
} finally { Pop-Location }
