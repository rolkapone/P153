#Requires -Version 7.0
<#
.SYNOPSIS
  Mesure VMAF (modele 4K), SSIM (Y) et PSNR (Y) d'un flux "distordu" contre la verite terrain 4K. Ecrit un JSON.
.DESCRIPTION
  Voir docs/qa/protocole.md. FFmpeg GPL de mesure avec libvmaf, HORS DEPOT : chemin passe en parametre.
  Distorted = sortie d'Enhance / Broadcast / Python (fichier 4K), ou entree 720p + -PreFilter pour la ligne Lanczos
  (aucun fichier intermediaire : scale et cadence se font a la volee).
  Mode : All | Kept | Generated | Aligned24  (images comparees, voir protocole 3.2).
  NON EXECUTE a la redaction : a valider au premier passage reel.
.EXAMPLE
  # Ligne Lanczos (plancher) :
  .\Measure-Quality.ps1 -FfmpegDir C:\Tools\ffmpeg -Reference ..\clips\ref4k\BBB-FAST.mkv `
     -Distorted ..\clips\in720\BBB-FAST.mkv -PreFilter 'scale=3840:2160:flags=lanczos,fps=60' `
     -Label Lanczos -ClipId BBB-FAST -Mode Generated -Set A -OutJson ..\..\docs\qa\resultats\run1\quality_BBB-FAST_none_Lanczos_Generated_A.json
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$FfmpegDir,
    [Parameter(Mandatory)][string]$Reference,
    [Parameter(Mandatory)][string]$Distorted,
    [Parameter(Mandatory)][string]$OutJson,
    [Parameter(Mandatory)][string]$ClipId,
    [ValidateSet('Enhance', 'Lanczos', 'Minterpolate', 'Broadcast', 'Python')][string]$Label = 'Enhance',
    [string]$Preset = 'none',                       # quality | balanced | fast | none
    [ValidateSet('A', 'B')][string]$Set = 'A',      # jeu A (sans perte) ou jeu B (H.264, comparaison Broadcast)
    [ValidateSet('All', 'Kept', 'Generated', 'Aligned24')][string]$Mode = 'All',
    [string]$PreFilter = '',                        # appliquee au flux distordu avant comparaison (ex. Lanczos)
    [switch]$Neg,                                   # second passage avec le modele NEG (garde-fou)
    [int]$Subsample = 1,                            # >1 : iteration rapide seulement, jamais pour un rapport officiel
    [int]$Threads = 8,
    [switch]$SkipAlignCheck,
    [string]$WorkDir
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'QaCommon.ps1')

$t0 = Get-Date
$Reference = (Resolve-Path $Reference).Path; $Distorted = (Resolve-Path $Distorted).Path; $OutJson = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutJson)
$tools = Resolve-FfmpegTools -FfmpegDir $FfmpegDir -NeedVmaf
if (-not $WorkDir) { $WorkDir = Join-Path ([System.IO.Path]::GetTempPath()) ("enhance-qa-" + [guid]::NewGuid().ToString('N').Substring(0, 8)) }
New-Item -ItemType Directory -Path $WorkDir -Force | Out-Null
$warnings = [System.Collections.Generic.List[string]]::new()

# --- Verifications d'entree -------------------------------------------------------------------------------
$ref = Get-VideoInfo $tools $Reference
if ($PreFilter) {
    $dist = Get-VideoInfo $tools $Distorted           # entree 720p : seules les images comptent
} else {
    $dist = Get-VideoInfo $tools $Distorted
    if ([int]$dist.width -ne 3840 -or [int]$dist.height -ne [int]$ref.height) {
        throw "Sortie $($dist.width)x$($dist.height) differente de la verite terrain $($ref.width)x$($ref.height) : comparaison refusee (defaut de conformite)."
    }
    if ($dist.color_space -ne 'bt709' -or $dist.color_range -ne 'tv') {
        $warnings.Add("Etiquettes couleur inattendues (space=$($dist.color_space), range=$($dist.color_range)) : mesure invalide (protocole 2.3).")
    }
}
$refFps = ConvertTo-Rate $ref.r_frame_rate
$distFps = ConvertTo-Rate $dist.r_frame_rate

# Nombre d'images disponibles dans l'espace d'index du flux distordu (apres PreFilter si fps=60 : x2 ou x2,5).
$nRef = [int]$ref.frames
switch ($Mode) {
    'Aligned24' { $distFrames = if ($PreFilter) { [int][math]::Floor($dist.frames * 60 / [math]::Max($distFps, 1)) } else { [int]$dist.frames } }
    default     { $distFrames = if ($PreFilter -and $Mode -in 'Kept', 'Generated') { [int]$dist.frames * 2 } else { [int]$dist.frames } }
}
if ($Mode -eq 'All' -and $PreFilter) { $distFrames = $nRef }

