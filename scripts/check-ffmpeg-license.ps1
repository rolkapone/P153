<#
.SYNOPSIS
  Refuse un build FFmpeg GPL ou non libre (ADR-025).
.DESCRIPTION
  Lance ffmpeg.exe -version et -buildconf (sans reseau) et echoue si la configuration contient
  --enable-gpl, --enable-nonfree, libx264, libx265, libfdk, etc. (libvmaf, BSD-2-Clause-Patent, est compatible LGPL).
.EXAMPLE
  scripts\check-ffmpeg-license.ps1 -Root third_party\ffmpeg\lgpl-shared
#>
[CmdletBinding()]
param([string]$Root = (Join-Path (Split-Path $PSScriptRoot -Parent) 'third_party\ffmpeg\lgpl-shared'))
$ErrorActionPreference = 'Stop'

$exe = Join-Path $Root 'bin\ffmpeg.exe'
if (-not (Test-Path $exe)) { throw "ffmpeg.exe introuvable : $exe" }

$version = (& $exe -hide_banner -version 2>&1) -join "`n"
if ($LASTEXITCODE -ne 0) { throw "ffmpeg -version a echoue." }
$conf = (& $exe -hide_banner -buildconf 2>&1) -join "`n"

$forbidden = @('--enable-gpl', '--enable-nonfree', '--enable-libx264', '--enable-libx265',
    '--enable-libfdk-aac', '--enable-libxvid', '--enable-openssl',
    '--enable-libsmbclient', '--enable-librtmp')
$all = $version + "`n" + $conf
$bad = @($forbidden | Where-Object { $all -match [regex]::Escape($_) })
$lic = (& $exe -hide_banner -L 2>&1) -join ' '
if ($lic -notmatch 'Lesser General Public License') { $bad += 'licence annoncee non LGPL' }
if ($lic -match '(?i)nonfree') { $bad += 'nonfree' }

if ($bad.Count -gt 0) {
    Write-Host "REFUSE : build FFmpeg non LGPL ($($bad -join ', '))." -ForegroundColor Red
    exit 1
}
$first = ($version -split "`n")[0]
Write-Host "OK : $first (aucune option GPL/nonfree)." -ForegroundColor Green
