#Requires -Version 7.0
<#
.SYNOPSIS
  Agrege les JSON de mesure (qualite, vitesse, conformite) d'un dossier en un rapport Markdown + CSV avec verdicts de seuils.
.DESCRIPTION
  Seuils : docs/qa/protocole.md section 7 (proposes [H]). Entrees (JSON ecrits par les autres scripts) :
    quality_*.json  (Measure-Quality.ps1), speed_*.json (Measure-Speed.ps1, Mode Speed et Leak), conformity_*.json.
  Sortie : <OutBase>.md et <OutBase>.csv (sans chemin personnel).
  -BaselineDir : dossier d'un rapport precedent de main, pour la regle de non-regression (> 5 %).
  NON EXECUTE a la redaction.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$ResultsDir,
    [Parameter(Mandatory)][string]$OutBase,          # ex. docs\qa\resultats\2026-11-03_1015_ENH-010_feature-x
    [Parameter(Mandatory)][string]$Story,
    [string]$Branch = 'inconnue',
    [string]$BaselineDir
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'QaCommon.ps1')

# --- Seuils proposes (protocole 7) ---------------------------------------------------------------
$Thr = @{
    quality  = @{ dVmaf = 5.0; dSsim = 0.02;  dPsnr = 1.5; dNeg = 3.0; minClipVmaf = 2.0; fps = 30;  gen = 3.0; genP1Margin = 0.0 }
    balanced = @{ dVmaf = 4.0; dSsim = 0.015; dPsnr = 1.0; dNeg = 2.0; minClipVmaf = 1.0; fps = 60;  gen = 3.0; genP1Margin = 0.0 }
    fast     = @{ dVmaf = 2.0; dSsim = 0.01;  dPsnr = 0.5; dNeg = 1.0; minClipVmaf = 0.0; fps = 100; gen = 1.0; genP1Margin = -2.0 }
}
$VramMax = 9216; $VramWarn = 8500; $LeakMax = 50; $PrivMax = 100; $BroadcastTolVmaf = 0.5; $RegressPct = 5.0

function Read-Jsons([string]$Dir, [string]$Schema) {
    if (-not $Dir -or -not (Test-Path $Dir)) { return @() }
    Get-ChildItem $Dir -Filter *.json -File | ForEach-Object { Get-Content $_.FullName -Raw | ConvertFrom-Json } |
        Where-Object { $_.PSObject.Properties['schema'] -and $_.schema -like "$Schema*" }
}

$q = @(Read-Jsons $ResultsDir 'enhance-qa-quality')
$sp = @(Read-Jsons $ResultsDir 'enhance-qa-speed')
$cf = @(Read-Jsons $ResultsDir 'enhance-qa-conformity')
$qBase = @(Read-Jsons $BaselineDir 'enhance-qa-quality')
$spBase = @(Read-Jsons $BaselineDir 'enhance-qa-speed')
if ($q.Count -eq 0 -and $sp.Count -eq 0 -and $cf.Count -eq 0) { throw 'Aucun JSON de mesure trouve.' }

$checks = [System.Collections.Generic.List[object]]::new()
function Add-Verdict([string]$Name, [string]$Status, [string]$Detail) { $checks.Add([pscustomobject]@{ name = $Name; status = $Status; detail = $Detail }) }

