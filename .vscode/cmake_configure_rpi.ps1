param(
    [Parameter(Mandatory = $false)][string]$WorkspaceFolder,
    [Parameter(Mandatory = $false)][string]$BuildDir,
    [Parameter(Mandatory = $false)][string]$ToolchainsRoot,
    [Parameter(Mandatory = $false)][switch]$ForceFresh
)

$ErrorActionPreference = 'Stop'

function Get-CMakeCacheValue {
    param(
        [Parameter(Mandatory = $true)][string]$CachePath,
        [Parameter(Mandatory = $true)][string]$Key
    )

    if (-not (Test-Path -LiteralPath $CachePath)) { return $null }
    $pattern = '^' + [regex]::Escape($Key) + ':[^=]*=(.*)$'
    foreach ($line in (Get-Content -LiteralPath $CachePath)) {
        $m = [regex]::Match($line, $pattern)
        if ($m.Success) {
            return $m.Groups[1].Value
        }
    }
    return $null
}

function Resolve-FullPath {
    param([Parameter(Mandatory = $true)][string]$Path)
    try {
        return (Resolve-Path -LiteralPath $Path).ProviderPath
    } catch {
        return [System.IO.Path]::GetFullPath($Path)
    }
}

function Get-VSCodeJsonSetting {
    param(
        [Parameter(Mandatory = $true)][string]$SettingsPath,
        [Parameter(Mandatory = $true)][string]$Key
    )

    if (-not (Test-Path -LiteralPath $SettingsPath)) { return $null }
    try {
        $json = Get-Content -LiteralPath $SettingsPath -Raw | ConvertFrom-Json
    } catch {
        return $null
    }
    $prop = $json.PSObject.Properties[$Key]
    if ($null -eq $prop) { return $null }
    return $prop.Value
}

function Expand-KnownTokens {
    param([AllowNull()][object]$Value)

    if ($null -eq $Value) { return $null }
    $s = [string]$Value
    if (-not $s) { return $s }

    $homeValue = $HOME
    if (-not $homeValue) { $homeValue = $env:USERPROFILE }

    $s = $s.Replace('$env:USERPROFILE', $env:USERPROFILE)
    $s = $s.Replace('$HOME', $homeValue)
    return $s
}

function Find-ToolchainsRoot {
    param(
        [Parameter(Mandatory = $true)][string]$StartFolder
    )

    $cur = Resolve-FullPath -Path $StartFolder
    while ($true) {
        $candidate = Join-Path $cur 'ToolChains'
        if (Test-Path -LiteralPath $candidate) {
            $sysGccCandidate = Join-Path $candidate 'SysGCC\GCC64RasPi4\gccArmCrossCompiler\bin\aarch64-none-linux-gnu-gcc.exe'
            $ninjaCandidate = Join-Path $candidate 'Ninja\ninja.exe'
            if ((Test-Path -LiteralPath $sysGccCandidate) -and (Test-Path -LiteralPath $ninjaCandidate)) {
                return (Resolve-FullPath -Path $candidate)
            }
        }

        $parent = Split-Path -Parent $cur
        if (-not $parent -or $parent -eq $cur) { break }
        $cur = $parent
    }

    return $null
}

if (-not $WorkspaceFolder) {
    $WorkspaceFolder = Resolve-FullPath -Path (Join-Path $PSScriptRoot '..')
} else {
    $WorkspaceFolder = Resolve-FullPath -Path $WorkspaceFolder
}

if (-not $BuildDir) {
    $BuildDir = Join-Path $WorkspaceFolder 'build-rpi'
} else {
    $BuildDir = Resolve-FullPath -Path $BuildDir
}

if (-not $ToolchainsRoot) {
    $settingsPath = Join-Path $PSScriptRoot 'settings.json'
    $fromSettings = Get-VSCodeJsonSetting -SettingsPath $settingsPath -Key 'custom.toolchainsRoot'
    $fromSettings = Expand-KnownTokens -Value $fromSettings

    if ($fromSettings) {
        if (-not [System.IO.Path]::IsPathRooted($fromSettings)) {
            $fromSettings = Join-Path $WorkspaceFolder $fromSettings
        }
        $ToolchainsRoot = $fromSettings
    } else {
        $found = Find-ToolchainsRoot -StartFolder $WorkspaceFolder
        if ($found) {
            $ToolchainsRoot = $found
        } else {
            $ToolchainsRoot = Join-Path $WorkspaceFolder '..\..\ToolChains'
        }
    }
}
$ToolchainsRoot = Resolve-FullPath -Path $ToolchainsRoot

