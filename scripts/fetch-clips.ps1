#Requires -Version 7.0
<#
.SYNOPSIS
  Recupere les clips de reference libres, verifie leur SHA-256, decoupe les extraits et genere les entrees 720p.
.DESCRIPTION
  Liste, licences et usage : docs/qa/clips.md ; donnees : tests/quality/clips.psd1.
  Sortie (ignoree par git) : tests/clips/{src,ref4k,in720,po}. Les clips ne sont JAMAIS commites.
  - Telechargement HTTPS seulement, hotes de la liste blanche de clips.psd1, via curl.exe (reprise possible), sans identifiant ni parametre de suivi.
  - SHA-256 : si la somme est connue, elle est verifiee (echec si differente). Si elle est vide, la somme observee est
    AFFICHEE et ecrite dans tests/clips/sha256-observed.txt, et le script s'arrete avec le code 3 sauf -AcceptNewHash
    (a consigner ensuite dans clips.psd1 et docs/qa/clips.md apres relecture).
  - Necessite le FFmpeg GPL de MESURE (hors depot, -FfmpegDir) avec libx264 pour le jeu B.
  NON EXECUTE a la redaction : ne lancer qu'apres validation de la liste par le PO et avis secdevops sur les licences.
.PARAMETER FfmpegDir   Dossier du FFmpeg de mesure (ffmpeg.exe, ffprobe.exe).
.PARAMETER Only        Identifiants de clips a traiter (ex. BBB-FAST,TOS-FACE). Par defaut : tous.
.PARAMETER SkipDownload  N'essaie pas de telecharger : n'utilise que tests/clips/src deja present.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$FfmpegDir,
    [string[]]$Only,
    [string]$Root,
    [switch]$SkipDownload,
    [switch]$AcceptNewHash,
    [switch]$Force
)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $repo 'tests\quality\QaCommon.ps1')
if (-not $Root) { $Root = Join-Path $repo 'tests\clips' }
$table = Get-ClipTable
$tools = Resolve-FfmpegTools -FfmpegDir $FfmpegDir -NeedX264

# Garde-fou : la destination doit etre ignoree par git (jamais de clip commite)
$git = Get-Command git -ErrorAction SilentlyContinue
if ($git) {
    New-Item -ItemType Directory -Path $Root -Force | Out-Null
    $chk = Invoke-Native $git.Source @('-C', $repo, 'check-ignore', '-q', (Join-Path $Root 'x.mkv'))
    if ($chk.ExitCode -ne 0) { throw "tests/clips/ n'est pas ignore par git (.gitignore) : arret par securite." }
}
foreach ($d in 'src', 'ref4k', 'in720', 'po', '_tmp') { New-Item -ItemType Directory -Path (Join-Path $Root $d) -Force | Out-Null }

$unknownHashes = [System.Collections.Generic.List[string]]::new()

function Get-Source([hashtable]$Src) {
    # Archive : le serveur sert un .zip ; le SHA-256 consigne est celui de l'archive telle que servie.
    $dest = Join-Path $Root "src\$($Src.FileName)"
    $dl = if ($Src.Archive) { Join-Path $Root "src\$($Src.Archive)" } else { $dest }
    if (-not (Test-Path $dl) -and -not (Test-Path $dest)) {
        if ($SkipDownload) { throw "Source absente : $($Src.FileName) (-SkipDownload)." }
        if (-not $Src.Url) { throw "URL non renseignee pour $($Src.SourceId) dans clips.psd1 (a confirmer, voir docs/qa/clips.md)." }
        $uri = [uri]$Src.Url
        if ($uri.Scheme -ne 'https') { throw "Seul HTTPS est autorise ($($Src.SourceId))." }
        if ($table.Hosts -notcontains $uri.Host) { throw "Hote non autorise : $($uri.Host)." }
        if ($uri.Query) { throw "URL avec parametres refusee (pas de suivi) : $($Src.SourceId)." }
        Write-Host "Telechargement : $($uri.Segments[-1]) depuis $($uri.Host)"
        $part = "$dl.part"
        & curl.exe --location --fail --silent --show-error --proto '=https' --proto-redir '=https' --continue-at - --output $part $Src.Url
        if ($LASTEXITCODE -ne 0) { throw "Echec du telechargement de $($Src.SourceId) (curl code $LASTEXITCODE)." }
        Move-Item $part $dl
    }
    if (Test-Path $dl) {
        $sha = (Get-FileHash -Algorithm SHA256 -Path $dl).Hash.ToLowerInvariant()
        if ($Src.Sha256) {
            if ($sha -ne $Src.Sha256.ToLowerInvariant()) { throw "SHA-256 different pour $(Split-Path $dl -Leaf) : attendu $($Src.Sha256), obtenu $sha. Fichier laisse en place pour examen." }
            Write-Host "SHA-256 verifie : $($Src.SourceId)"
        } else {
            Write-Warning "SHA-256 inconnu pour $($Src.SourceId) : observe $sha (a consigner)."
            Add-Content -Path (Join-Path $Root 'sha256-observed.txt') -Value "$($Src.SourceId) $(Split-Path $dl -Leaf) $sha"
            $unknownHashes.Add($Src.SourceId)
        }
    }
    if ($Src.Archive -and -not (Test-Path $dest)) {
        # extraction du seul membre attendu (nom exact, pas de chemin : protection contre zip-slip)
        Add-Type -AssemblyName System.IO.Compression.FileSystem
        $zip = [System.IO.Compression.ZipFile]::OpenRead($dl)
        try {
            $entry = $zip.Entries | Where-Object { $_.Name -eq $Src.FileName -and $_.FullName -eq $_.Name } | Select-Object -First 1
            if (-not $entry) { throw "Membre '$($Src.FileName)' absent de l'archive ($(($zip.Entries | ForEach-Object FullName) -join ', '))." }
            [System.IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $dest, $true)
        } finally { $zip.Dispose() }
    }
    return $dest
}
function Invoke-Ffmpeg([string[]]$FfArgs) {
    $r = Invoke-Native $tools.Ffmpeg (@('-hide_banner', '-nostats', '-loglevel', 'error', '-y') + $FfArgs)
    if ($r.ExitCode -ne 0) { throw "ffmpeg a echoue (code $($r.ExitCode)) : $($r.StdErr.Trim())" }
}

