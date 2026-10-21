<#
.SYNOPSIS
  Installe le hook git LOCAL pre-commit (non versionne : .git\hooks) qui lance precommit-check.ps1,
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
Write-Host "Hook installe : .git\hooks\pre-commit ; identite locale : $(& git -C $root config user.name) <$(& git -C $root config user.email)>"
