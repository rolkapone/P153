<#
.SYNOPSIS
  Preuve "zero reseau" cote build (ADR-027, docs/security/zero-reseau.md, schema v1 de
  scripts/network-allowlist.json).
.DESCRIPTION
  C1 (sources)  : motifs interdits dans src/, CMakeLists.txt, cmake/, QML, vcpkg.json (tests/ exclus).
  C2 (fichiers) : DLL/plugins interdits dans le dossier deploye.
  C3 (imports)  : dumpbin /imports (imports normaux ET differes) ; DLL reseau interdites comparees
                  AU NIVEAU DES FONCTIONS aux exceptions de la liste blanche (avec expiration).
  C4 (chaines)  : binaires d'Enhance : URL, chemins personnels, chemin de build, e-mail ;
                  table de certificats Authenticode exclue.
  Code de sortie : 0 = OK, 1 = violation, 2 = outil manquant / entree invalide.
.PARAMETER Path
  Dossier deploye a analyser (defaut : build\ci\bin s'il existe, sinon build\ci).
.PARAMETER SourcesOnly
  Ne fait que C1 (aucun outil MSVC requis ; utilisable en CI Linux avec PowerShell 7).
.PARAMETER Package
  Mode livrable : les exceptions marquees scope=dev-only ne sont PAS acceptees.
.PARAMETER Report
  Ecrit un rapport Markdown (chemins relatifs, sans information personnelle).
.PARAMETER ThirdPartyStrings
  Liste aussi les URL ASCII des binaires tiers (lent, rapport seulement).
#>
[CmdletBinding()]
param(
    [string]$Path = '',
    [switch]$SourcesOnly,
    [switch]$Package,
    [string]$Report = '',
    [switch]$ThirdPartyStrings
)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/common.ps1"

$repo = $script:RepoRoot
$allowFile = Join-Path $PSScriptRoot 'network-allowlist.json'
if (-not (Test-Path $allowFile)) { Write-Host "Liste blanche introuvable : scripts/network-allowlist.json"; exit 2 }
$allow = Get-Content $allowFile -Raw | ConvertFrom-Json
if ($allow.schemaVersion -ne 1) { Write-Host "Schema de liste blanche inattendu : $($allow.schemaVersion)"; exit 2 }

$violations = New-Object System.Collections.Generic.List[string]
$warnings = New-Object System.Collections.Generic.List[string]
$notes = New-Object System.Collections.Generic.List[string]
$today = (Get-Date).Date

function Get-Rel([string]$full, [string]$root) {
    return ([IO.Path]::GetRelativePath($root, $full)) -replace '\\', '/'
}

# ----------------------------------------------------------------------------------------------
# C1 - sources
# ----------------------------------------------------------------------------------------------
function Invoke-SourceScan {
    $rules = @(
        @{ Id = 'header';  Re = '(?i)(winsock2?\.h|ws2tcpip\.h|mswsock\.h|winhttp\.h|wininet\.h|urlmon\.h|iphlpapi\.h|windns\.h|websocket\.h|#\s*include\s*[<"]http\.h[>"])' },
        @{ Id = 'pragma';  Re = '(?i)#\s*pragma\s+comment\s*\(\s*lib\s*,\s*"(ws2_32|wsock32|winhttp|wininet|urlmon|mswsock)' },
        @{ Id = 'func';    Re = '\b(WSAStartup|getaddrinfo|gethostbyname|InternetOpenUrl|InternetOpen|HttpSendRequest|WinHttpOpen|URLDownloadToFile|URLOpenStream|IBackgroundCopyManager|NVSDK_NGX_UpdateFeature|avformat_network_init|avio_open2?)\b' },
        @{ Id = 'sock';    Re = '(?<![\w.>:])(::)?socket\s*\(' },
        @{ Id = 'connect'; Re = '(?<![\w.>:])::connect\s*\(' },
        @{ Id = 'qt';      Re = '(?i)(\bQNetwork\w*|\bQTcpSocket\b|\bQUdpSocket\b|\bQSslSocket\b|\bQWebSocket\b|\bQDnsLookup\b|\bQHostInfo\b|\bXMLHttpRequest\b|Qt\.openUrlExternally|import\s+QtNetwork|import\s+QtWebEngine|import\s+QtWebView)' },
        @{ Id = 'qmlurl';  Re = '(?i)source\s*:\s*"https?:' }
    )
    $cmakeRules = @(
        @{ Id = 'cmake-lib'; Re = '(?i)(\bws2_32\b|\bwinhttp\b|\bwininet\b|\burlmon\b|Qt6?::Network|CURL::|cpprest)' },
        @{ Id = 'cmake-qt';  Re = '(?i)find_package\s*\(\s*Qt6[^)]*\bNetwork\b' }
    )
    $urlRe = '"https?://'

    $files = New-Object System.Collections.Generic.List[IO.FileInfo]
    foreach ($d in 'src', 'cmake') {
        $p = Join-Path $repo $d
        if (Test-Path $p) { Get-ChildItem $p -Recurse -File | ForEach-Object { $files.Add($_) } }
    }
    $cm = Join-Path $repo 'CMakeLists.txt'
    if (Test-Path $cm) { $files.Add((Get-Item $cm)) }

    $urlExc = @($allow.sourceUrlExceptions)
    $n = 0
    foreach ($f in $files) {
        $rel = Get-Rel $f.FullName $repo
        $ext = $f.Extension.ToLowerInvariant()
        $isCode = $ext -in '.cpp', '.hpp', '.h', '.cc', '.cxx', '.c', '.cu', '.cuh', '.qml', '.js', '.inl', '.ixx'
        $isCmake = ($f.Name -eq 'CMakeLists.txt') -or $ext -in '.cmake', '.in'
        if (-not ($isCode -or $isCmake)) { continue }
        $n++
        $lines = Get-Content -LiteralPath $f.FullName
        for ($i = 0; $i -lt $lines.Count; $i++) {
            $line = $lines[$i]
            if ($line -match 'enh-network-allow:\s*SEC-\d+') { continue }
            if ($isCmake) {
                $code = ($line -replace '#.*$', '')
                foreach ($r in $cmakeRules) {
                    if ($code -match $r.Re) { $violations.Add("C1 $rel`:$($i + 1) [$($r.Id)] $($Matches[0])") }
                }
            }
            if ($isCode) {
                foreach ($r in $rules) {
                    if ($line -match $r.Re) { $violations.Add("C1 $rel`:$($i + 1) [$($r.Id)] $($Matches[0])") }
                }
                if ($ext -ne '.qml' -and $line -match $urlRe -and ($urlExc -notcontains $rel)) {
                    $violations.Add("C1 $rel`:$($i + 1) [url] chaine http(s) dans le code")
                }
            }
        }
    }
    # vcpkg.json : ports et features interdits.
    $vj = Join-Path $repo 'vcpkg.json'
    if (Test-Path $vj) {
        $m = Get-Content $vj -Raw | ConvertFrom-Json
        $badPorts = 'qtnetworkauth', 'qtwebengine', 'qtwebview', 'qtwebsockets', 'qthttpserver', 'curl', 'cpprestsdk', 'openssl'
        $badFeat = @{ 'qtbase' = @('network', 'openssl'); 'ffmpeg' = @('gpl', 'nonfree', 'openssl', 'x264', 'x265', 'fdk-aac') }
        $deps = @($m.dependencies)
        foreach ($prop in $m.features.PSObject.Properties) { $deps += @($prop.Value.dependencies) }
        foreach ($d in $deps) {
            if ($null -eq $d) { continue }
            $name = if ($d -is [string]) { $d } else { $d.name }
            if ($badPorts -contains $name) { $violations.Add("C1 vcpkg.json : port interdit '$name'") }
            if ($d -isnot [string] -and $d.PSObject.Properties['features'] -and $badFeat.ContainsKey($name)) {
                foreach ($ft in @($d.features)) {
                    if ($badFeat[$name] -contains $ft) { $violations.Add("C1 vcpkg.json : feature interdite $name[$ft]") }
                }
            }
        }
        $n++
    }
    # Overlay ports : aucun qtbase[network] reintroduit.
    $ov = Join-Path $repo 'vcpkg/overlay-ports'
    if (Test-Path $ov) {
        Get-ChildItem $ov -Recurse -Filter vcpkg.json | ForEach-Object {
            $o = Get-Content $_.FullName -Raw | ConvertFrom-Json
            foreach ($d in @($o.dependencies)) {
                if ($d -isnot [string] -and $d.name -eq 'qtbase' -and $d.PSObject.Properties['features'] -and (@($d.features) -contains 'network')) {
                    $violations.Add("C1 overlay $((Get-Rel $_.FullName $repo)) : depend de qtbase[network]")
                }
            }
        }
    }
    # Exigence positive : le module qui appelle avformat_open_input doit imposer protocol_whitelist,
    # remplacer io_open et filtrer les demuxeurs (isAllowedDemuxer). Echec si absent ou introuvable.
    $ff = Join-Path $repo 'src/pipeline/ffmpeg'
    $ffSources = @()
    if (Test-Path $ff) { $ffSources = @(Get-ChildItem $ff -Recurse -File | Where-Object { $_.Extension -in '.cpp', '.hpp', '.h' }) }
    $allFf = ($ffSources | ForEach-Object { Get-Content -LiteralPath $_.FullName -Raw }) -join "`n"
    if ($allFf -match 'avformat_open_input') {
        foreach ($req in 'protocol_whitelist', 'io_open', 'isAllowedDemuxer') {
            if ($allFf -notmatch [regex]::Escape($req)) { $violations.Add("C1 src/pipeline/ffmpeg : $req absent") }
        }
    } else {
        $violations.Add('C1 src/pipeline/ffmpeg : appel a avformat_open_input introuvable, exigences protocol_whitelist / io_open / isAllowedDemuxer non verifiables')
    }
    $notes.Add("C1 : $n fichier(s) de sources analyses.")
}

# ----------------------------------------------------------------------------------------------
# C2/C3/C4 - binaires
# ----------------------------------------------------------------------------------------------
function Get-PeImports([string]$file) {
    # Retourne une table : dll (minuscules) -> liste de fonctions ("#N" pour un ordinal).
    $out = @{}
    $lines = & dumpbin /nologo /imports $file 2>&1
    $cur = $null
    foreach ($l in $lines) {
        $s = [string]$l
        if ($s -match '^\s{4}(\S+\.(dll|DLL|exe|EXE|drv|ocx|sys))\s*$') {
            $cur = $Matches[1].ToLowerInvariant()
            if (-not $out.ContainsKey($cur)) { $out[$cur] = New-Object System.Collections.Generic.List[string] }
            continue
        }
        if ($null -eq $cur) { continue }
        if ($s -match '^\s{6,}Ordinal\s+(\d+)\s*$') { $out[$cur].Add('#' + $Matches[1]); continue }
        if ($s -match '^\s{10,}[0-9A-Fa-f]{1,4}\s+(\S+)\s*$') {
            if ($Matches[1] -notmatch '^(Import|Index|time|Characteristics)$') { $out[$cur].Add($Matches[1]) }
        }
    }
    return $out
}

$script:ordinalCache = @{}
function Resolve-Ordinal([string]$dll, [string]$fn) {
    # "#115" -> "WSAStartup" via dumpbin /exports de la DLL systeme ; sinon inchange.
    if ($fn -notmatch '^#(\d+)$') { return $fn }
    $ord = [int]$Matches[1]
    if (-not $script:ordinalCache.ContainsKey($dll)) {
        $map = @{}
        $sys = Join-Path ([Environment]::GetFolderPath('System')) $dll
        if (Test-Path $sys) {
            foreach ($l in (& dumpbin /nologo /exports $sys 2>&1)) {
                if ("$l" -match '^\s+(\d+)\s+[0-9A-Fa-f]+\s+[0-9A-Fa-f]{8}\s+(\S+)') { $map[[int]$Matches[1]] = $Matches[2] }
            }
        }
        $script:ordinalCache[$dll] = $map
    }
    $m = $script:ordinalCache[$dll]
    if ($m.ContainsKey($ord)) { return $m[$ord] }
    return $fn
}

function Get-PeSecurityRange([byte[]]$b) {
    # Plage (offset, taille) de la table de certificats Authenticode, ou $null.
    if ($b.Length -lt 0x100) { return $null }
    $pe = [BitConverter]::ToInt32($b, 0x3C)
    if ($pe -lt 0 -or $pe + 0x100 -gt $b.Length) { return $null }
    $magic = [BitConverter]::ToUInt16($b, $pe + 24)
    $ddOff = if ($magic -eq 0x20B) { $pe + 24 + 112 } elseif ($magic -eq 0x10B) { $pe + 24 + 96 } else { return $null }
    $secOff = [BitConverter]::ToUInt32($b, $ddOff + 4 * 8)
    $secSize = [BitConverter]::ToUInt32($b, $ddOff + 4 * 8 + 4)
    if ($secOff -eq 0 -or $secSize -eq 0 -or ([long]$secOff + $secSize) -gt $b.Length) { return $null }
    return @([int]$secOff, [int]$secSize)
}

function Test-ExceptionActive($ex) {
    # $true si l'exception est utilisable ; ajoute un avertissement/violation sinon.
    if ($Package -and $ex.PSObject.Properties['scope'] -and $ex.scope -eq 'dev-only') { return $false }
    if ($ex.PSObject.Properties['expires'] -and $ex.expires) {
        $d = [datetime]::MinValue
        if ([datetime]::TryParseExact([string]$ex.expires, 'yyyy-MM-dd', $null, 'None', [ref]$d)) {
            if ($d.Date -lt $today) { return $false }
        } else {
            return $false   # date illisible : exception inutilisable, signalee comme violation par l'appelant
        }
    }
    return $true
}

function Invoke-BinaryScan([string]$dir) {
    $dumpbin = Get-Command dumpbin -ErrorAction SilentlyContinue
    if (-not $dumpbin) { Write-Host 'dumpbin introuvable (environnement MSVC).'; exit 2 }
    $bins = @(Get-ChildItem $dir -Recurse -File | Where-Object { $_.Extension -in '.exe', '.dll' })
    $allFiles = @(Get-ChildItem $dir -Recurse -File)

    # Exceptions : expirees ou hors portee = erreurs signalees une fois.
    foreach ($ex in @($allow.exceptions)) {
        if ($Package -and $ex.PSObject.Properties['scope'] -and $ex.scope -eq 'dev-only') {
            $notes.Add("Exception $($ex.id) (dev-only) ignoree en mode -Package.")
        } elseif (-not (Test-ExceptionActive $ex)) {
            $dd = [datetime]::MinValue
            if (-not [datetime]::TryParseExact([string]$ex.expires, 'yyyy-MM-dd', $null, 'None', [ref]$dd)) {
                $violations.Add("Exception $($ex.id) : date 'expires' illisible ('$($ex.expires)', format attendu yyyy-MM-dd).")
            } else {
                $violations.Add("Exception $($ex.id) expiree le $($ex.expires) : a re-valider par secdevops.")
            }
        } elseif ($ex.expires) {
            $notes.Add("Exception $($ex.id) : echeance '$($ex.expires)'.")
        }
    }

    # C2 : fichiers interdits.
    foreach ($f in $allFiles) {
        $rel = Get-Rel $f.FullName $dir
        foreach ($pat in @($allow.forbiddenFiles)) {
            $hit = if ($pat.Contains('/')) { $rel -like "*$pat" } else { $f.Name -like $pat }
            if ($hit) { $violations.Add("C2 fichier interdit : $rel (motif $pat)"); break }
        }
    }

    $forbidden = @($allow.forbiddenImports | ForEach-Object { $_.ToLowerInvariant() })
    $warnList = @($allow.warnImports | ForEach-Object { $_.ToLowerInvariant() })
    $ownPats = @($allow.stringScan.ownBinaries)
    $failPats = @($allow.stringScan.ownBinaryFailPatterns)
    $urlExc = @($allow.stringScan.ownBinaryUrlExceptions)
    $emailOk = '81617903+rolkapone@users.noreply.github.com'
    $warnSeen = @{}
    $summary = New-Object System.Collections.Generic.List[string]

    foreach ($b in $bins) {
        $rel = Get-Rel $b.FullName $dir
        $imps = Get-PeImports $b.FullName
        foreach ($dll in $imps.Keys) {
            $funcs = @($imps[$dll] | ForEach-Object { if ($forbidden -contains $dll) { Resolve-Ordinal $dll $_ } else { $_ } })
            if ($warnList -contains $dll) {
                $k = "$($b.Name) -> $dll"
                if (-not $warnSeen.ContainsKey($k)) { $warnSeen[$k] = $true; $warnings.Add("C3 import a signaler : $rel -> $dll ($($funcs.Count) fonction(s))") }
            }
            if ($forbidden -notcontains $dll) { continue }
            # Exceptions applicables a ce module + cette DLL.
            $excs = @($allow.exceptions | Where-Object { ($b.Name -like $_.module) -and ($_.import.ToLowerInvariant() -eq $dll) -and (Test-ExceptionActive $_) })
            $bad = New-Object System.Collections.Generic.List[string]
            foreach ($fn in $funcs) {
                $okFn = $false
                foreach ($ex in $excs) {
                    $lst = @($ex.allowedFunctions)
                    if ($lst -contains '*' -or $lst -contains $fn) { $okFn = $true; break }
                }
                if (-not $okFn) { $bad.Add($fn) }
            }
            if ($funcs.Count -eq 0) { $bad.Add('(import sans fonction lisible)') }
            if ($bad.Count -gt 0) {
                $shown = ($bad | Select-Object -First 8) -join ', '
                $more = if ($bad.Count -gt 8) { " ... (+$($bad.Count - 8))" } else { '' }
                $violations.Add("C3 import reseau non autorise : $rel -> $dll : $shown$more")
            } else {
                $summary.Add("$rel -> $dll : $($funcs.Count) fonction(s), couvertes par une exception")
            }
        }

        # C4 : binaires d'Enhance.
        $isOwn = $false
        foreach ($p in $ownPats) { if ($b.Name -like $p) { $isOwn = $true } }
        if ($isOwn) {
            $bytes = [IO.File]::ReadAllBytes($b.FullName)
            $r = Get-PeSecurityRange $bytes
            $len = $bytes.Length
            if ($null -ne $r) {
                # Zero les octets du certificat (n'affecte pas l'analyse, plus rapide que de decouper).
                [Array]::Clear($bytes, $r[0], $r[1])
            }
            $ascii = [Text.Encoding]::GetEncoding(28591).GetString($bytes)
            $u0 = [Text.Encoding]::Unicode.GetString($bytes, 0, $len - ($len % 2))
            $u1 = [Text.Encoding]::Unicode.GetString($bytes, 1, ($len - 1) - (($len - 1) % 2))
            # Toutes les occurrences sont examinees (pas seulement la premiere) ; une exception ne
            # couvre qu'une URL COMPLETE identique (egalite exacte, sans joker), cf. SEC-26.
            $urlOk = @($urlExc | ForEach-Object { if ($_ -is [string]) { $_ } else { $_.url } })
            foreach ($pat in $failPats) {
                $hit = $false
                $isUrl = $pat.StartsWith('http')
                foreach ($txt in $ascii, $u0, $u1) {
                    foreach ($mm in [regex]::Matches($txt, $pat)) {
                        $win = $txt.Substring($mm.Index, [Math]::Min(512, $txt.Length - $mm.Index))
                        $tok = [regex]::Match($win, '^[^\s"''<>\x00]*').Value
                        if ($isUrl -and ($urlOk -ccontains $tok)) { continue }
                        $shownTok = if ($isUrl) { $tok } else { '(chemin masque)' }
                        $violations.Add("C4 chaine interdite dans $rel (motif $pat) : $shownTok")
                        $hit = $true
                        break
                    }
                    if ($hit) { break }
                }
            }
            $emailRe = '[A-Za-z0-9._%+\-]{2,}@[A-Za-z0-9\-]+(\.[A-Za-z0-9\-]+)*\.(com|net|org|fr|io|dev|eu)\b'
            foreach ($txt in $ascii, $u0, $u1) {
                foreach ($mm in [regex]::Matches($txt, $emailRe)) {
                    if ($mm.Value -ne $emailOk) { $violations.Add("C4 adresse e-mail dans $rel"); break }
                }
            }
            $summary.Add("$rel : chaines propres analysees (certificat Authenticode exclu : $($null -ne $r))")
        } elseif ($ThirdPartyStrings) {
            $bytes = [IO.File]::ReadAllBytes($b.FullName)
            $txt = [Text.Encoding]::GetEncoding(28591).GetString($bytes)
            $hosts = [regex]::Matches($txt, 'https?://([A-Za-z0-9.\-]+)') | ForEach-Object { $_.Groups[1].Value.ToLowerInvariant() } | Sort-Object -Unique
            if ($hosts) { $notes.Add("C4 tiers $rel : hotes cites : $($hosts -join ', ')") }
        }
    }
    $notes.Add("C2/C3/C4 : $($bins.Count) binaire(s), $($allFiles.Count) fichier(s) analyses.")
    foreach ($s in $summary) { $notes.Add($s) }
}

# ----------------------------------------------------------------------------------------------
Invoke-SourceScan
if (-not $SourcesOnly) {
    if (-not $Path) {
        $Path = if (Test-Path (Join-Path $repo 'build/ci/bin')) { 'build/ci/bin' } else { 'build/ci' }
    }
    $dir = if ([IO.Path]::IsPathRooted($Path)) { $Path } else { Join-Path $repo $Path }
    if (-not (Test-Path $dir)) { Write-Host "Dossier introuvable : $Path"; exit 2 }
    $dir = (Resolve-Path $dir).Path
    $null = Enter-EnhVsEnv
    Invoke-BinaryScan $dir
}

# Rapport.
$status = if ($violations.Count -eq 0) { 'OK' } else { 'ECHEC' }
if ($Report) {
    $md = New-Object System.Collections.Generic.List[string]
    $md.Add('# Rapport check-no-network')
    $md.Add('')
    $md.Add("Resultat : **$status** (mode : $(if ($SourcesOnly) { 'sources' } else { 'sources + binaires' })$(if ($Package) { ', livrable' }))")
    $md.Add('')
    $md.Add("## Violations ($($violations.Count))"); $violations | ForEach-Object { $md.Add("- $_") }
    $md.Add(''); $md.Add("## Avertissements ($($warnings.Count))"); $warnings | ForEach-Object { $md.Add("- $_") }
    $md.Add(''); $md.Add('## Notes'); $notes | ForEach-Object { $md.Add("- $_") }
    $rp = if ([IO.Path]::IsPathRooted($Report)) { $Report } else { Join-Path $repo $Report }
    Set-Content -LiteralPath $rp -Value $md -Encoding UTF8
}
$notes | ForEach-Object { Write-Host "  . $_" }
$warnings | ForEach-Object { Write-Host "  ! $_" -ForegroundColor Yellow }
if ($violations.Count -gt 0) {
    $violations | ForEach-Object { Write-Host "  - $_" -ForegroundColor Red }
    Write-Host "check-no-network ECHEC : $($violations.Count) violation(s)." -ForegroundColor Red
    exit 1
}
Write-Host 'check-no-network OK.' -ForegroundColor Green
exit 0