$ninjaExe = Join-Path $ToolchainsRoot 'Ninja\ninja.exe'
$sysGccRoot = Join-Path $ToolchainsRoot 'SysGCC\GCC64RasPi4'
$crossBin = Join-Path $sysGccRoot 'gccArmCrossCompiler\bin'

$cmakeToolchainFile = Join-Path $WorkspaceFolder 'cmake\toolchains\aarch64-linux-gnu.cmake'

$cc = Join-Path $crossBin 'aarch64-none-linux-gnu-gcc.exe'
$cxx = Join-Path $crossBin 'aarch64-none-linux-gnu-g++.exe'

Write-Host "WorkspaceFolder : $WorkspaceFolder" -ForegroundColor Cyan
Write-Host "BuildDir        : $BuildDir" -ForegroundColor Cyan
Write-Host "ToolchainsRoot  : $ToolchainsRoot" -ForegroundColor Cyan

foreach ($p in @($ninjaExe, $cmakeToolchainFile, $cc, $cxx)) {
    if (-not (Test-Path -LiteralPath $p)) {
        throw "Required path not found: $p`nEither place a ToolChains folder in a parent directory of the workspace or set custom.toolchainsRoot in .vscode/settings.json."
    }
}

$cmakeArgs = @('-S', $WorkspaceFolder, '-B', $BuildDir)

$cachePath = Join-Path $BuildDir 'CMakeCache.txt'

$needsFreshConfigure = $false
$useFresh = [bool]$ForceFresh
if (Test-Path -LiteralPath $cachePath) {
    $cachedCc = Get-CMakeCacheValue -CachePath $cachePath -Key 'CMAKE_C_COMPILER'
    $cachedCxx = Get-CMakeCacheValue -CachePath $cachePath -Key 'CMAKE_CXX_COMPILER'
    $cachedNinja = Get-CMakeCacheValue -CachePath $cachePath -Key 'CMAKE_MAKE_PROGRAM'
    $cachedToolchain = Get-CMakeCacheValue -CachePath $cachePath -Key 'CMAKE_TOOLCHAIN_FILE'

    if (-not $cachedCc -or -not (Test-Path -LiteralPath $cachedCc) -or ((Resolve-FullPath -Path $cachedCc) -ne $cc)) { $needsFreshConfigure = $true }
    if (-not $cachedCxx -or -not (Test-Path -LiteralPath $cachedCxx) -or ((Resolve-FullPath -Path $cachedCxx) -ne $cxx)) { $needsFreshConfigure = $true }
    if (-not $cachedNinja -or -not (Test-Path -LiteralPath $cachedNinja) -or ((Resolve-FullPath -Path $cachedNinja) -ne $ninjaExe)) { $needsFreshConfigure = $true }

    # If the toolchain file is stored as an unexpanded token (e.g. ${custom...}) or points elsewhere,
    # recreate to keep the project portable across machines/drives.
    if (-not $cachedToolchain) {
        $needsFreshConfigure = $true
    } elseif ($cachedToolchain -match '\$\{' -or $cachedToolchain -match '\$env:' -or $cachedToolchain -match '\%\w+\%') {
        $needsFreshConfigure = $true
    } else {
        try {
            $cachedToolchainResolved = $cachedToolchain
            if (-not [System.IO.Path]::IsPathRooted($cachedToolchainResolved)) {
                $cachedToolchainResolved = Join-Path $BuildDir $cachedToolchainResolved
            }
            if (-not (Test-Path -LiteralPath $cachedToolchainResolved)) {
                $needsFreshConfigure = $true
            } elseif ((Resolve-FullPath -Path $cachedToolchainResolved) -ne $cmakeToolchainFile) {
                $needsFreshConfigure = $true
            }
        } catch {
            $needsFreshConfigure = $true
        }
    }

    if ($needsFreshConfigure) {
        $useFresh = $true
        Write-Host "Existing build cache uses a different/missing toolchain; running a fresh configure in $BuildDir" -ForegroundColor Yellow
    }
}

$needExplicitArgs = $useFresh -or (-not (Test-Path -LiteralPath $cachePath))
if ($useFresh) {
    $cmakeArgs = @('--fresh') + $cmakeArgs
}

if ($needExplicitArgs) {
    $cmakeArgs += @(
        '-G', 'Ninja',
        "-DCMAKE_MAKE_PROGRAM=$ninjaExe",
        "-DCMAKE_TOOLCHAIN_FILE=$cmakeToolchainFile",
        "-DTOOLCHAIN_BIN_DIR=$crossBin",
        "-DCMAKE_C_COMPILER=$cc",
        "-DCMAKE_CXX_COMPILER=$cxx",
        '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'
    )
}

Write-Host 'Running cmake configure:' -ForegroundColor Green
Write-Host ('cmake ' + ($cmakeArgs -join ' '))
& cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
