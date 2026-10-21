#Requires -Version 7.0
# Genere le rapport Markdown de la mesure de reference a partir des JSON de Run-LanczosReference.ps1.
param([Parameter(Mandatory)][string]$InDir, [Parameter(Mandatory)][string]$OutMd)
$r = (Get-Content (Join-Path $InDir 'reference-lanczos.json') -Raw | ConvertFrom-Json).results
$f = [cultureinfo]::InvariantCulture
$sb = [System.Text.StringBuilder]::new()
[void]$sb.AppendLine('| Clip | Reference | Jeu | Images comparees | VMAF | VMAF-NEG | VMAF p1 | VMAF min | SSIM Y | PSNR Y (dB) | Alignement |')
[void]$sb.AppendLine('|---|---|---|---|---|---|---|---|---|---|---|')
foreach ($x in ($r | Sort-Object clip, label, set, mode)) {
    $neg = if ($x.vmaf_neg) { $x.vmaf_neg.mean.ToString('F2', $f) } else { '-' }
    [void]$sb.AppendLine(('| {0} | {1} {2} | {3} | {4} | {5} | {6} | {7} | {8} | {9} | {10} | {11} |' -f $x.clip, $x.label, $x.mode, $x.set, $x.frames_compared,
        $x.vmaf.mean.ToString('F2', $f), $neg, $x.vmaf.p1.ToString('F2', $f), $x.vmaf.min.ToString('F2', $f), $x.ssim_y.ToString('F4', $f), $x.psnr_y.ToString('F2', $f), $x.align))
}
$t = $r[0].tool
$md = @"
# Mesure de reference Lanczos (plancher) - $(Get-Date -Format yyyy-MM-dd)

Machine : $($r[0].machine.gpu), pilote $($r[0].machine.driver), Windows $($r[0].machine.windows). Depot : $($r[0].git).
Outils : $($t.ffmpeg) ; libvmaf $($t.libvmaf) ; modeles $($t.model) et $($t.model_neg). Details : docs/qa/outils.md.

$($sb.ToString())
"@
Set-Content -Path $OutMd -Value $md -Encoding utf8NoBOM
