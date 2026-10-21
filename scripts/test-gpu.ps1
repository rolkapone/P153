<#
.SYNOPSIS
  Tests GPU locaux (ADR-031) : verifie les prerequis, compile en mode GPU, lance les tests
  CTest "gpu", ecrit un rapport horodate dans docs\qa\resultats\.
.DESCRIPTION
  Codes de sortie : 0 = tout vert ; 1 = un test est rouge ; 2 = prerequis manquant.
  -CheckOnly : ne verifie que les prerequis (pas de build, pas de rapport).
  -Quick     : omet les mesures longues.
  -Story     : filtre les tests par expression reguliere (ex. ENH-010).
.EXAMPLE
  scripts\test-gpu.ps1 -CheckOnly
#>
[CmdletBinding()]
param(
    [string]$Preset = 'local-gpu',
    [switch]$CheckOnly,
    [switch]$Quick,
    [string]$Story = ''
)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot\common.ps1"

$req = Get-Content (Join-Path $PSScriptRoot 'gpu-requirements.json') -Raw | ConvertFrom-Json
$checks = New-Object System.Collections.Generic.List[object]
$info = [ordered]@{}
function Add-Check([string]$Name, [bool]$Ok, [string]$Detail, [bool]$Blocking = $true) {
    $status = if ($Ok) { 'OK' } elseif ($Blocking) { 'MANQUANT' } else { 'AVERTISSEMENT' }
    $checks.Add([pscustomobject]@{ Name = $Name; Status = $status; Detail = $Detail })
    $color = if ($Ok) { 'Green' } elseif ($Blocking) { 'Red' } else { 'Yellow' }
    Write-Host ("[{0}] {1} : {2}" -f $status, $Name, $Detail) -ForegroundColor $color
}

# --- 1. Carte et pilote ---------------------------------------------------------------------
$smi = Get-Command nvidia-smi -ErrorAction SilentlyContinue
if (-not $smi) {
    Add-Check 'Carte NVIDIA' $false 'nvidia-smi introuvable (pilote NVIDIA non installe ?)'
} else {
    $line = (& nvidia-smi --query-gpu=name,driver_version,compute_cap,memory.total,memory.used --format=csv,noheader,nounits | Select-Object -First 1)
    $p = $line -split ',\s*'
    $info.GPU = $p[0]; $info.Pilote = $p[1]; $info.CapaciteCalcul = $p[2]
    $info.VramTotaleMiB = $p[3]; $info.VramUtiliseeDepartMiB = $p[4]
    Add-Check 'Carte NVIDIA' ([double]::Parse($p[2], [cultureinfo]::InvariantCulture) -ge [double]::Parse($req.minComputeCapability, [cultureinfo]::InvariantCulture)) `
        ("{0}, capacite de calcul {1} (>= {2} requis), {3} Mio" -f $p[0], $p[2], $req.minComputeCapability, $p[3])
    Add-Check 'Pilote' ([version]$p[1] -ge [version]$req.minDriver) ("{0} (>= {1} requis)" -f $p[1], $req.minDriver)
    if ($req.PSObject.Properties['referenceDriver'] -and $req.referenceDriver -and ([version]$p[1] -ne [version]$req.referenceDriver)) {
        # Avertissement seulement : les mesures de reference ont ete prises avec ce pilote.
        Add-Check 'Pilote de reference' $false ("{0} installe, mesures de reference prises avec {1} (comparaisons a interpreter avec prudence)" -f $p[1], $req.referenceDriver) $false
    }
    Add-Check 'VRAM au depart' ([int]$p[4] -le [int]$req.maxUsedVramAtStartMiB) `
        ("{0} Mio deja utilises (seuil {1} Mio)" -f $p[4], $req.maxUsedVramAtStartMiB) $false
}