# Expressions de selection (index n) pour chaque flux, et nombre maxi d'images dans l'espace de chaque flux.
switch ($Mode) {
    'All'       { $selD = '1';               $selR = '1';               $limD = $distFrames;                  $limR = $nRef }
    'Kept'      { $selD = 'not(mod(n,2))';   $selR = 'not(mod(n,2))';   $limD = $distFrames;                  $limR = $nRef }
    'Generated' { $selD = 'mod(n,2)';        $selR = 'mod(n,2)';        $limD = $distFrames;                  $limR = $nRef }
    'Aligned24' { $selD = 'not(mod(n,5))';   $selR = 'not(mod(n,2))';   $limD = $distFrames;                  $limR = $nRef }
}
# Alignement : la derniere image interpolee peut manquer (+/- 1), on compare sur le plus petit nombre commun.
$nPairsD = if ($Mode -eq 'Aligned24') { [math]::Ceiling($limD / 5.0) } elseif ($Mode -eq 'All') { $limD } else { [math]::Floor($limD / 2.0) }
$nPairsR = if ($Mode -eq 'Aligned24') { [math]::Ceiling($limR / 2.0) } elseif ($Mode -eq 'All') { $limR } else { [math]::Floor($limR / 2.0) }
$pairs = [int][math]::Min($nPairsD, $nPairsR)
if ([math]::Abs($nPairsD - $nPairsR) -gt 1) {
    $warnings.Add("Nombre d'images different de plus de 1 (distordu=$nPairsD, reference=$nPairsR) : defaut de conformite probable.")
}
if ($pairs -lt 1) { throw 'Aucune paire d''images a comparer.' }
if ($Subsample -gt 1) { $warnings.Add("Sous-echantillonnage x$Subsample : mesure d'iteration, non officielle.") }
$subFilter = if ($Subsample -gt 1) { ",select='not(mod(n,$Subsample))'" } else { '' }

function New-FilterScript {
    param([string]$Model, [string]$LogName, [string]$Extra = 'name=psnr|name=float_ssim')
    $pre = if ($PreFilter) { "$PreFilter," } else { '' }
    $features = if ($Extra) { ":feature=$Extra" } else { '' }
    # setpts : meme base de temps pour les deux flux apres selection, sinon libvmaf apparie par horodatage et se decale.
    @"
[0:v]${pre}select='($selD)*lt(n\,$limD)'${subFilter},setpts=N/30/TB,format=yuv420p10le[d];
[1:v]select='($selR)*lt(n\,$limR)'${subFilter},setpts=N/30/TB,format=yuv420p10le[r];
[d][r]libvmaf=model=version=${Model}${features}:log_fmt=json:log_path=${LogName}:n_threads=$Threads
"@
}

# --- Controle d'alignement temporel (protocole 3.3) ------------------------------------------------------
$alignStatus = 'non_execute'
if (-not $SkipAlignCheck) {
    # En mode Generated, le controle d'alignement se fait sur les images conservees (paires) : sur les images impaires, le decalage 0 et +1 sont ambigus (une image dupliquee est a 1 image de la verite dans les deux cas).
    $alD = if ($Mode -eq 'Generated') { 'not(mod(n,2))' } else { $selD }; $alR = if ($Mode -eq 'Generated') { 'not(mod(n,2))' } else { $selR }
    $psnrs = @{}
    foreach ($shift in -1, 0, 1) {
        $dTrim = [math]::Max($shift, 0); $rTrim = [math]::Max(-$shift, 0)
        $pre = if ($PreFilter) { "$PreFilter," } else { '' }
        $g = "[0:v]${pre}select='($alD)*lt(n\,$limD)',setpts=N/30/TB,format=yuv420p10le,trim=start_frame=${dTrim}:end_frame=$($dTrim + 20),setpts=PTS-STARTPTS[d];" +
             "[1:v]select='($alR)*lt(n\,$limR)',setpts=N/30/TB,format=yuv420p10le,trim=start_frame=${rTrim}:end_frame=$($rTrim + 20),setpts=PTS-STARTPTS[r];[d][r]psnr"
        $gf = Join-Path $WorkDir "align_$shift.txt"
        Set-Content -Path $gf -Value $g -Encoding ascii
        $o = Invoke-Native $tools.Ffmpeg @('-hide_banner', '-nostats', '-i', $Distorted, '-i', $Reference, '-/filter_complex', $gf, '-f', 'null', '-') -WorkDir $WorkDir
        if ($o.StdErr -match 'PSNR y:([0-9.]+|inf)') { $psnrs[$shift] = if ($Matches[1] -eq 'inf') { 100.0 } else { [double]$Matches[1] } }
        else { $psnrs[$shift] = -1.0 }
    }
    $best = ($psnrs.GetEnumerator() | Sort-Object Value -Descending | Select-Object -First 1).Key
    $alignStatus = if ($best -eq 0) { 'ok' } else { 'ALIGNEMENT_SUSPECT' }
    if ($best -ne 0) { $warnings.Add("Alignement suspect : le meilleur PSNR est au decalage $best image(s), pas 0. Mesure rejetee.") }
}

