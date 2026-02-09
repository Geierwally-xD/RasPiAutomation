param(
    [Parameter(Mandatory = $false)][string]$ToolchainsRoot
)

# PowerShell-Skript: Löscht das build-rpi-Verzeichnis, wenn CMakeCache.txt einen anderen Quellpfad enthält
$ErrorActionPreference = 'Stop'

$sourceDir = (Resolve-Path -LiteralPath $PSScriptRoot).ProviderPath
$buildDir = Join-Path $sourceDir 'build-rpi'
$cacheFile = Join-Path $buildDir 'CMakeCache.txt'

if (Test-Path $cacheFile) {
    $cacheContent = Get-Content $cacheFile -Raw
    if ($cacheContent -match 'CMAKE_HOME_DIRECTORY:INTERNAL=(.+)') {
        $cachedSource = $matches[1]
        if ($cachedSource -ne $sourceDir) {
            Write-Host "CMakeCache.txt verweist auf $cachedSource, aktuelles Verzeichnis ist $sourceDir. Lösche $buildDir ..."
            Remove-Item $buildDir -Recurse -Force
        }
    }
}

# CMake-Konfiguration starten (delegiert an das robuste VSCode-Wrapper-Skript)
$wrapper = Join-Path $sourceDir '.vscode\cmake_configure_rpi.ps1'
if (-not (Test-Path -LiteralPath $wrapper)) {
    throw "Wrapper not found: $wrapper"
}

$args = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $wrapper, '-WorkspaceFolder', $sourceDir, '-BuildDir', $buildDir)
if ($ToolchainsRoot) {
    $args += @('-ToolchainsRoot', $ToolchainsRoot)
}

& powershell @args
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
