#Requires -Version 7.0
<#
.SYNOPSIS
  Mesure de reference "plancher" : Lanczos (+ duplication d'images) et Lanczos + minterpolate, contre la verite terrain 4K.
  Ecrit un JSON par mesure dans -OutDir et un resume reference-lanczos.json. Voir docs/qa/protocole.md 3.4.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$FfmpegDir,
    [string]$ClipsDir = (Join-Path $PSScriptRoot '..\clips'),
    [Parameter(Mandatory)][string]$OutDir,
    [string[]]$Clips = @('BBB-FAST', 'BBB-PAN', 'TOS-FACE', 'TOS-DARK', 'SYN-TEXT'),
    [switch]$SkipMinterpolate,
    [switch]$Force                 # sinon, une mesure dont le JSON existe deja est reprise telle quelle
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'QaCommon.ps1')
$table = Get-ClipTable
New-Item -ItemType Directory -Path $OutDir -Force | Out-Null
$tools = Resolve-FfmpegTools -FfmpegDir $FfmpegDir -NeedVmaf
$mq = Join-Path $PSScriptRoot 'Measure-Quality.ps1'
$rows = [System.Collections.Generic.List[object]]::new()
foreach ($id in $Clips) {
    $clip = $table.Clips | Where-Object { $_.Id -eq $id }
    $refFile = Join-Path $ClipsDir "ref4k\$id.mkv"
    $h = (Get-VideoInfo $tools $refFile).height
    $scale = "scale=3840:${h}:flags=lanczos"
    $plan = switch ($clip.Mode) {
        'Interp60' { @(@{ L='Lanczos'; Set='A'; Mode='Kept' }, @{ L='Lanczos'; Set='A'; Mode='Generated' },
                       @{ L='Lanczos'; Set='B'; Mode='Kept' }, @{ L='Lanczos'; Set='B'; Mode='Generated' }) +
                    $(if (-not $SkipMinterpolate) { @(@{ L='Minterpolate'; Set='A'; Mode='Kept' }, @{ L='Minterpolate'; Set='A'; Mode='Generated' }) }) }
        'Align24'  { @(@{ L='Lanczos'; Set='A'; Mode='Aligned24' }, @{ L='Lanczos'; Set='B'; Mode='Aligned24' }) }
        default    { @(@{ L='Lanczos'; Set='A'; Mode='All' }, @{ L='Lanczos'; Set='B'; Mode='All' }) }
    }
    foreach ($p in $plan) {
        $ext = if ($p.Set -eq 'A') { 'mkv' } else { 'mp4' }
        $dist = Join-Path $ClipsDir "in720\$id.$ext"
        $fps = if ($clip.Mode -in 'Interp60', 'Align24') { ',fps=60' } else { '' }
        $pre = if ($p.L -eq 'Minterpolate') { "minterpolate=fps=60:mi_mode=mci:mc_mode=aobmc:me_mode=bidir:vsbmc=1,$scale" } else { "$scale$fps" }
        $out = Join-Path $OutDir "quality_${id}_$($p.L)_$($p.Mode)_$($p.Set).json"
        $neg = @(if ($p.Set -eq 'A' -and $p.L -eq 'Lanczos') { '-Neg' })
        # processus fils : isole les variables (Measure-Quality utilise des noms de variables communs avec ce script)
        if ((Test-Path $out) -and -not $Force) { Write-Host "reprise : $(Split-Path $out -Leaf)"; $rows.Add((Get-Content $out -Raw | ConvertFrom-Json)); continue }
        & pwsh -NoProfile -File $mq -FfmpegDir $FfmpegDir -Reference $refFile -Distorted $dist -PreFilter $pre -Label $p.L -ClipId $id -Mode $p.Mode -Set $p.Set -OutJson $out @neg
        if ($LASTEXITCODE -ne 0) { Write-Warning "$id $($p.L) $($p.Mode) $($p.Set) : code $LASTEXITCODE" }
        if (Test-Path $out) { $rows.Add((Get-Content $out -Raw | ConvertFrom-Json)) }
    }
}
Write-JsonFile -Object @{ schema = 'enhance-qa-reference/1'; date = (Get-Date -Format 'yyyy-MM-dd'); results = $rows } -Path (Join-Path $OutDir 'reference-lanczos.json')


