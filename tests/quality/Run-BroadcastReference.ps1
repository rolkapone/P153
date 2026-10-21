#Requires -Version 7.0
<#
.SYNOPSIS
  Mesure des sorties NVIDIA Broadcast 2.3 (jeu B) contre la verite terrain 4K. Les fichiers gardent leur nom horodate
  d'origine (<ID>_AAAA-MM-JJ_HH_MM_SS.mp4) : correspondance par prefixe. Voir docs/qa/broadcast.md.
  Si la sortie n'est pas 3840 de large (cas TOS : Broadcast n'a pas agrandi), elle est mesuree telle quelle apres
  un Lanczos a la resolution de la verite terrain (PreFilter), et le rapport le signale (defaut de conformite).
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$FfmpegDir,
    [string]$ClipsDir = (Join-Path $PSScriptRoot '..\clips'),
    [Parameter(Mandatory)][string]$OutDir,
    [string[]]$Clips = @('BBB-FAST', 'BBB-PAN', 'TOS-FACE', 'TOS-DARK', 'SYN-TEXT')
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'QaCommon.ps1')
function Get-Prop($o, $n) { $p = $o.PSObject.Properties[$n]; if ($p) { $p.Value } else { 'absent' } }
$table = Get-ClipTable
New-Item -ItemType Directory -Path $OutDir -Force | Out-Null
$tools = Resolve-FfmpegTools -FfmpegDir $FfmpegDir -NeedVmaf
$mq = Join-Path $PSScriptRoot 'Measure-Quality.ps1'
$rows = [System.Collections.Generic.List[object]]::new()
$conf = [System.Collections.Generic.List[object]]::new()
foreach ($id in $Clips) {
    $clip = $table.Clips | Where-Object { $_.Id -eq $id }
    $file = Get-ChildItem (Join-Path $ClipsDir 'broadcast') -Filter "${id}_*.mp4" | Sort-Object Name | Select-Object -Last 1
    if (-not $file) { Write-Warning "$id : sortie Broadcast absente"; continue }
    $refFile = Join-Path $ClipsDir "ref4k\$id.mkv"
    $ref = Get-VideoInfo $tools $refFile
    $d = Get-VideoInfo $tools $file.FullName
    $conf.Add([ordered]@{ clip = $id; file = $file.Name; codec = $d.codec_name; profile = $d.profile; pix_fmt = $d.pix_fmt; width = [int]$d.width; height = [int]$d.height
            fps = $d.r_frame_rate; frames = $d.frames; duration = $d.duration; color_space = (Get-Prop $d "color_space"); color_range = (Get-Prop $d "color_range")
            expected = "$($ref.width)x$($ref.height)" })
    $upscaled = ([int]$d.width -eq 3840)
    $pre = if ($upscaled) { '' } else { "scale=3840:$($ref.height):flags=lanczos" }
    $plan = switch ($clip.Mode) {
        'Interp60' { @('Kept', 'Generated') }
        'Align24'  { @('Aligned48') }
        default    { @('All') }
    }
    foreach ($m in $plan) {
        $out = Join-Path $OutDir "quality_${id}_Broadcast_${m}_B.json"
        $a = @('-NoProfile', '-File', $mq, '-FfmpegDir', $FfmpegDir, '-Reference', $refFile, '-Distorted', $file.FullName, '-Label', 'Broadcast', '-ClipId', $id, '-Mode', $m, '-Set', 'B', '-OutJson', $out, '-Neg')
        if ($pre) { $a += @('-PreFilter', $pre) }
        & pwsh @a
        if ($LASTEXITCODE -ne 0) { Write-Warning "$id $m : code $LASTEXITCODE" }
        if (Test-Path $out) { $rows.Add((Get-Content $out -Raw | ConvertFrom-Json)) }
    }
}
Write-JsonFile -Object @{ schema = 'enhance-qa-reference/1'; date = (Get-Date -Format 'yyyy-MM-dd'); conformity = $conf; results = $rows } -Path (Join-Path $OutDir 'reference-broadcast.json')
