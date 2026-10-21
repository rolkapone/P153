<#
.SYNOPSIS
  Applique clang-format (ou, avec -Check, verifie sans modifier : meme commande que la CI).
#>
[CmdletBinding()]
param([switch]$Check)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot\common.ps1"

$vs = Get-VsInstallPath
$cf = Join-Path $vs 'VC\Tools\Llvm\x64\bin\clang-format.exe'
if (-not (Test-Path $cf)) { $cf = (Get-Command clang-format -ErrorAction Stop).Source }
Write-Host ("clang-format " + (& $cf --version))

$files = & git -C $script:RepoRoot ls-files -co --exclude-standard -- 'src/*.cpp' 'src/*.hpp' 'src/*.h' 'tests/*.cpp' 'tests/*.hpp'
$files = @($files | ForEach-Object { Join-Path $script:RepoRoot $_ })
if ($files.Count -eq 0) { Write-Host 'Aucun fichier C++.'; return }
if ($Check) { & $cf --dry-run --Werror @files } else { & $cf -i @files }
if ($LASTEXITCODE -ne 0) { throw 'clang-format : differences detectees.' }
