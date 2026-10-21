#Requires -Version 7.0
<#
.SYNOPSIS
  Verifie avec ffprobe qu'un fichier produit par Enhance est conforme (protocole 6). Ecrit JSON + Markdown.
.DESCRIPTION
  -Source : fichier d'entree d'origine (duree, cadence, pistes audio / sous-titres attendues).
  Code de sortie : 0 = tout vert ; 1 = au moins un controle rouge ; 2 = erreur d'outil.
  NON EXECUTE a la redaction.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$FfmpegDir,
    [Parameter(Mandatory)][string]$File,
    [Parameter(Mandatory)][string]$Source,
    [ValidateSet('hevc', 'av1')][string]$Codec = 'hevc',
    [ValidateSet(8, 10)][int]$Bits = 10,
    [int]$Width = 3840,
    [int]$Height = 2160,
    [switch]$AllowMkv,                  # bascule MKV annoncee (sous-titre incompatible MP4)
    [string]$OutJson,
    [string]$OutMd
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'QaCommon.ps1')

$tools = Resolve-FfmpegTools -FfmpegDir $FfmpegDir
$checks = [System.Collections.Generic.List[object]]::new()
function Add-Check {
    param([string]$Name, $Expected, $Actual, [bool]$Ok)
    $checks.Add([ordered]@{ name = $Name; expected = "$Expected"; actual = "$Actual"; status = $(if ($Ok) { 'OK' } else { 'KO' }) })
}

function Get-Probe {
    param([string]$Path)
    $r = Invoke-Native $tools.Ffprobe @('-v', 'error', '-show_format', '-show_streams', '-show_chapters', '-of', 'json', $Path)
    if ($r.ExitCode -ne 0) { throw "ffprobe a echoue : $($r.StdErr.Trim())" }
    return ($r.StdOut | ConvertFrom-Json)
}

function Get-Prop { param($Obj, [string]$Name) if ($null -eq $Obj) { return $null }; $p = $Obj.PSObject.Properties[$Name]; if ($p) { return $p.Value } else { return $null } }

try {
    $out = Get-Probe $File
    $src = Get-Probe $Source
} catch { Write-Error $_; exit 2 }

$ov = @($out.streams | Where-Object codec_type -eq 'video')[0]
$sv = @($src.streams | Where-Object codec_type -eq 'video')[0]
$oa = @($out.streams | Where-Object codec_type -eq 'audio')
$sa = @($src.streams | Where-Object codec_type -eq 'audio')
$os = @($out.streams | Where-Object codec_type -eq 'subtitle')
$ss = @($src.streams | Where-Object codec_type -eq 'subtitle')

# --- Conteneur
$fmtNames = "$($out.format.format_name)"
$isMp4 = $fmtNames -match 'mp4'
$isMkv = $fmtNames -match 'matroska'
Add-Check 'conteneur' $(if ($AllowMkv) { 'mp4 ou matroska (bascule annoncee)' } else { 'mp4' }) $fmtNames ($isMp4 -or ($AllowMkv -and $isMkv))
if ($isMp4) {
    # moov en tete : lecture des 64 premiers Kio, atome 'moov' avant 'mdat'
    $fs = [System.IO.File]::OpenRead($File)
    try { $buf = New-Object byte[] 65536; $n = $fs.Read($buf, 0, $buf.Length) } finally { $fs.Dispose() }
    $head = [System.Text.Encoding]::ASCII.GetString($buf, 0, $n)
    $iMoov = $head.IndexOf('moov'); $iMdat = $head.IndexOf('mdat')
    Add-Check 'moov en tete (demarrage rapide)' 'moov avant mdat' $(if ($iMoov -ge 0) { 'oui' } else { 'non' }) ($iMoov -ge 0 -and ($iMdat -lt 0 -or $iMoov -lt $iMdat))
}