$colorTags = @('-colorspace', 'bt709', '-color_primaries', 'bt709', '-color_trc', 'bt709', '-color_range', 'tv')

function New-Ref4k([hashtable]$Clip, [string]$SrcFile, [string]$RefOut) {
    $info = Get-VideoInfo $tools $SrcFile
    $pix = if ($info.pix_fmt -match '10') { 'yuv420p10le' } else { 'yuv420p' }
    # Normalisation : largeur 3840, hauteur selon le ratio d'origine (paire), SANS bandes noires ajoutees :
    # 16:9 -> 3840x2160 ; film 2,39:1 (Tears of Steel) -> 3840x1710. Des bandes noires gonfleraient les scores (zones identiques).
    # Hauteur finale multiple de 6 (donc 720p = hauteur/3 paire) : 1714 -> recadrage de 2 lignes en haut et en bas (Tears of Steel, 3840x1714 d'origine).
    $hs = [int][math]::Round(3840.0 * [int]$info.height / [int]$info.width)
    $h = 6 * [int][math]::Floor($hs / 6)
    $vf = "scale=3840:${hs}:flags=lanczos,crop=3840:${h},setsar=1,format=$pix"
    Invoke-Ffmpeg (@('-ss', "$($Clip.Start)", '-i', $SrcFile, '-t', "$($Clip.Duration)", '-map', '0:v:0', '-an', '-sn', '-vf', $vf,
            '-c:v', 'ffv1', '-level', '3', '-coder', '1', '-context', '1', '-slices', '24', '-slicecrc', '1', '-fps_mode', 'passthrough') + $colorTags + @($RefOut))
}

function New-In720([hashtable]$Clip, [string]$RefFile) {
    $ref = Get-VideoInfo $tools $RefFile
    $pix = $ref.pix_fmt
    $chain = "scale=1280:$([int]($ref.height) / 3):flags=lanczos"
    $rate = @()
    if ($Clip.Mode -eq 'Interp60') {
        # on retire une image sur deux, sans melange : l'image n et n+1 deviennent l'image n/2
        $chain += ",select='not(mod(n,2))',setpts=N/30/TB"; $rate = @('-r', '30')
    }
    $outA = Join-Path $Root "in720\$($Clip.Id).mkv"
    Invoke-Ffmpeg (@('-i', $RefFile, '-vf', "$chain,format=$pix", '-an', '-c:v', 'ffv1', '-level', '3', '-slices', '24') + $(if ($rate.Count) { $rate } else { @('-fps_mode', 'passthrough') }) + $colorTags + @($outA))
    # Jeu B : H.264 8 bits, CRF 12 (ce que NVIDIA Broadcast sait lire)
    $outB = Join-Path $Root "in720\$($Clip.Id).mp4"
    Invoke-Ffmpeg (@('-i', $outA, '-an', '-vf', 'format=yuv420p', '-c:v', 'libx264', '-preset', 'slow', '-crf', '12', '-movflags', '+faststart', '-fps_mode', 'passthrough') + $colorTags + @($outB))
}