# --- Qualite : tableau + deltas vs Lanczos ---------------------------------------------------------
$rows = [System.Collections.Generic.List[object]]::new()
foreach ($m in $q) {
    $lan = $q | Where-Object { $_.label -eq 'Lanczos' -and $_.clip -eq $m.clip -and $_.set -eq $m.set -and $_.mode -eq $m.mode } | Select-Object -First 1
    $bro = $q | Where-Object { $_.label -eq 'Broadcast' -and $_.clip -eq $m.clip -and $_.mode -eq $m.mode } | Select-Object -First 1
    $negVal = if ($m.vmaf_neg) { $m.vmaf_neg.mean } else { $null }
    $rows.Add([pscustomobject]@{
            clip = $m.clip; preset = $m.preset; label = $m.label; set = $m.set; mode = $m.mode
            vmaf = $m.vmaf.mean; vmaf_p1 = $m.vmaf.p1; vmaf_neg = $negVal; ssim = $m.ssim_y; psnr = $m.psnr_y
            dVmaf = $(if ($lan) { $m.vmaf.mean - $lan.vmaf.mean } else { $null })
            dNeg = $(if ($lan -and $lan.vmaf_neg -and $negVal) { $negVal - $lan.vmaf_neg.mean } else { $null })
            dSsim = $(if ($lan) { $m.ssim_y - $lan.ssim_y } else { $null })
            dPsnr = $(if ($lan) { $m.psnr_y - $lan.psnr_y } else { $null })
            p1Lan = $(if ($lan) { $lan.vmaf.p1 } else { $null })
            dVsBroadcast = $(if ($bro -and $m.label -eq 'Enhance' -and $m.set -eq 'B') { $m.vmaf.mean - $bro.vmaf.mean } else { $null })
            dSsimBro = $(if ($bro -and $m.label -eq 'Enhance' -and $m.set -eq 'B') { $m.ssim_y - $bro.ssim_y } else { $null })
            dPsnrBro = $(if ($bro -and $m.label -eq 'Enhance' -and $m.set -eq 'B') { $m.psnr_y - $bro.psnr_y } else { $null })
            align = $m.align; warn = ($m.warnings -join ' / ')
        })
}

foreach ($preset in 'quality', 'balanced', 'fast') {
    $t = $Thr[$preset]
    # SR seule : jeu A, images conservees / alignees / toutes (hors Generated)
    $sr = @($rows | Where-Object { $_.label -eq 'Enhance' -and $_.preset -eq $preset -and $_.set -eq 'A' -and $_.mode -ne 'Generated' -and $null -ne $_.dVmaf })
    if ($sr.Count) {
        $mv = ($sr | Measure-Object dVmaf -Average).Average; $ms = ($sr | Measure-Object dSsim -Average).Average; $mp = ($sr | Measure-Object dPsnr -Average).Average
        Add-Verdict "[$preset] VMAF vs Lanczos (moyenne, >= +$($t.dVmaf))" $(if ($mv -ge $t.dVmaf) { 'OK' } else { 'KO' }) ("{0:+0.00;-0.00}" -f $mv)
        Add-Verdict "[$preset] SSIM vs Lanczos (moyenne, >= +$($t.dSsim))" $(if ($ms -ge $t.dSsim) { 'OK' } else { 'KO' }) ("{0:+0.0000;-0.0000}" -f $ms)
        Add-Verdict "[$preset] PSNR vs Lanczos (moyenne, >= +$($t.dPsnr) dB)" $(if ($mp -ge $t.dPsnr) { 'OK' } else { 'KO' }) ("{0:+0.00;-0.00}" -f $mp)
        $negs = @($sr | Where-Object { $null -ne $_.dNeg })
        if ($negs.Count) { $mn = ($negs | Measure-Object dNeg -Average).Average; Add-Verdict "[$preset] VMAF-NEG vs Lanczos (garde-fou, >= +$($t.dNeg))" $(if ($mn -ge $t.dNeg) { 'OK' } else { 'KO' }) ("{0:+0.00;-0.00}" -f $mn) }
        else { Add-Verdict "[$preset] VMAF-NEG (garde-fou)" 'NON VERIFIE' 'passer -Neg a Measure-Quality' }
        foreach ($c in $sr) {
            $bad = ($c.dVmaf -lt $t.minClipVmaf) -or ($c.dSsim -lt 0) -or ($c.dPsnr -lt 0)
            if ($bad) { Add-Verdict "[$preset] minimum par clip : $($c.clip)" 'KO' ("dVMAF {0:+0.00;-0.00}, dSSIM {1:+0.0000;-0.0000}, dPSNR {2:+0.00;-0.00}" -f $c.dVmaf, $c.dSsim, $c.dPsnr) }
        }
    } else { Add-Verdict "[$preset] qualite vs Lanczos" 'NON VERIFIE' 'aucune mesure Enhance / Lanczos appariee (jeu A)' }

    # Interpolation : images generees
    $gen = @($rows | Where-Object { $_.label -eq 'Enhance' -and $_.preset -eq $preset -and $_.set -eq 'A' -and $_.mode -eq 'Generated' -and $null -ne $_.dVmaf })
    foreach ($c in $gen) {
        Add-Verdict "[$preset] interpolation $($c.clip) : VMAF Generated vs duplication (>= +$($t.gen))" $(if ($c.dVmaf -ge $t.gen) { 'OK' } else { 'KO' }) ("{0:+0.00;-0.00}" -f $c.dVmaf)
        Add-Verdict "[$preset] interpolation $($c.clip) : 1er centile >= duplication ($($t.genP1Margin))" $(if ($c.vmaf_p1 -ge ($c.p1Lan + $t.genP1Margin)) { 'OK' } else { 'KO' }) ("{0:N1} contre {1:N1}" -f $c.vmaf_p1, $c.p1Lan)
    }
}

