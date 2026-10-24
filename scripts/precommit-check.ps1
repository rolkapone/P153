<#
.SYNOPSIS
  Garde-fous avant commit (ADR-035) : identite git noreply, fichiers interdits, binaires,
  gros fichiers, secrets simples, adresses e-mail personnelles. Lance par le hook pre-commit.
.DESCRIPTION
  -CheckBranch <branche> : verifie en plus les auteurs et les messages de main..<branche> (avant un merge).
  -CommitMsgFile <fichier> : mode hook commit-msg : refuse tout message qui mentionne Claude/Anthropic
  (trailer Co-Authored-By, "Generated with ...") - regle du PO, aucune mention de l'assistant.
  Code de sortie 0 = OK, 1 = bloque.
#>
[CmdletBinding()]
param([string]$CheckBranch = '', [string]$CommitMsgFile = '')
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$expectedName = 'Carol MBONZIMA'
$expectedMail = '81617903+rolkapone@users.noreply.github.com'
$problems = New-Object System.Collections.Generic.List[string]

function Invoke-Git { & git -C $root @args }

# Motif des mentions interdites dans un message de commit (liste blanche d'identite : aucune autre).
$msgForbidden = '(?im)^\s*co-authored-by:.*(claude|anthropic)|generated with .*claude|noreply@anthropic'
if ($CommitMsgFile) {
    $msg = Get-Content -LiteralPath $CommitMsgFile -Raw
    if ($msg -match $msgForbidden) {
        Write-Host 'COMMIT BLOQUE : le message mentionne Claude/Anthropic (Co-Authored-By ou "Generated with"). Retirer ces lignes.' -ForegroundColor Red
        exit 1
    }
    if ($msg -notmatch '(?im)^Agent:\s*\S+') {
        Write-Host 'Avertissement : ligne "Agent: <role>" absente du message (non bloquant).' -ForegroundColor Yellow
    }
    exit 0
}

# --- 1. Identite ----------------------------------------------------------------------------
$mail = (Invoke-Git config user.email)
$name = (Invoke-Git config user.name)
if ($mail -ne $expectedMail) { $problems.Add("git config user.email = '$mail' (attendu : $expectedMail ; config LOCALE uniquement)") }
if ($name -ne $expectedName) { $problems.Add("git config user.name = '$name' (attendu : $expectedName)") }

if ($CheckBranch) {
    $authors = Invoke-Git log --format='%an <%ae>|%cn <%ce>' "main..$CheckBranch"
    foreach ($a in $authors) {
        foreach ($who in ($a -split '\|')) {
            if ($who -ne "$expectedName <$expectedMail>") { $problems.Add("Auteur/committer non conforme sur $CheckBranch : $who") }
        }
    }
    $bodies = (Invoke-Git log --format='%B' "main..$CheckBranch") -join "`n"
    if ($bodies -match $msgForbidden) { $problems.Add("Un message de commit de $CheckBranch mentionne Claude/Anthropic") }
}

# --- 2. Fichiers indexes --------------------------------------------------------------------
$staged = @(Invoke-Git diff --cached --name-only --diff-filter=ACMR)
$forbiddenPath = '^(\.claude/|CLAUDE\.md$|LISEZMOI-EQUIPE\.md$|KICKOFF\.txt$|_claude-setup/|third_party/|build/|out/|dist/|vcpkg_installed/|\.vs/|\.env($|\.)|logs/)'
$forbiddenExt = '\.(dll|lib|exe|zip|msix|pdb|obj|ilk|exp|onnx|engine|plan|trt|mp4|mkv|mov|avi|webm|mxf|y4m|yuv|pfx|p12|pem|key|cer|crt|jks|7z|msi|pt|pth|ckpt|safetensors|pkl|dmp|log)$'
$allowedBinaries = @()   # liste blanche explicite (chemins relatifs), apres validation secdevops
$maxBytes = 5MB

foreach ($f in $staged) {
    if ($f -match $forbiddenPath) { $problems.Add("Chemin interdit : $f") }
    if (($f -match $forbiddenExt) -and ($allowedBinaries -notcontains $f)) { $problems.Add("Binaire/media interdit : $f (liste blanche : scripts/precommit-check.ps1)") }
    $full = Join-Path $root $f
    if ((Test-Path $full) -and ((Get-Item $full).Length -gt $maxBytes)) { $problems.Add("Fichier > 5 Mio : $f (Git LFS apres validation secdevops)") }
}

# --- 3. Contenu ajoute : secrets et e-mails -------------------------------------------------
$secretPatterns = @(
    'AKIA[0-9A-Z]{16}',
    '-----BEGIN (RSA |EC |OPENSSH |DSA )?PRIVATE KEY-----',
    'gh[pousr]_[A-Za-z0-9]{36,}',
    'github_pat_[A-Za-z0-9_]{20,}',
    '(?i)(password|passwd|secret|api[_-]?key|token)\s*[:=]\s*["''][^"'']{8,}["'']'
)
$mailPattern = '[A-Za-z0-9._%+\-]+@[A-Za-z0-9\-]+(\.[A-Za-z0-9\-]+)*\.[A-Za-z]{2,}\b'   # TLD alphabetique : ecarte color=white@0.12
$mailAllowed = '(^81617903\+rolkapone@users\.noreply\.github\.com$|@example\.(com|org)|@2x\.)'
$currentFile = ''
foreach ($line in (Invoke-Git diff --cached -U0 --no-color)) {
    if ($line -match '^\+\+\+ b/(.+)$') { $currentFile = $Matches[1]; continue }
    if ($line -notmatch '^\+' -or $line -match '^\+\+\+') { continue }
    if ($currentFile -eq 'scripts/precommit-check.ps1') { continue }  # contient les motifs
    foreach ($p in $secretPatterns) {
        if ($line -match $p) { $problems.Add("Secret possible dans ${currentFile} (motif $p)") }
    }
    foreach ($m in [regex]::Matches($line, $mailPattern)) {
        if ($m.Value -notmatch $mailAllowed) { $problems.Add("Adresse e-mail dans ${currentFile} : $($m.Value) (seule l'adresse noreply est autorisee)") }
    }
}

# --- 4. gitleaks (si installe) ----------------------------------------------------------------
$gl = Get-Command gitleaks -ErrorAction SilentlyContinue
if (-not $gl) { $cand = 'C:\Tools\gitleaks\gitleaks.exe'; if (Test-Path $cand) { $gl = Get-Item $cand } }
if ($gl) {
    $exe = if ($gl -is [IO.FileInfo]) { $gl.FullName } else { $gl.Source }
    $prev = $ErrorActionPreference; $ErrorActionPreference = 'Continue'   # gitleaks ecrit ses logs sur stderr
    & $exe git --staged --config (Join-Path $root '.gitleaks.toml') --redact --no-banner 2>&1 | Out-Null
    $glExit = $LASTEXITCODE; $ErrorActionPreference = $prev
    if ($glExit -ne 0) { $problems.Add('gitleaks a detecte un secret ou une information personnelle (relancer : gitleaks git --staged --config .gitleaks.toml --redact -v)') }
} else {
    Write-Host 'gitleaks non installe : controle ignore (filet : motifs ci-dessus).' -ForegroundColor Yellow
}

if ($problems.Count -gt 0) {
    Write-Host 'COMMIT BLOQUE :' -ForegroundColor Red
    $problems | Sort-Object -Unique | ForEach-Object { Write-Host "  - $_" -ForegroundColor Red }
    exit 1
}
Write-Host ("precommit-check OK ({0} fichier(s) indexe(s))." -f $staged.Count) -ForegroundColor Green
exit 0
