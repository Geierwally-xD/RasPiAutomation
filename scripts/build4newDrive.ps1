param(
    [Parameter(Mandatory = $false)][string]$WorkspaceFolder,
    [Parameter(Mandatory = $false)][string]$BuildDir,
    [Parameter(Mandatory = $false)][string]$ToolchainsRoot,
    # Keep this as string to be resilient against odd argument passing/quoting.
    # Supported values: 1/0, true/false, yes/no, on/off.
    [Parameter(Mandatory = $false)][string]$Fresh = '1',
    # Equivalent to VS Code “CMake: Delete Cache and Reconfigure” (best-effort):
    # deletes CMakeCache.txt + CMakeFiles/ (does NOT delete the whole build dir).
    # Supported values: 1/0, true/false, yes/no, on/off.
    [Parameter(Mandatory = $false)][string]$DeleteCache = '1'
)

$ErrorActionPreference = 'Stop'

function Resolve-FullPath {
    param([Parameter(Mandatory = $true)][string]$Path)
    try {
        return (Resolve-Path -LiteralPath $Path).ProviderPath
    } catch {
        return [System.IO.Path]::GetFullPath($Path)
    }
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

$configureWrapper = Join-Path $WorkspaceFolder '.vscode\cmake_configure_rpi.ps1'
if (-not (Test-Path -LiteralPath $configureWrapper)) {
    throw "Configure wrapper not found: $configureWrapper"
}

Write-Host "WorkspaceFolder : $WorkspaceFolder" -ForegroundColor Cyan
Write-Host "BuildDir        : $BuildDir" -ForegroundColor Cyan

# On a new drive / new machine, a stale build folder is the most common problem.
$doFresh = $true
if ($null -ne $Fresh) {
    $freshText = ([string]$Fresh).Trim().ToLowerInvariant()
    if ($freshText -match '^(0|false|no|off)$') { $doFresh = $false }
    elseif ($freshText -match '^(1|true|yes|on)$') { $doFresh = $true }
}

if ($doFresh -and (Test-Path -LiteralPath $BuildDir)) {
    Write-Host "Removing build directory for fresh configure: $BuildDir" -ForegroundColor Yellow
    try {
        Remove-Item -LiteralPath $BuildDir -Recurse -Force
    } catch {
        Write-Host "Could not delete $BuildDir (likely locked). Will fall back to 'cmake --fresh' during configure." -ForegroundColor Yellow
    }
}

$doDeleteCache = $true
if ($null -ne $DeleteCache) {
    $deleteText = ([string]$DeleteCache).Trim().ToLowerInvariant()
    if ($deleteText -match '^(0|false|no|off)$') { $doDeleteCache = $false }
    elseif ($deleteText -match '^(1|true|yes|on)$') { $doDeleteCache = $true }
}

if ($doDeleteCache -and (Test-Path -LiteralPath $BuildDir)) {
    $cacheFile = Join-Path $BuildDir 'CMakeCache.txt'
    $cacheDir = Join-Path $BuildDir 'CMakeFiles'

    Write-Host "Deleting CMake cache (best-effort): $cacheFile ; $cacheDir" -ForegroundColor Yellow
    try {
        if (Test-Path -LiteralPath $cacheFile) { Remove-Item -LiteralPath $cacheFile -Force }
        if (Test-Path -LiteralPath $cacheDir) { Remove-Item -LiteralPath $cacheDir -Recurse -Force }
    } catch {
        Write-Host "Could not fully delete cache (likely locked). Will fall back to 'cmake --fresh' during configure." -ForegroundColor Yellow
    }
}

$psArgs = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $configureWrapper, '-WorkspaceFolder', $WorkspaceFolder, '-BuildDir', $BuildDir)
if ($ToolchainsRoot) { $psArgs += @('-ToolchainsRoot', $ToolchainsRoot) }
if ($doFresh -or $doDeleteCache) { $psArgs += @('-ForceFresh') }
& powershell @psArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host 'Building...' -ForegroundColor Green
& cmake --build $BuildDir
exit $LASTEXITCODE
