#Requires -Version 7.0
<#
.SYNOPSIS
  Vitesse (images/s de sortie), VRAM (pic, fuite sur 10 fichiers) via enhance-bench.exe + nvidia-smi. Ecrit un JSON.
.DESCRIPTION
  Protocole : docs/qa/protocole.md sections 4 et 5. Exige la cible enhance-bench (protocole 4.3) :
    enhance-bench --in <f> --out <f> --preset quality|balanced|fast --codec hevc|av1 --bits 8|10 --report <json>
  Mode Speed : 1 chauffe non comptee + N repetitions, mediane, min-max, alerte si (max-min)/mediane > 3 %.
  Mode Leak  : traite 10 fichiers a la suite, releve la VRAM au repos entre chaque fichier, calcule la derive.
  Echantillonnage externe : nvidia-smi toutes les 200 ms. Carte au repos, autres applications GPU fermees.
  NON EXECUTE a la redaction.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Bench,
    [Parameter(Mandatory)][string[]]$InputFiles,         # Speed : 1 fichier ; Leak : 1 a 10 fichiers (repetes jusqu'a 10)
    [Parameter(Mandatory)][string]$OutJson,
    [Parameter(Mandatory)][string]$OutDir,               # dossier de sortie des videos produites (jetables)
    [ValidateSet('Speed', 'Leak')][string]$Mode = 'Speed',
    [ValidateSet('quality', 'balanced', 'fast')][string]$Preset = 'balanced',
    [ValidateSet('hevc', 'av1')][string]$Codec = 'hevc',
    [ValidateSet(8, 10)][int]$Bits = 10,
    [int]$Repeat = 5,
    [int]$PauseSeconds = 20,
    [switch]$Sustained,                                  # ajoute un essai de 3 min en boucle (chauffe thermique)
    [int]$SustainedSeconds = 180
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'QaCommon.ps1')

$smi = (Get-Command nvidia-smi -ErrorAction Stop).Source
New-Item -ItemType Directory -Path $OutDir -Force | Out-Null

function Get-GpuState {
    $o = Invoke-Native $smi @('--query-gpu=memory.used,temperature.gpu,utilization.gpu,clocks.sm', '--format=csv,noheader,nounits')
    $f = ($o.StdOut.Trim() -split ',') | ForEach-Object { [double]$_.Trim() }
    return [pscustomobject]@{ vram_mib = $f[0]; temp_c = $f[1]; util_pct = $f[2]; sm_mhz = $f[3] }
}

