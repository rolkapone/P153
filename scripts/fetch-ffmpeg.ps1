<#
.SYNOPSIS
  Telecharge le FFmpeg LGPL de developpement (BtbN win64-lgpl-shared) au tag epingle (ADR-025).
  Necessite le reseau ; a lancer a la main, jamais en CI. Accord du PO obtenu le 2026-10-21.
.DESCRIPTION
  Lit deps/ffmpeg.lock.json (tag, asset, SHA-256), telecharge, VERIFIE le SHA-256 (echec si
  different), extrait dans third_party/ffmpeg/lgpl-shared/ puis lance check-ffmpeg-license.ps1.
#>
[CmdletBinding()]
param(
    [string]$Dest = (Join-Path (Split-Path $PSScriptRoot -Parent) 'third_party\ffmpeg\lgpl-shared'),
    [switch]$Force
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$lock = Get-Content (Join-Path $repo 'deps\ffmpeg.lock.json') -Raw | ConvertFrom-Json

if ((Test-Path (Join-Path $Dest 'bin\ffmpeg.exe')) -and -not $Force) {
    Write-Host "Deja present : $Dest (utiliser -Force pour retelecharger)."
    & (Join-Path $PSScriptRoot 'check-ffmpeg-license.ps1') -Root $Dest
    exit $LASTEXITCODE
}
if ($lock.url -notmatch '^https://github\.com/BtbN/FFmpeg-Builds/releases/download/') {
    throw "URL inattendue dans le verrou : $($lock.url)"
}

$tmp = Join-Path ([IO.Path]::GetTempPath()) ("enh-ffmpeg-" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $tmp | Out-Null
try {
    $zip = Join-Path $tmp $lock.asset
    Write-Host "Telechargement : $($lock.asset)"
    $ProgressPreference = 'SilentlyContinue'
    Invoke-WebRequest -Uri $lock.url -OutFile $zip -UseBasicParsing
    $actual = (Get-FileHash -Algorithm SHA256 $zip).Hash.ToLowerInvariant()
    if ($actual -ne $lock.sha256.ToLowerInvariant()) {
        throw "SHA-256 different. Attendu $($lock.sha256), obtenu $actual. Archive rejetee."
    }
    Write-Host "SHA-256 verifie : $actual"
    $ex = Join-Path $tmp 'x'
    Expand-Archive -LiteralPath $zip -DestinationPath $ex
    $top = @(Get-ChildItem $ex -Directory)
    if ($top.Count -ne 1) { throw 'Arborescence d archive inattendue.' }
    if (Test-Path $Dest) { Remove-Item -Recurse -Force $Dest }
    New-Item -ItemType Directory -Force -Path (Split-Path $Dest) | Out-Null
    Move-Item $top[0].FullName $Dest
} finally {
    Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
}
& (Join-Path $PSScriptRoot 'check-ffmpeg-license.ps1') -Root $Dest
exit $LASTEXITCODE
