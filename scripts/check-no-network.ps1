<#
.SYNOPSIS
  Squelette de la preuve "zero reseau" cote build (ADR-027). La liste blanche
  (scripts\network-allowlist.json) sera definie par secdevops.
.DESCRIPTION
  Analyse un dossier deploye (par defaut build\ci) :
   1. interdit les DLL reseau connues (Qt6Network, winhttp, wininet, libcurl, cpprest) ;
   2. liste les imports reseau (ws2_32, mswsock, winhttp, wininet) de chaque exe/dll via dumpbin
      et les compare a la liste blanche.
  Code de sortie 0 = OK, 1 = violation, 2 = outil manquant.
#>
[CmdletBinding()]
param([string]$Path = 'build\ci')
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot\common.ps1"

$dir = Join-Path $script:RepoRoot $Path
if (-not (Test-Path $dir)) { Write-Host "Dossier introuvable : $Path"; exit 2 }

$bannedNames = '^(Qt6?Network.*|winhttp|wininet|libcurl.*|cpprest.*)\.dll$'
$networkImports = 'ws2_32.dll', 'mswsock.dll', 'winhttp.dll', 'wininet.dll', 'wsock32.dll'
# TODO(secdevops) : format de la liste blanche { "<binaire>": { "<dll>": "<justification>" } }.
$allowFile = Join-Path $PSScriptRoot 'network-allowlist.json'
$allow = $null
if (Test-Path $allowFile) { $allow = Get-Content $allowFile -Raw | ConvertFrom-Json }

$tools = Enter-EnhVsEnv
$dumpbin = Get-Command dumpbin -ErrorAction SilentlyContinue
if (-not $dumpbin) { Write-Host 'dumpbin introuvable (environnement MSVC).'; exit 2 }

$violations = @()
$bins = Get-ChildItem $dir -Recurse -Include *.exe, *.dll -File
foreach ($b in $bins) {
    if ($b.Name -match $bannedNames) { $violations += "DLL reseau interdite : $($b.FullName.Substring($script:RepoRoot.Length + 1))" }
    $deps = & dumpbin /nologo /dependents $b.FullName | ForEach-Object { $_.Trim().ToLowerInvariant() }
    foreach ($n in $networkImports) {
        if ($deps -contains $n) {
            $entry = if ($allow) { $allow.PSObject.Properties[$b.Name] } else { $null }
            $ok = ($null -ne $entry) -and ($null -ne $entry.Value.PSObject.Properties[$n])
            if (-not $ok) { $violations += "Import reseau non autorise : $($b.Name) -> $n" }
        }
    }
}
if ($violations.Count -gt 0) {
    $violations | ForEach-Object { Write-Host "  - $_" -ForegroundColor Red }
    exit 1
}
Write-Host ("check-no-network OK ({0} binaire(s) analyses)." -f $bins.Count) -ForegroundColor Green
exit 0