function New-Synthetic([hashtable]$Clip, [string]$RefOut) {
    # Texte fin defilant + grille de lignes de 1 px sur fond sombre : proxy de l'interface d'un jeu et du texte de sous-titres.
    $tmp = Join-Path $Root '_tmp'
    $lines = 1..140 | ForEach-Object { 'Ligne ' + $_.ToString('D3') + '  Enhance - ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz 0123456789 .,;:!? ()[]{}<>/\|+-=*' }
    Set-Content -Path (Join-Path $tmp 'text.txt') -Value ($lines -join "`n") -Encoding ascii
    $fontPath = 'C\:/Windows/Fonts/segoeui.ttf'
    if (-not (Test-Path 'C:\Windows\Fonts\segoeui.ttf')) { throw 'Police Segoe UI absente : SYN-TEXT non generable ici.' }
    $g = @"
color=c=0x181818:s=3840x2160:r=60:d=$($Clip.Duration),drawgrid=w=40:h=40:t=1:c=white@0.12,
drawtext=fontfile='${fontPath}':textfile=text.txt:fontsize=26:fontcolor=white:x=60:y=h-mod(t*180\,h+3600),
drawtext=fontfile='${fontPath}':textfile=text.txt:fontsize=12:fontcolor=0xDDDDDD:x=2000:y=h-mod(t*120\,h+1700),
format=yuv420p
"@
    Set-Content -Path (Join-Path $tmp 'syn.filter') -Value ($g -replace "`r?`n", '') -Encoding ascii
    $r = Invoke-Native $tools.Ffmpeg (@('-hide_banner', '-nostats', '-loglevel', 'error', '-y', '-/filter_complex', 'syn.filter', '-c:v', 'ffv1', '-level', '3', '-slices', '24') + $colorTags + @($RefOut)) -WorkDir $tmp
    if ($r.ExitCode -ne 0) { throw "Generation de SYN-TEXT echouee : $($r.StdErr.Trim())" }
}

# --- Boucle principale --------------------------------------------------------------------------------
$todo = $table.Clips | Where-Object { -not $Only -or $Only -contains $_.Id }
$srcCache = @{}
foreach ($clip in $todo) {
    Write-Host "== $($clip.Id) ($($clip.Kind)) =="
    $refOut = Join-Path $Root "ref4k\$($clip.Id).mkv"
    switch ($clip.Kind) {
        'Download' {
            if ((Test-Path $refOut) -and -not $Force) { Write-Host 'ref4k deja present'; }
            else {
                if (-not $srcCache.ContainsKey($clip.SourceId)) {
                    $src = $table.Sources | Where-Object { $_.SourceId -eq $clip.SourceId }
                    $srcCache[$clip.SourceId] = Get-Source $src
                }
                New-Ref4k $clip $srcCache[$clip.SourceId] $refOut
            }
            if (-not (Test-Path (Join-Path $Root "in720\$($clip.Id).mp4")) -or $Force) { New-In720 $clip $refOut }
        }
        'Synthetic' {
            if (-not (Test-Path $refOut) -or $Force) { New-Synthetic $clip $refOut }
            if (-not (Test-Path (Join-Path $Root "in720\$($clip.Id).mp4")) -or $Force) { New-In720 $clip $refOut }
        }
        'PO' {
            $f = Get-ChildItem (Join-Path $Root 'po') -Filter "$($clip.Id).*" -File -ErrorAction SilentlyContinue | Select-Object -First 1
            if ($f) {
                $sha = (Get-FileHash -Algorithm SHA256 -Path $f.FullName).Hash.ToLowerInvariant()
                $log = Join-Path $Root 'po\SHA256.txt'
                $prev = if (Test-Path $log) { Get-Content $log | Where-Object { $_ -like "$($clip.Id) *" } } else { $null }
                if (-not $prev) { Add-Content -Path $log -Value "$($clip.Id) $sha" }
                elseif ($prev -notmatch $sha) { Write-Warning "$($clip.Id) : l'extrait a change depuis la premiere fois (SHA-256 different)." }
            } else { Write-Warning "$($clip.Id) : a copier par le PO dans tests/clips/po/ (voir docs/qa/clips.md section 3)." }
        }
    }
}
Remove-Item (Join-Path $Root '_tmp') -Recurse -Force -ErrorAction SilentlyContinue

if ($unknownHashes.Count -and -not $AcceptNewHash) {
    Write-Warning "Sommes SHA-256 a consigner (tests/clips/sha256-observed.txt) puis a reporter dans tests/quality/clips.psd1 et docs/qa/clips.md : $($unknownHashes -join ', ')"
    exit 3
}
Write-Host 'Termine.'




