# Fonctions communes des scripts QA (a charger avec : . "$PSScriptRoot\QaCommon.ps1").
# ASCII seulement dans ce fichier (compatibilite PowerShell). Aucune donnee personnelle ecrite dans les sorties.
Set-StrictMode -Version Latest

function Get-RepoRoot {
    # tests/quality -> racine du depot
    return (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
}

function Invoke-Native {
    # Lance un executable, capture stdout / stderr sans passer par le shell (pas de probleme de quoting).
    param(
        [Parameter(Mandatory)][string]$Exe,
        [string[]]$Arguments = @(),
        [string]$WorkDir
    )
    $psi = [System.Diagnostics.ProcessStartInfo]::new($Exe)
    foreach ($a in $Arguments) { $psi.ArgumentList.Add($a) }
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.UseShellExecute = $false
    $psi.CreateNoWindow = $true
    if ($WorkDir) { $psi.WorkingDirectory = $WorkDir }
    $p = [System.Diagnostics.Process]::Start($psi)
    $outTask = $p.StandardOutput.ReadToEndAsync()
    $errTask = $p.StandardError.ReadToEndAsync()
    $p.WaitForExit()
    return [pscustomobject]@{ ExitCode = $p.ExitCode; StdOut = $outTask.Result; StdErr = $errTask.Result }
}

function Resolve-FfmpegTools {
    # FfmpegDir : dossier du FFmpeg GPL de MESURE (hors depot), contenant ffmpeg.exe et ffprobe.exe (directement ou dans bin\).
    param([Parameter(Mandatory)][string]$FfmpegDir, [switch]$NeedVmaf, [switch]$NeedX264)
    $cands = @($FfmpegDir, (Join-Path $FfmpegDir 'bin'))
    $dir = $cands | Where-Object { Test-Path (Join-Path $_ 'ffmpeg.exe') } | Select-Object -First 1
    if (-not $dir) { throw "ffmpeg.exe introuvable dans '$FfmpegDir' (ni dans bin\)." }
    $ff = Join-Path $dir 'ffmpeg.exe'
    $fp = Join-Path $dir 'ffprobe.exe'
    if (-not (Test-Path $fp)) { throw "ffprobe.exe introuvable a cote de ffmpeg.exe." }
    $v = Invoke-Native $ff @('-hide_banner', '-version')
    $versionLine = ($v.StdOut -split "`r?`n")[0]
    $hasVmaf = $v.StdOut -match '--enable-libvmaf'
    $hasX264 = $v.StdOut -match '--enable-libx264'
    if ($NeedVmaf -and -not $hasVmaf) { throw "Ce FFmpeg n'est pas compile avec libvmaf (--enable-libvmaf absent)." }
    if ($NeedX264 -and -not $hasX264) { throw "Ce FFmpeg n'est pas compile avec libx264 (necessaire au jeu B)." }
    return [pscustomobject]@{ Ffmpeg = $ff; Ffprobe = $fp; Version = $versionLine; Libvmaf = $hasVmaf; Libx264 = $hasX264 }
}

function Get-VideoInfo {
    # Informations du premier flux video + nombre d'images lues (par paquets : rapide, sans decoder).
    param([Parameter(Mandatory)]$Tools, [Parameter(Mandatory)][string]$File)
    $r = Invoke-Native $Tools.Ffprobe @('-v', 'error', '-select_streams', 'v:0', '-count_packets',
        '-show_entries', 'stream=codec_name,profile,pix_fmt,width,height,r_frame_rate,avg_frame_rate,nb_read_packets,start_time,duration,color_range,color_space,color_transfer,color_primaries',
        '-of', 'json', $File)
    if ($r.ExitCode -ne 0) { throw "ffprobe a echoue sur un fichier (code $($r.ExitCode)) : $($r.StdErr.Trim())" }
    $s = ($r.StdOut | ConvertFrom-Json).streams[0]
    $s | Add-Member -NotePropertyName frames -NotePropertyValue ([int]$s.nb_read_packets) -Force
    return $s
}

function ConvertTo-Rate {
    param([string]$Text)
    if ($Text -match '^(\d+)/(\d+)$' -and [double]$Matches[2] -ne 0) { return [double]$Matches[1] / [double]$Matches[2] }
    return 0.0
}

function Get-MachineInfo {
    # Sans nom de machine, sans nom d'utilisateur.
    $gpu = 'inconnu'; $driver = 'inconnu'; $vram = 0
    $smi = Get-Command nvidia-smi -ErrorAction SilentlyContinue
    if ($smi) {
        $o = Invoke-Native $smi.Source @('--query-gpu=name,driver_version,memory.total', '--format=csv,noheader,nounits')
        if ($o.ExitCode -eq 0) {
            $f = ($o.StdOut.Trim() -split ',') | ForEach-Object { $_.Trim() }
            $gpu = $f[0]; $driver = $f[1]; $vram = [int]$f[2]
        }
    }
    $os = [System.Environment]::OSVersion.Version.ToString()
    return [ordered]@{ gpu = $gpu; driver = $driver; vram_total_mib = $vram; windows = $os }
}

function Get-GitHashSafe {
    $git = Get-Command git -ErrorAction SilentlyContinue
    if (-not $git) { return 'inconnu' }
    $o = Invoke-Native $git.Source @('-C', (Get-RepoRoot), 'rev-parse', '--short', 'HEAD')
    if ($o.ExitCode -eq 0) { return $o.StdOut.Trim() } else { return 'inconnu' }
}

function Get-ClipTable {
    return Import-PowerShellDataFile (Join-Path $PSScriptRoot 'clips.psd1')
}

function Get-Percentile {
    param([double[]]$Values, [double]$P)
    if (-not $Values -or $Values.Count -eq 0) { return $null }
    $sorted = $Values | Sort-Object
    $idx = [math]::Max(0, [math]::Min($sorted.Count - 1, [math]::Floor(($P / 100.0) * ($sorted.Count - 1))))
    return [double]$sorted[$idx]
}

function Write-JsonFile {
    param([Parameter(Mandatory)]$Object, [Parameter(Mandatory)][string]$Path)
    $dir = Split-Path $Path -Parent
    if ($dir -and -not (Test-Path $dir)) { New-Item -ItemType Directory -Path $dir -Force | Out-Null }
    $Object | ConvertTo-Json -Depth 8 | Set-Content -Path $Path -Encoding utf8
}
