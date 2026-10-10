param(
    [Parameter(Mandatory = $false)][string]$WorkspaceFolder
)

$ErrorActionPreference = 'Stop'

if (-not $WorkspaceFolder) {
    $WorkspaceFolder = (Resolve-Path (Join-Path $PSScriptRoot '..')).ProviderPath
}

$buildDirs = @(
    (Join-Path $WorkspaceFolder 'build-rpi-debug'),
    (Join-Path $WorkspaceFolder 'build-rpi-release')
)

foreach ($dir in $buildDirs) {
    if (Test-Path -LiteralPath $dir) {
        Write-Host "Cleaning: $dir" -ForegroundColor Cyan
        & cmake --build $dir --target clean
        if ($LASTEXITCODE -ne 0) {
            exit $LASTEXITCODE
        }
    }
}

Write-Host 'Clean finished.' -ForegroundColor Green
