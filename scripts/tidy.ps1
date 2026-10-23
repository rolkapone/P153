<#
.SYNOPSIS
  clang-tidy local sur les fichiers de src/ presents dans compile_commands.json du preset choisi.
  Complete la CI, qui n'analyse que les fichiers du preset ci (sans GPU ni UI) : lancer avec
  -Preset local-gpu (ou dev) avant le merge des stories [GPU] et [UI].
.EXAMPLE
  scripts\tidy.ps1 -Preset local-gpu
#>
[CmdletBinding()]
param(
    [ValidateSet('dev', 'dev-nogpu', 'local-gpu', 'release', 'ci', 'ci-ui')]
    [string]$Preset = 'local-gpu'
)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot\common.ps1"

$tidy = Join-Path (Get-VsInstallPath) 'VC\Tools\Llvm\x64\bin\clang-tidy.exe'
if (-not (Test-Path $tidy)) { $tidy = (Get-Command clang-tidy -ErrorAction Stop).Source }
$buildDir = Join-Path $script:RepoRoot "build\$Preset"
$db = Join-Path $buildDir 'compile_commands.json'
if (-not (Test-Path $db)) {
    Write-Host "compile_commands.json absent : configuration du preset $Preset." -ForegroundColor Yellow
    & "$PSScriptRoot\build.ps1" -Preset $Preset -ConfigureOnly
    if (-not (Test-Path $db)) { throw "compile_commands.json introuvable dans $buildDir." }
}
Push-Location $script:RepoRoot
try {
    $built = (Get-Content $db -Raw | ConvertFrom-Json).file |
        ForEach-Object { (Resolve-Path -LiteralPath $_).Path } | Sort-Object -Unique
    $files = @(git ls-files 'src/*.cpp' | Where-Object { (Resolve-Path -LiteralPath $_).Path -in $built })
    Write-Host "$($files.Count) fichier(s) analyses (preset $Preset)."
    if ($files.Count -eq 0) { return }
    & $tidy -p $buildDir --quiet --extra-arg=-Wno-unused-command-line-argument @files
    if ($LASTEXITCODE -ne 0) { throw 'clang-tidy : findings bloquants (bugprone-*, owning-memory).' }
} finally { Pop-Location }
