param(
    [Parameter(Mandatory = $false)][string]$WorkspaceFolder,
    [Parameter(Mandatory = $false)][string]$BuildDir,
    [Parameter(Mandatory = $false)][string]$ToolchainsRoot
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

# Ensure configure is correct (also repairs stale cache/toolchain paths).
if ($ToolchainsRoot) {
    & powershell -NoProfile -ExecutionPolicy Bypass -File $configureWrapper -WorkspaceFolder $WorkspaceFolder -BuildDir $BuildDir -ToolchainsRoot $ToolchainsRoot
} else {
    & powershell -NoProfile -ExecutionPolicy Bypass -File $configureWrapper -WorkspaceFolder $WorkspaceFolder -BuildDir $BuildDir
}
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# Clean if a build system exists (ignore if clean target isn't available yet)
if (Test-Path -LiteralPath (Join-Path $BuildDir 'build.ninja')) {
    Write-Host 'Running clean...' -ForegroundColor Green
    & cmake --build $BuildDir --target clean
}

Write-Host 'Building...' -ForegroundColor Green
& cmake --build $BuildDir
exit $LASTEXITCODE