# --- Video : codec, profil, pixel, taille
$expCodec = $Codec
Add-Check 'codec video' $expCodec $ov.codec_name ($ov.codec_name -eq $expCodec)
$expProfile = if ($Codec -eq 'hevc') { if ($Bits -eq 10) { 'Main 10' } else { 'Main' } } else { 'Main' }
Add-Check 'profil' $expProfile (Get-Prop $ov 'profile') ((Get-Prop $ov 'profile') -eq $expProfile)
$expPix = if ($Bits -eq 10) { 'yuv420p10le' } else { 'yuv420p' }
Add-Check 'format de pixel' $expPix $ov.pix_fmt ($ov.pix_fmt -eq $expPix)
Add-Check 'resolution' "${Width}x$Height" "$($ov.width)x$($ov.height)" ([int]$ov.width -eq $Width -and [int]$ov.height -eq $Height)
$sar = Get-Prop $ov 'sample_aspect_ratio'
Add-Check 'PAR 1:1' '1:1 (ou absent)' $sar (($null -eq $sar) -or $sar -in '1:1', '0:1')
if ($isMp4) {
    $tag = Get-Prop $ov 'codec_tag_string'
    $expTag = if ($Codec -eq 'hevc') { 'hvc1' } else { 'av01' }
    Add-Check 'etiquette MP4' $expTag $tag ($tag -eq $expTag)
}

# --- Couleur SDR BT.709 limite (HDR10 hors MVP)
foreach ($pair in @(@('color_space', 'bt709'), @('color_transfer', 'bt709'), @('color_primaries', 'bt709'), @('color_range', 'tv'))) {
    $v = Get-Prop $ov $pair[0]
    Add-Check $pair[0] $pair[1] $v ($v -eq $pair[1])
}

# --- Cadence
$srcFps = ConvertTo-Rate (Get-Prop $sv 'avg_frame_rate')
$expFps = if ($srcFps -ge 50) { $srcFps } else { 60.0 }
$outR = ConvertTo-Rate $ov.r_frame_rate
$outA = ConvertTo-Rate $ov.avg_frame_rate
Add-Check 'cadence attendue (>=50 conservee, sinon 60)' ("{0:N3}" -f $expFps) ("{0:N3}" -f $outA) ([math]::Abs($outA - $expFps) -lt 0.05)
Add-Check 'CFR (r_frame_rate = avg_frame_rate)' ("{0:N3}" -f $outR) ("{0:N3}" -f $outA) ([math]::Abs($outR - $outA) -lt 0.01)

# --- Duree et nombre d'images, horodatages
$srcDur = [double](Get-Prop $sv 'duration'); if ($srcDur -eq 0) { $srcDur = [double]$src.format.duration }
$outDur = [double](Get-Prop $ov 'duration'); if ($outDur -eq 0) { $outDur = [double]$out.format.duration }
$frameDur = 1.0 / [math]::Max($expFps, 1)
Add-Check 'duree video = source +/- 1 image' ("{0:N3} s" -f $srcDur) ("{0:N3} s" -f $outDur) ([math]::Abs($outDur - $srcDur) -le $frameDur + 0.001)
$pk = Invoke-Native $tools.Ffprobe @('-v', 'error', '-select_streams', 'v:0', '-show_entries', 'packet=pts_time,dts_time', '-of', 'csv=p=0', $File)
$pts = @($pk.StdOut -split "`r?`n" | Where-Object { $_ } | ForEach-Object { [double](($_ -split ',')[0]) })
$expFrames = [math]::Round($srcDur * $expFps)
Add-Check 'nombre d images = round(duree x cadence) +/- 1' "$expFrames" "$($pts.Count)" ([math]::Abs($pts.Count - $expFrames) -le 1)
$sorted = $true; for ($i = 1; $i -lt $pts.Count; $i++) { if ($pts[$i] -le $pts[$i - 1]) { $sorted = $false; break } }
Add-Check 'horodatages strictement croissants' 'oui' $(if ($sorted) { 'oui' } else { 'non' }) $sorted
if ($pts.Count -gt 2) {
    $maxJitter = 0.0
    for ($i = 1; $i -lt $pts.Count; $i++) { $maxJitter = [math]::Max($maxJitter, [math]::Abs(($pts[$i] - $pts[$i - 1]) - $frameDur)) }
    Add-Check 'regularite (ecart max a 1/cadence < 2 ms)' '< 0.002 s' ("{0:N4} s" -f $maxJitter) ($maxJitter -lt 0.002)
}