function Start-Sampler {
    $log = Join-Path ([System.IO.Path]::GetTempPath()) ("enhance-smi-" + [guid]::NewGuid().ToString('N').Substring(0, 8) + '.csv')
    $p = Start-Process -FilePath $smi -ArgumentList @('--query-gpu=memory.used,temperature.gpu,clocks.sm,power.draw', '--format=csv,noheader,nounits', '-lms', '200') `
        -RedirectStandardOutput $log -NoNewWindow -PassThru
    return [pscustomobject]@{ Process = $p; Log = $log }
}
function Stop-Sampler($s) {
    try { Stop-Process -Id $s.Process.Id -Force -ErrorAction SilentlyContinue } catch {}
    Start-Sleep -Milliseconds 300
    $rows = @(Get-Content $s.Log -ErrorAction SilentlyContinue | Where-Object { $_ -match '\d' } | ForEach-Object {
            $f = ($_ -split ',') | ForEach-Object { $_.Trim() }
            [pscustomobject]@{ vram = [double]$f[0]; temp = [double]$f[1]; sm = [double]$f[2]; power = [double]($f[3] -replace '[^\d.]', '') }
        })
    Remove-Item $s.Log -Force -ErrorAction SilentlyContinue
    return $rows
}

function Invoke-BenchRun {
    param([string]$In, [string]$Tag)
    $out = Join-Path $OutDir "$Tag.mp4"
    $rep = Join-Path $OutDir "$Tag.report.json"
    $rest = Get-GpuState
    $s = Start-Sampler
    $r = Invoke-Native $Bench @('--in', $In, '--out', $out, '--preset', $Preset, '--codec', $Codec, '--bits', "$Bits", '--report', $rep)
    $rows = Stop-Sampler $s
    $rep_j = if (Test-Path $rep) { Get-Content $rep -Raw | ConvertFrom-Json } else { $null }
    $peakExt = if ($rows.Count) { ($rows | Measure-Object vram -Maximum).Maximum - $rest.vram_mib } else { $null }
    Remove-Item $out -Force -ErrorAction SilentlyContinue
    return [pscustomobject]@{ exit = $r.ExitCode; report = $rep_j; rest = $rest; vram_peak_external_mib = $peakExt; samples = $rows; stderr = $r.StdErr.Trim() }
}

function Get-BenchValue { param($Run, [string]$Name) if ($Run.report) { $p = $Run.report.PSObject.Properties[$Name]; if ($p) { return $p.Value } } return $null }

$result = [ordered]@{
    schema = 'enhance-qa-speed/1'; mode = $Mode; preset = $Preset; codec = $Codec; bits = $Bits
    machine = Get-MachineInfo; git = Get-GitHashSafe
    start_state = Get-GpuState
    warnings = [System.Collections.Generic.List[string]]::new()
}
if ($result.start_state.util_pct -gt 5 -or $result.start_state.temp_c -gt 50) {
    $result.warnings.Add("Carte pas au repos au depart (utilisation $($result.start_state.util_pct) %, $($result.start_state.temp_c) C) : mesure a refaire.")
}
if ($result.start_state.vram_mib -gt 2500) {
    $result.warnings.Add("VRAM deja utilisee au depart : $($result.start_state.vram_mib) Mio (fermer les autres applications). Pic mesure en difference.")
}

if ($Mode -eq 'Speed') {
    $in = $InputFiles[0]
    $warm = Invoke-BenchRun $in 'warmup'                      # chauffe : non comptee (modeles, moteurs, caches)
    if ($warm.exit -ne 0) { throw "enhance-bench a echoue pendant la chauffe (code $($warm.exit)) : $($warm.stderr)" }
    $runs = @()
    for ($i = 1; $i -le $Repeat; $i++) {
        Start-Sleep -Seconds $PauseSeconds
        $r = Invoke-BenchRun $in "run$i"
        if ($r.exit -ne 0) { throw "enhance-bench a echoue a la repetition $i (code $($r.exit)) : $($r.stderr)" }
        $runs += [ordered]@{
            run = $i; fps_out = [double](Get-BenchValue $r 'fps_out'); wall_s = [double](Get-BenchValue $r 'wall_s')
            vram_peak_inprocess_mib = Get-BenchValue $r 'vram_peak_mib'; vram_rest_mib = Get-BenchValue $r 'vram_rest_mib'
            vram_peak_external_mib = $r.vram_peak_external_mib; stages = Get-BenchValue $r 'stages'
            temp_max_c = $(if ($r.samples.Count) { ($r.samples | Measure-Object temp -Maximum).Maximum } else { $null })
            sm_mhz_mean = $(if ($r.samples.Count) { [math]::Round(($r.samples | Measure-Object sm -Average).Average) } else { $null })
        }
    }
    $fps = @($runs | ForEach-Object { $_.fps_out } | Sort-Object)
    $median = if ($fps.Count % 2) { $fps[[int][math]::Floor($fps.Count / 2)] } else { ($fps[$fps.Count / 2 - 1] + $fps[$fps.Count / 2]) / 2 }
    $spread = if ($median -gt 0) { ($fps[-1] - $fps[0]) / $median } else { 1 }
    $result['runs'] = $runs
    $result['fps_out_median'] = [math]::Round($median, 2)
    $result['fps_out_min'] = $fps[0]; $result['fps_out_max'] = $fps[-1]
    $result['spread_pct'] = [math]::Round($spread * 100, 1)
    $result['unstable'] = ($spread -gt 0.03)
    if ($result['unstable']) { $result.warnings.Add("Mesure instable : ecart (max-min)/mediane = $($result['spread_pct']) % > 3 %. Toutes les passes sont conservees ; ne pas relancer pour obtenir un meilleur chiffre.") }
    $vi = @($runs | ForEach-Object { $_.vram_peak_inprocess_mib } | Where-Object { $null -ne $_ })
    $result['vram_peak_inprocess_mib'] = if ($vi.Count) { ($vi | Measure-Object -Maximum).Maximum } else { $null }
    $ve = @($runs | ForEach-Object { $_.vram_peak_external_mib } | Where-Object { $null -ne $_ })
    $result['vram_peak_external_mib'] = if ($ve.Count) { ($ve | Measure-Object -Maximum).Maximum } else { $null }
    if ($result['vram_peak_inprocess_mib'] -and $result['vram_peak_external_mib'] -and
        [math]::Abs($result['vram_peak_inprocess_mib'] - $result['vram_peak_external_mib']) / $result['vram_peak_inprocess_mib'] -gt 0.10) {
        $result.warnings.Add('Ecart > 10 % entre VRAM in-process et nvidia-smi : chercher une consommation hors processus.')
    }
    if ($Sustained) {
        $end = (Get-Date).AddSeconds($SustainedSeconds); $loop = @(); $k = 0
        while ((Get-Date) -lt $end) { $k++; $r = Invoke-BenchRun $in "sustained$k"; $loop += [double](Get-BenchValue $r 'fps_out') }
        $result['sustained_fps_last'] = $loop[-1]
        $result['sustained_ratio'] = [math]::Round($loop[-1] / [math]::Max($median, 0.001), 3)
        if ($result['sustained_ratio'] -lt 0.9) { $result.warnings.Add("Chauffe : vitesse soutenue = $($result['sustained_ratio']) de la vitesse a froid (budgets juges sur la valeur soutenue).") }
    }
}
else {
    # Mode Leak : 10 fichiers a la suite, mesure au repos entre chaque
    $files = @(); while ($files.Count -lt 10) { $files += $InputFiles }; $files = $files[0..9]
    $series = @()
    for ($i = 0; $i -lt 10; $i++) {
        $r = Invoke-BenchRun $files[$i] ("leak{0:D2}" -f ($i + 1))
        if ($r.exit -ne 0) { throw "enhance-bench a echoue au fichier $($i + 1) (code $($r.exit)) : $($r.stderr)" }
        Start-Sleep -Seconds 2
        $after = Get-GpuState
        $series += [ordered]@{
            file = $i + 1; vram_after_mib = $after.vram_mib; vram_peak_inprocess_mib = Get-BenchValue $r 'vram_peak_mib'
            private_mib_end = Get-BenchValue $r 'process_private_mib_end'
        }
    }
    $y = @($series | ForEach-Object { $_.vram_after_mib })
    $n = $y.Count; $xm = ($n - 1) / 2.0; $ym = ($y | Measure-Object -Average).Average
    $num = 0.0; $den = 0.0; for ($i = 0; $i -lt $n; $i++) { $num += ($i - $xm) * ($y[$i] - $ym); $den += ($i - $xm) * ($i - $xm) }
    $slope = if ($den -gt 0) { $num / $den } else { 0 }
    $result['series'] = $series
    $result['vram_drift_mib'] = $y[-1] - $y[0]                    # apres fichier 10 - apres fichier 1
    $result['vram_slope_mib_per_file'] = [math]::Round($slope, 2)
    $result['vram_leak_status'] = if ($result['vram_drift_mib'] -gt 50) { 'FAIL' } elseif ($slope -gt 2) { 'SUSPECT' } else { 'OK' }
    $priv = @($series | ForEach-Object { $_.private_mib_end } | Where-Object { $null -ne $_ })
    if ($priv.Count -ge 2) { $result['private_mem_drift_mib'] = $priv[-1] - $priv[0] }
}

Write-JsonFile -Object $result -Path $OutJson
if ($Mode -eq 'Speed') { Write-Host ("{0}: {1} i/s (mediane), ecart {2} %, VRAM pic {3} Mio" -f $Preset, $result['fps_out_median'], $result['spread_pct'], $result['vram_peak_inprocess_mib']) }
else { Write-Host ("Fuite VRAM : derive {0} Mio, pente {1} Mio/fichier -> {2}" -f $result['vram_drift_mib'], $result['vram_slope_mib_per_file'], $result['vram_leak_status']) }