# Broadcast (jeu B, Qualite max, >= 2 clips)
$vb = @($rows | Where-Object { $null -ne $_.dVsBroadcast -and $_.preset -eq 'quality' -and $_.mode -ne 'Generated' })
if ($vb.Count -ge 2) {
    foreach ($c in $vb) {
        $ok = ($c.dVsBroadcast -ge -$BroadcastTolVmaf) -and ($c.dSsimBro -ge 0) -and ($c.dPsnrBro -ge 0)
        Add-Verdict "Broadcast 2.3 : $($c.clip) (VMAF >= -0,5 ; SSIM, PSNR >= 0)" $(if ($ok) { 'OK' } else { 'KO' }) ("dVMAF {0:+0.00;-0.00}, dSSIM {1:+0.0000;-0.0000}, dPSNR {2:+0.00;-0.00}" -f $c.dVsBroadcast, $c.dSsimBro, $c.dPsnrBro)
    }
} else { Add-Verdict 'Broadcast 2.3 (>= 2 clips)' 'NON VERIFIE' 'sorties Broadcast absentes ou moins de 2 clips (voir docs/qa/broadcast.md)' }

# --- Vitesse et VRAM --------------------------------------------------------------------------------
foreach ($s in ($sp | Where-Object mode -eq 'Speed')) {
    $t = $Thr[$s.preset]; $f = $s.fps_out_median
    $state = if ($f -ge $t.fps) { 'OK' } elseif ($f -ge 0.8 * $t.fps) { 'AVERTISSEMENT' } else { 'KO' }
    if ($s.unstable -and $state -eq 'OK') { $state = 'INSTABLE' }
    Add-Verdict "[$($s.preset)] vitesse (budget $($t.fps) i/s ; KO sous $([math]::Round(0.8*$t.fps)))" $state ("{0:N1} i/s, ecart {1} %" -f $f, $s.spread_pct)
    $vp = $s.vram_peak_inprocess_mib
    if ($null -ne $vp) { Add-Verdict "[$($s.preset)] VRAM pic <= $VramMax Mio" $(if ($vp -gt $VramMax) { 'KO' } elseif ($vp -gt $VramWarn) { 'AVERTISSEMENT' } else { 'OK' }) ("$vp Mio") }
    else { Add-Verdict "[$($s.preset)] VRAM pic" 'NON VERIFIE' 'pas de mesure in-process (enhance-bench)' }
    # regression vs main
    $b = $spBase | Where-Object { $_.mode -eq 'Speed' -and $_.preset -eq $s.preset -and $_.codec -eq $s.codec -and $_.bits -eq $s.bits } | Select-Object -First 1
    if ($b) {
        $dF = ($f - $b.fps_out_median) / $b.fps_out_median * 100
        Add-Verdict "[$($s.preset)] non-regression vitesse vs main (> -$RegressPct %)" $(if ($dF -lt -$RegressPct) { 'KO' } else { 'OK' }) ("{0:+0.0;-0.0} %" -f $dF)
        if ($b.vram_peak_inprocess_mib -and $vp) {
            $dV = ($vp - $b.vram_peak_inprocess_mib) / $b.vram_peak_inprocess_mib * 100
            Add-Verdict "[$($s.preset)] non-regression VRAM vs main (< +$RegressPct %)" $(if ($dV -gt $RegressPct) { 'KO' } else { 'OK' }) ("{0:+0.0;-0.0} %" -f $dV)
        }
    }
}
foreach ($l in ($sp | Where-Object mode -eq 'Leak')) {
    Add-Verdict "[$($l.preset)] derive VRAM sur 10 fichiers (<= $LeakMax Mio, pente non positive)" $(if ($l.vram_leak_status -eq 'OK') { 'OK' } elseif ($l.vram_leak_status -eq 'SUSPECT') { 'AVERTISSEMENT' } else { 'KO' }) ("derive {0} Mio, pente {1} Mio/fichier" -f $l.vram_drift_mib, $l.vram_slope_mib_per_file)
    if ($l.PSObject.Properties['private_mem_drift_mib']) { Add-Verdict "[$($l.preset)] derive memoire systeme (<= $PrivMax Mio)" $(if ($l.private_mem_drift_mib -gt $PrivMax) { 'KO' } else { 'OK' }) ("$($l.private_mem_drift_mib) Mio") }
}