# --- Audio : pistes, codec, langue, synchro
Add-Check 'nombre de pistes audio' $sa.Count $oa.Count ($sa.Count -eq $oa.Count)
for ($i = 0; $i -lt [math]::Min($sa.Count, $oa.Count); $i++) {
    $lsrc = Get-Prop (Get-Prop $sa[$i] 'tags') 'language'; $lout = Get-Prop (Get-Prop $oa[$i] 'tags') 'language'
    Add-Check "audio #$i langue" $lsrc $lout ($lsrc -eq $lout)
    $aStart = [double](Get-Prop $oa[$i] 'start_time'); $vStart = [double](Get-Prop $ov 'start_time')
    Add-Check "audio #$i : decalage de depart A/V <= 40 ms" '<= 0.040 s' ("{0:N3} s" -f [math]::Abs($aStart - $vStart)) ([math]::Abs($aStart - $vStart) -le 0.040)
    $aDur = [double](Get-Prop $oa[$i] 'duration')
    if ($aDur -gt 0) { Add-Check "audio #$i : duree audio - duree video <= 40 ms" '<= 0.040 s' ("{0:N3} s" -f [math]::Abs($aDur - $outDur)) ([math]::Abs($aDur - $outDur) -le 0.040) }
}

# --- Sous-titres : conserves, sauf bascule annoncee
Add-Check 'nombre de pistes de sous-titres' $ss.Count $os.Count ($ss.Count -eq $os.Count)
for ($i = 0; $i -lt [math]::Min($ss.Count, $os.Count); $i++) {
    $lsrc = Get-Prop (Get-Prop $ss[$i] 'tags') 'language'; $lout = Get-Prop (Get-Prop $os[$i] 'tags') 'language'
    Add-Check "sous-titre #$i langue" $lsrc $lout ($lsrc -eq $lout)
}

# --- Chapitres
$sc = @($src.chapters).Count; $oc = @($out.chapters).Count
Add-Check 'chapitres conserves' $sc $oc ($sc -eq $oc)

# --- Aucune donnee personnelle dans les etiquettes
$tagText = @()
foreach ($o in @($out.format) + @($out.streams)) { $t = Get-Prop $o 'tags'; if ($t) { $tagText += ($t.PSObject.Properties | ForEach-Object { "$($_.Value)" }) } }
$leak = $tagText | Where-Object { $_ -match '[A-Za-z]:\\' -or $_ -match '\\Users\\' -or $_ -match '/home/' -or $_ -match '@[A-Za-z0-9.-]+\.[A-Za-z]{2,}' }
Add-Check 'aucun chemin ni e-mail dans les tags' 'aucun' $(if ($leak) { 'present' } else { 'aucun' }) (-not $leak)

# --- Integrite du decodage
$dec = Invoke-Native $tools.Ffmpeg @('-v', 'error', '-nostats', '-i', $File, '-f', 'null', '-')
$decErr = ($dec.StdErr.Trim() -split "`r?`n" | Where-Object { $_ }).Count
Add-Check 'decodage complet sans erreur' '0 ligne d erreur' "$decErr" ($dec.ExitCode -eq 0 -and $decErr -eq 0)

# --- Sorties
$ko = @($checks | Where-Object status -eq 'KO')
$report = [ordered]@{
    schema = 'enhance-qa-conformity/1'
    file = (Split-Path $File -Leaf)             # nom seul : jamais de chemin
    tool = $tools.Version
    verdict = $(if ($ko.Count -eq 0) { 'PASS' } else { 'FAIL' })
    ko = $ko.Count
    checks = $checks
}
if ($OutJson) { Write-JsonFile -Object $report -Path $OutJson }
$md = [System.Text.StringBuilder]::new()
[void]$md.AppendLine("### Conformite : $($report.file) : **$($report.verdict)**")
[void]$md.AppendLine('')
[void]$md.AppendLine('| Controle | Attendu | Constate | |')
[void]$md.AppendLine('|---|---|---|---|')
foreach ($c in $checks) { [void]$md.AppendLine("| $($c.name) | $($c.expected) | $($c.actual) | $($c.status) |") }
if ($OutMd) { Set-Content -Path $OutMd -Value $md.ToString() -Encoding utf8 } else { Write-Host $md.ToString() }
exit $(if ($ko.Count -eq 0) { 0 } else { 1 })