# --- Mesure VMAF (+ SSIM, PSNR) --------------------------------------------------------------------------
function Invoke-Vmaf {
    param([string]$Model, [string]$LogName, [string]$Features)
    $gf = Join-Path $WorkDir "$LogName.filter.txt"
    Set-Content -Path $gf -Value (New-FilterScript -Model $Model -LogName "$LogName.json" -Extra $Features) -Encoding ascii
    $o = Invoke-Native $tools.Ffmpeg @('-hide_banner', '-nostats', '-i', $Distorted, '-i', $Reference, '-/filter_complex', $gf, '-f', 'null', '-') -WorkDir $WorkDir
    $logPath = Join-Path $WorkDir "$LogName.json"
    if ($o.ExitCode -ne 0 -or -not (Test-Path $logPath)) { throw "libvmaf a echoue (code $($o.ExitCode)) : $($o.StdErr.Trim())" }
    return (Get-Content $logPath -Raw | ConvertFrom-Json)
}

function Get-FrameSeries {
    param($Json, [string]$Metric)
    return @($Json.frames | ForEach-Object { $p = $_.metrics.PSObject.Properties[$Metric]; if ($null -ne $p) { [double]$p.Value } })
}

$std = Invoke-Vmaf -Model 'vmaf_4k_v0.6.1' -LogName 'std' -Features 'name=psnr|name=float_ssim'
$vmafFrames = Get-FrameSeries $std 'vmaf'
$psnrFrames = (Get-FrameSeries $std 'psnr_y') | ForEach-Object { [math]::Min($_, 60.0) }   # plafond 60 dB par image
$result = [ordered]@{
    schema      = 'enhance-qa-quality/1'
    clip        = $ClipId; label = $Label; preset = $Preset; set = $Set; mode = $Mode
    tool        = [ordered]@{ ffmpeg = $tools.Version; libvmaf = $(if ($std.PSObject.Properties['version']) { "$($std.version)" } else { 'inconnue' }); model = 'vmaf_4k_v0.6.1' }
    machine     = Get-MachineInfo
    git         = Get-GitHashSafe
    frames_compared = $vmafFrames.Count
    subsample   = $Subsample
    align       = $alignStatus
    vmaf        = [ordered]@{
        mean = [double]$std.pooled_metrics.vmaf.mean; harmonic_mean = [double]$std.pooled_metrics.vmaf.harmonic_mean
        min = [double]$std.pooled_metrics.vmaf.min; p1 = (Get-Percentile $vmafFrames 1)
    }
    ssim_y      = [double]$std.pooled_metrics.float_ssim.mean
    psnr_y      = [math]::Round((($psnrFrames | Measure-Object -Average).Average), 4)
    vmaf_neg    = $null
    warnings    = @()
    elapsed_s   = 0
}
if ($Neg) {
    $negRes = Invoke-Vmaf -Model 'vmaf_4k_v0.6.1neg' -LogName 'neg' -Features ''
    $result.vmaf_neg = [ordered]@{ mean = [double]$negRes.pooled_metrics.vmaf.mean; min = [double]$negRes.pooled_metrics.vmaf.min }
    $result.tool['model_neg'] = 'vmaf_4k_v0.6.1neg'
}
$result.warnings = @($warnings)
$result.elapsed_s = [math]::Round(((Get-Date) - $t0).TotalSeconds, 1)
Write-JsonFile -Object $result -Path $OutJson
Remove-Item -Path $WorkDir -Recurse -Force -ErrorAction SilentlyContinue

Write-Host ("{0} {1} {2} [{3}/{4}] VMAF={5:N2} SSIM={6:N4} PSNR={7:N2} dB  ({8} images, {9} s)" -f `
        $ClipId, $Label, $Preset, $Set, $Mode, $result.vmaf.mean, $result.ssim_y, $result.psnr_y, $result.frames_compared, $result.elapsed_s)
if ($alignStatus -eq 'ALIGNEMENT_SUSPECT') { exit 3 }
exit 0