# Non-regression qualite vs main (relative > 5 %)
foreach ($m in ($rows | Where-Object { $_.label -eq 'Enhance' })) {
    $b = $qBase | Where-Object { $_.label -eq 'Enhance' -and $_.clip -eq $m.clip -and $_.preset -eq $m.preset -and $_.set -eq $m.set -and $_.mode -eq $m.mode } | Select-Object -First 1
    if ($b) {
        $bad = @()
        if ($m.vmaf -lt $b.vmaf.mean * (1 - $RegressPct / 100)) { $bad += 'VMAF' }
        if ($m.ssim -lt $b.ssim_y * (1 - $RegressPct / 100)) { $bad += 'SSIM' }
        if ($m.psnr -lt $b.psnr_y * (1 - $RegressPct / 100)) { $bad += 'PSNR' }
        if ($bad.Count) { Add-Verdict "non-regression qualite vs main : $($m.clip) [$($m.preset)]" 'KO' ($bad -join ', ') }
        elseif ($m.vmaf -lt $b.vmaf.mean - 1) { Add-Verdict "non-regression qualite vs main : $($m.clip) [$($m.preset)]" 'AVERTISSEMENT' ("VMAF {0:N2} -> {1:N2}" -f $b.vmaf.mean, $m.vmaf) }
    }
}

# Conformite
foreach ($c in $cf) { Add-Verdict "conformite : $($c.file)" $(if ($c.verdict -eq 'PASS') { 'OK' } else { 'KO' }) "$($c.ko) controle(s) rouge(s)" }
if ($cf.Count -eq 0) { Add-Verdict 'conformite des fichiers produits' 'NON VERIFIE' 'aucun conformity_*.json' }
foreach ($m in $q) { if ($m.align -eq 'ALIGNEMENT_SUSPECT') { Add-Verdict "alignement temporel : $($m.clip) / $($m.label)" 'KO' 'mesure rejetee (protocole 3.3)' } }

# Verdict global : KO -> FAIL ; NON VERIFIE ne donne jamais un PASS silencieux (liste dans le rapport)
$ko = @($checks | Where-Object status -eq 'KO')
$nv = @($checks | Where-Object status -eq 'NON VERIFIE')
$verdict = if ($ko.Count) { 'FAIL' } elseif ($nv.Count) { 'PASS (partiel : voir NON VERIFIE)' } else { 'PASS' }

