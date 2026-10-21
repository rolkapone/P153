<#
.SYNOPSIS
  Installe les hooks git LOCAUX pre-commit et commit-msg (non versionnes : .git\hooks) qui lancent
  precommit-check.ps1 (le second refuse toute mention de Claude/Anthropic dans un message),
  et fixe l'identite git locale de l'equipe.
#>
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

& git -C $root config --local user.name 'Carol MBONZIMA'
& git -C $root config --local user.email '81617903+rolkapone@users.noreply.github.com'

$hook = Join-Path $root '.git\hooks\pre-commit'
$content = @'
#!/bin/sh
# Hook local installe par scripts/install-hooks.ps1
root="$(git rev-parse --show-toplevel)"
exec powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$root/scripts/precommit-check.ps1"
'@
# Fins de ligne LF : le hook est interprete par sh (Git for Windows).
[System.IO.File]::WriteAllText($hook, ($content -replace "`r`n", "`n") + "`n", (New-Object System.Text.UTF8Encoding($false)))
$hook2 = Join-Path $root '.git\hooks\commit-msg'
$content2 = @'
#!/bin/sh
# Hook local installe par scripts/install-hooks.ps1 : aucune mention de Claude/Anthropic.
root="$(git rev-parse --show-toplevel)"
exec powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$root/scripts/precommit-check.ps1" -CommitMsgFile "$1"
'@
[System.IO.File]::WriteAllText($hook2, ($content2 -replace "`r`n", "`n") + "`n", (New-Object System.Text.UTF8Encoding($false)))
Write-Host "Hooks installes : pre-commit, commit-msg ; identite locale : $(& git -C $root config user.name) <$(& git -C $root config user.email)>"