# --- 2. CUDA Toolkit ------------------------------------------------------------------------
$nvcc = Get-Command nvcc -ErrorAction SilentlyContinue
if (-not $nvcc -and $env:CUDA_PATH) {
    $cand = Join-Path $env:CUDA_PATH 'bin\nvcc.exe'
    if (Test-Path $cand) { $nvcc = Get-Command $cand }
}
if (-not $nvcc) {
    $cand = Get-ChildItem "$env:ProgramFiles\NVIDIA GPU Computing Toolkit\CUDA\v$($req.cudaToolkit)*\bin\nvcc.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($cand) { $nvcc = Get-Command $cand.FullName }
}
if (-not $nvcc) {
    Add-Check 'CUDA Toolkit' $false "nvcc introuvable (version epinglee : $($req.cudaToolkit))"
} else {
    $v = ((& $nvcc.Source --version) -join ' ')
    $m = [regex]::Match($v, 'release (\d+\.\d+), (V[\d.]+)')
    $info.NVCC = $m.Groups[2].Value
    Add-Check 'CUDA Toolkit' ($m.Success -and $m.Groups[1].Value -eq $req.cudaToolkit) `
        ("nvcc release {0} {1} (epingle : {2})" -f $m.Groups[1].Value, $m.Groups[2].Value, $req.cudaToolkit)
}

# --- 3. SDK NVIDIA (detection par fichier sentinelle, cmake/EnhNvidia.cmake) -----------------
$tools = $null
try { $tools = Enter-EnhVsEnv } catch { Add-Check 'Visual Studio / CMake' $false $_.Exception.Message }
if ($tools) {
    $sdkOut = & $tools.CMake "-DENH_NVIDIA_SDK_DIR=$env:ENH_NVIDIA_SDK_DIR" -P (Join-Path $script:RepoRoot 'cmake\EnhNvidia.cmake') 2>&1
    foreach ($l in ($sdkOut | ForEach-Object { "$_" })) {
        if ($l -match 'SDK (\S+) : OK') { Add-Check "SDK $($Matches[1])" $true 'present' }
        elseif ($l -match 'SDK (\S+) : MANQUANT \(attendu (.+)\)') {
            $rel = $Matches[2].Replace($script:RepoRoot.Replace('\', '/') + '/', '')
            Add-Check "SDK $($Matches[1])" $false "attendu : $rel"
        }
    }
}

# --- 4. FFmpeg LGPL (ADR-025) ---------------------------------------------------------------
$ffRoot = if ($env:ENH_FFMPEG_ROOT) { $env:ENH_FFMPEG_ROOT } else { Join-Path $script:RepoRoot 'third_party\ffmpeg\lgpl-shared' }
$ffOk = Test-Path (Join-Path $ffRoot 'include\libavcodec\avcodec.h')
Add-Check 'FFmpeg LGPL (shared)' $ffOk $(if ($ffOk) { 'en-tetes presents' } else { 'absent : third_party\ffmpeg\lgpl-shared\include\libavcodec\avcodec.h (voir ADR-025, scripts\fetch-ffmpeg.ps1)' })
if ($ffOk) {
    & "$PSScriptRoot\check-ffmpeg-license.ps1" -Root $ffRoot *> $null
    Add-Check 'FFmpeg : licence LGPL (check-ffmpeg-license)' ($LASTEXITCODE -eq 0) $(if ($LASTEXITCODE -eq 0) { 'aucune option GPL/nonfree' } else { 'build GPL ou nonfree refuse : voir scripts\check-ffmpeg-license.ps1' })
}

# --- 5. Disque ------------------------------------------------------------------------------
$drive = Get-PSDrive -Name ($script:RepoRoot.Substring(0, 1))
$freeGiB = [math]::Round($drive.Free / 1GB, 1)
Add-Check 'Espace disque' ($freeGiB -ge $req.minFreeDiskGiB) ("{0} Gio libres (>= {1})" -f $freeGiB, $req.minFreeDiskGiB)

$missing = @($checks | Where-Object { $_.Status -eq 'MANQUANT' })
$exit = 0
$testRows = @()
$status = 'PREREQUIS MANQUANTS'

if ($missing.Count -gt 0) {
    $exit = 2
    Write-Host ("{0} prerequis manquant(s)." -f $missing.Count) -ForegroundColor Red
} elseif (-not $CheckOnly) {
    # --- 6. Build GPU + tests CTest label gpu --------------------------------------------------
    & "$PSScriptRoot\build.ps1" -Preset $Preset
    $ctestArgs = @('--test-dir', (Join-Path $script:RepoRoot "build\$Preset"), '-L', 'gpu', '--output-on-failure', '--no-tests=error')
    if ($Story) { $ctestArgs += @('-R', $Story) }
    $out = & $tools.Ctest @ctestArgs 2>&1 | ForEach-Object { "$_" }
    $out | ForEach-Object { Write-Host $_ }
    $ctestExit = $LASTEXITCODE
    $testRows = @($out | Where-Object { $_ -match '^\s*\d+/\d+ Test\s+#\d+:' })
    $exit = if ($ctestExit -eq 0) { 0 } else { 1 }
    $status = if ($exit -eq 0) { 'VERT' } else { 'ROUGE' }
}

if ($CheckOnly) {
    Write-Host ("Verification des prerequis terminee, code de sortie {0}." -f $exit)
    exit $exit
}

# --- 7. Rapport horodate (aucun chemin personnel, aucun e-mail) ------------------------------
$resDir = Join-Path $script:RepoRoot 'docs\qa\resultats'
New-Item -ItemType Directory -Force -Path $resDir | Out-Null
$stamp = Get-Date -Format 'yyyy-MM-dd_HHmm'
$report = Join-Path $resDir ("{0}_{1}.md" -f $stamp, (Get-EnhGitBranch))
$md = New-Object System.Text.StringBuilder
[void]$md.AppendLine("# Tests GPU locaux - $stamp")
[void]$md.AppendLine('')
[void]$md.AppendLine("- Resultat : **$status** (code de sortie $exit)")
[void]$md.AppendLine("- Commit : $(Get-EnhGitHash) (branche $(Get-EnhGitBranch))")
[void]$md.AppendLine("- Preset : $Preset" + $(if ($Quick) { ' (-Quick)' } else { '' }) + $(if ($Story) { ", filtre : $Story" } else { '' }))
foreach ($k in $info.Keys) { [void]$md.AppendLine("- ${k} : $($info[$k])") }
[void]$md.AppendLine('')
[void]$md.AppendLine('## Prerequis')
[void]$md.AppendLine('')
[void]$md.AppendLine('| Element | Statut | Detail |')
[void]$md.AppendLine('|---|---|---|')
foreach ($c in $checks) { [void]$md.AppendLine("| $($c.Name) | $($c.Status) | $($c.Detail) |") }
[void]$md.AppendLine('')
[void]$md.AppendLine('## Tests (CTest, label gpu)')
[void]$md.AppendLine('')
if ($testRows.Count -gt 0) { foreach ($r in $testRows) { [void]$md.AppendLine("- $($r.Trim())") } } else { [void]$md.AppendLine('Non executes.') }
[void]$md.AppendLine('')
[void]$md.AppendLine('## Mesures')
[void]$md.AppendLine('')
# TODO (gpu-engineer / qa-tester) : boucle VRAM (10 traitements) et images/s sur REF-A. Jamais de faux vert.
[void]$md.AppendLine('- Boucle VRAM (10 traitements) : non mesuree (pas encore implementee)')
[void]$md.AppendLine('- Images/s sur REF-A : non mesuree (clip absent ou pas encore implemente)')
Set-Content -Path $report -Value $md.ToString() -Encoding UTF8
Write-Host ("Rapport : docs\qa\resultats\{0}" -f (Split-Path $report -Leaf))
exit $exit