# --- Markdown + CSV -----------------------------------------------------------------------------------
$any = if ($q.Count) { $q[0] } else { $null }
$md = [System.Text.StringBuilder]::new()
[void]$md.AppendLine("# Rapport QA : $Story ($Branch)")
[void]$md.AppendLine('')
[void]$md.AppendLine("VERDICT QA: $verdict")
[void]$md.AppendLine('')
[void]$md.AppendLine('## Conditions')
$mach = if ($any) { $any.machine } elseif ($sp.Count) { $sp[0].machine } else { $null }
if ($mach) { [void]$md.AppendLine("- Machine : $($mach.gpu), pilote $($mach.driver), Windows $($mach.windows)") }
if ($any) { [void]$md.AppendLine("- FFmpeg de mesure : $($any.tool.ffmpeg) ; libvmaf $($any.tool.libvmaf) ; modele $($any.tool.model)") }
[void]$md.AppendLine("- Commit : $(Get-GitHashSafe)")
$tools = @($q | ForEach-Object { "$($_.tool.ffmpeg)|$($_.tool.model)" } | Select-Object -Unique)
if ($tools.Count -gt 1) { [void]$md.AppendLine('- ATTENTION : outils de mesure differents entre mesures du rapport (comparaison non valide).') }
[void]$md.AppendLine('')
[void]$md.AppendLine('## Seuils')
[void]$md.AppendLine('| Controle | Etat | Valeur |')
[void]$md.AppendLine('|---|---|---|')
foreach ($c in $checks) { [void]$md.AppendLine("| $($c.name) | $($c.status) | $($c.detail) |") }
[void]$md.AppendLine('')
[void]$md.AppendLine('## Mesures (clip, prereglage)')
[void]$md.AppendLine('| Clip | Prereglage | Reference | Jeu | Images | VMAF | VMAF-NEG | SSIM | PSNR Y | dVMAF vs Lanczos | vs Broadcast (dVMAF) | images/s | VRAM pic |')
[void]$md.AppendLine('|---|---|---|---|---|---|---|---|---|---|---|---|---|')
foreach ($r in ($rows | Sort-Object clip, preset, label, set, mode)) {
    $spd = $sp | Where-Object { $_.mode -eq 'Speed' -and $_.preset -eq $r.preset } | Select-Object -First 1
    $fps = if ($spd -and $r.label -eq 'Enhance') { '{0:N1}' -f $spd.fps_out_median } else { '' }
    $vr = if ($spd -and $r.label -eq 'Enhance') { "$($spd.vram_peak_inprocess_mib)" } else { '' }
    $neg = if ($null -ne $r.vmaf_neg) { '{0:N2}' -f $r.vmaf_neg } else { '' }
    $dv = if ($null -ne $r.dVmaf) { '{0:+0.00;-0.00}' -f $r.dVmaf } else { '' }
    $db = if ($null -ne $r.dVsBroadcast) { '{0:+0.00;-0.00}' -f $r.dVsBroadcast } else { '' }
    [void]$md.AppendLine(("| {0} | {1} | {2} | {3} | {4} | {5:N2} | {6} | {7:N4} | {8:N2} | {9} | {10} | {11} | {12} |" -f $r.clip, $r.preset, $r.label, $r.set, $r.mode, $r.vmaf, $neg, $r.ssim, $r.psnr, $dv, $db, $fps, $vr))
}
$warns = @($rows | Where-Object { $_.warn } | ForEach-Object { "- $($_.clip) / $($_.label) : $($_.warn)" })
foreach ($s in $sp) { foreach ($w in $s.warnings) { $warns += "- vitesse [$($s.preset)] : $w" } }
if ($warns.Count) { [void]$md.AppendLine(''); [void]$md.AppendLine('## Avertissements'); $warns | ForEach-Object { [void]$md.AppendLine($_) } }

$dir = Split-Path $OutBase -Parent
if ($dir -and -not (Test-Path $dir)) { New-Item -ItemType Directory -Path $dir -Force | Out-Null }
Set-Content -Path "$OutBase.md" -Value $md.ToString() -Encoding utf8
$rows | Export-Csv -Path "$OutBase.csv" -NoTypeInformation -Encoding utf8 -UseCulture:$false
Write-Host "VERDICT QA: $verdict  ($($ko.Count) KO, $($nv.Count) non verifie)  -> $OutBase.md"
exit $(if ($ko.Count) { 1 } else { 0 })
