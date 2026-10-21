<#
.SYNOPSIS
  Genere localement le corpus de decodage d'ENH-008 (matrice "noyau" + cas limites) dans
  tests\clips\gen\ (ignore par git). Aucun telechargement : source = tests\clips\in720\BBB-FAST.mp4.
.DESCRIPTION
  Encodeurs de la build FFmpeg LGPL du projet (third_party\ffmpeg\lgpl-shared) : h264_nvenc,
  hevc_nvenc, av1_nvenc (RTX requise), libvpx-vp9, mpeg2video, prores_ks. Un encodeur absent est
  signale "non genere" (le test correspondant echoue : jamais de PASS par defaut).
  ASCII seulement (PowerShell 5.1) : les noms Unicode sont construits par codes de caracteres.
.EXAMPLE
  powershell -NoProfile -ExecutionPolicy Bypass -File tests\gpu\corpus\make-corpus.ps1
#>
[CmdletBinding()]
param(
    [string]$FfmpegBin = '',
    [double]$Seconds = 4,
    [switch]$Force
)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
if (-not $FfmpegBin) {
    $FfmpegBin = if ($env:ENH_FFMPEG_ROOT) { Join-Path $env:ENH_FFMPEG_ROOT 'bin' } else { Join-Path $repo 'third_party\ffmpeg\lgpl-shared\bin' }
}
$ff = Join-Path $FfmpegBin 'ffmpeg.exe'
if (-not (Test-Path $ff)) { throw "ffmpeg.exe introuvable : $ff" }
$src = Join-Path $repo 'tests\clips\in720\BBB-FAST.mp4'
if (-not (Test-Path $src)) { throw "Clip source absent : $src (scripts\fetch-clips.ps1)" }
$out = Join-Path $repo 'tests\clips\gen'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$t = [string]::Format([cultureinfo]::InvariantCulture, '{0}', $Seconds)

# nom | arguments d'encodage
$matrix = @(
    @('core_mp4_h264.mp4',     '-c:v h264_nvenc -profile:v high -bf 2 -b:v 6M'),
    @('core_mp4_hevc10.mp4',   '-c:v hevc_nvenc -profile:v main10 -pix_fmt p010le -b:v 6M -tag:v hvc1'),
    @('core_mkv_av1_10.mkv',   '-c:v av1_nvenc -pix_fmt p010le -b:v 6M'),
    @('core_webm_vp9.webm',    '-c:v libvpx-vp9 -deadline realtime -cpu-used 8 -b:v 4M -pix_fmt yuv420p'),
    @('core_ts_mpeg2.ts',      '-c:v mpeg2video -q:v 3 -bf 2'),
    @('core_avi_h264.avi',     '-c:v h264_nvenc -profile:v high -bf 0 -b:v 6M'),
    @('core_mov_h264.mov',     '-c:v h264_nvenc -profile:v high -bf 2 -b:v 6M'),
    @('core_mov_prores_hq.mov','-c:v prores_ks -profile:v 3 -pix_fmt yuv422p10le')
)
$failed = 0
foreach ($m in $matrix) {
    $dst = Join-Path $out $m[0]
    if ((Test-Path $dst) -and -not $Force) { Write-Host "[deja la] $($m[0])"; continue }
    $args = @('-v', 'error', '-y', '-i', $src, '-t', $t, '-an', '-sn') + ($m[1] -split ' ') + @($dst)
    & $ff @args
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path $dst)) {
        Write-Host "[NON GENERE] $($m[0]) (encodeur absent ou erreur)" -ForegroundColor Red
        $failed++
    } else {
        Write-Host "[ok] $($m[0])" -ForegroundColor Green
    }
}

# Cas limites (protocole QA 8.1, 8.2, 8.4, 8.7)
$bytes = [IO.File]::ReadAllBytes($src)
[IO.File]::WriteAllBytes((Join-Path $out 'edge_zero.mp4'), [byte[]]@())
$cut = New-Object byte[] ([int]($bytes.Length * 0.6))
[Array]::Copy($bytes, $cut, $cut.Length)
[IO.File]::WriteAllBytes((Join-Path $out 'edge_truncated.mp4'), $cut)
$bad = [byte[]]$bytes.Clone()
$rng = New-Object System.Random 8
for ($i = [int]($bad.Length * 0.4); $i -lt [int]($bad.Length * 0.4) + 65536; $i++) { $bad[$i] = [byte]$rng.Next(256) }
[IO.File]::WriteAllBytes((Join-Path $out 'edge_corrupt.mp4'), $bad)
# "te<e accent> <3 kanji> & espace.mkv"
$uni = 't' + [char]0x00E9 + ' ' + [char]0x65E5 + [char]0x672C + [char]0x8A9E + ' & espace.mkv'
Copy-Item (Join-Path $out 'core_mkv_av1_10.mkv') (Join-Path $out $uni) -Force -ErrorAction SilentlyContinue
# Chemin > 260 caracteres (prefixe \\?\ : PowerShell 5.1 / .NET Framework)
$long = Join-Path $out 'long'
for ($i = 0; $i -lt 5; $i++) { $long = Join-Path $long ([string]([char](0x61 + $i)) * 55) }
$longFile = Join-Path $long ('vid' + [char]0x00E9 + 'o longue.mp4')
[IO.Directory]::CreateDirectory('\\?\' + $long) | Out-Null
[IO.File]::Copy($src, '\\?\' + $longFile, $true)
Write-Host ("Cas limites : edge_zero / edge_truncated / edge_corrupt / nom Unicode / chemin de {0} caracteres" -f $longFile.Length)
if ($failed -gt 0) { Write-Host "$failed fichier(s) non genere(s)." -ForegroundColor Red; exit 1 }
Write-Host "Corpus pret : tests\clips\gen" -ForegroundColor Green
