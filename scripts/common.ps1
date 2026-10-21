# Fonctions communes (a charger par dot-sourcing : . "$PSScriptRoot\common.ps1").
# ASCII seulement : compatible Windows PowerShell 5.1 et PowerShell 7.
Set-StrictMode -Version Latest

$script:RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

function Get-VsInstallPath {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path $vswhere)) { throw "vswhere introuvable : Visual Studio 2026 n'est pas installe." }
    $path = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $path) { throw "Visual Studio avec les outils C++ (x64) est introuvable." }
    return $path
}

# Assainit l'environnement du processus (shells d'agents / Git Bash) :
#  - retire msys64/mingw du PATH (ld.exe de MinGW passerait avant link.exe de MSVC) ;
#  - retrouve VCPKG_ROOT (variable utilisateur/machine, sinon C:\Tools\vcpkg) et le cache binaire.
function Initialize-EnhEnv {
    $kept = $env:PATH -split ';' | Where-Object { $_ -and ($_ -notmatch '(?i)[\\/](msys(32|64)|mingw(32|64))([\\/]|$)') }
    $env:PATH = $kept -join ';'
    foreach ($name in 'VCPKG_ROOT', 'VCPKG_DEFAULT_BINARY_CACHE') {
        if (-not [Environment]::GetEnvironmentVariable($name, 'Process')) {
            $v = [Environment]::GetEnvironmentVariable($name, 'User')
            if (-not $v) { $v = [Environment]::GetEnvironmentVariable($name, 'Machine') }
            if ($v) { [Environment]::SetEnvironmentVariable($name, $v, 'Process') }
        }
    }
    if (-not $env:VCPKG_ROOT -and (Test-Path 'C:\Tools\vcpkg\vcpkg.exe')) { $env:VCPKG_ROOT = 'C:\Tools\vcpkg' }
    if (-not $env:VCPKG_DEFAULT_BINARY_CACHE -and (Test-Path 'F:\vcpkg-cache')) { $env:VCPKG_DEFAULT_BINARY_CACHE = 'F:\vcpkg-cache' }
}

# Charge l'environnement "x64 Native Tools" dans la session courante (cl, link, Ninja, CMake de VS).
# Le CMake de MSYS2 du PATH est ecarte : on utilise celui de VS (ADR-020).
function Enter-EnhVsEnv {
    Initialize-EnhEnv
    $vs = Get-VsInstallPath
    $devShell = Join-Path $vs 'Common7\Tools\Launch-VsDevShell.ps1'
    # Launch-VsDevShell appelle vswhere par le PATH.
    $env:PATH = (Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer') + ';' + $env:PATH
    $here = Get-Location
    & $devShell -Arch amd64 -HostArch amd64 -SkipAutomaticLocation | Out-Null
    Set-Location $here
    $cmakeDir = Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin'
    $ninjaDir = Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja'
    $env:PATH = "$cmakeDir;$ninjaDir;$env:PATH"
    # Langue demandee a cl (peut etre ignoree si le pack anglais est absent).
    $env:VSLANG = "1033"
    # cl emet les messages dans la page de code de la console : UTF-8 partout pour que le prefixe
    # /showIncludes detecte par CMake soit identique a celui vu par Ninja (suivi des en-tetes).
    chcp 65001 | Out-Null
    $toolset = $env:VCToolsVersion
    if (-not $toolset -or -not $toolset.StartsWith('14.5')) {
        Write-Warning "Toolset MSVC $toolset : v145 (14.5x) attendu (ADR-020)."
    }
    return [pscustomobject]@{ VsPath = $vs; CMake = (Join-Path $cmakeDir 'cmake.exe'); Ctest = (Join-Path $cmakeDir 'ctest.exe') }
}

function Get-EnhGitHash {
    $h = & git -C $script:RepoRoot rev-parse --short HEAD 2>$null
    if ($LASTEXITCODE -ne 0) { return 'sans-commit' }
    return $h
}

function Get-EnhGitBranch {
    $b = & git -C $script:RepoRoot rev-parse --abbrev-ref HEAD 2>$null
    if ($LASTEXITCODE -ne 0) { return 'inconnue' }
    return ($b -replace '[^A-Za-z0-9._-]', '-')
}
