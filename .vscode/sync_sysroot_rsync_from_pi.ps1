<#
Sync Sysroot from Raspberry Pi to Windows.
1. Creates the Sysroot directory on the Pi (via rsync)
2. Packs the Sysroot as a .tar.gz archive on the Pi
3. Copies the archive via SCP to the gccArmCrossCompiler directory
4. Outputs instructions for unpacking with WSL
#>

param(
    [string]$HostName,
    [string]$UserName = 'pi',
    [string]$KeyPath = "$(Join-Path $HOME '.ssh\id_ed25519_raspi')"
)

$ErrorActionPreference = 'Stop'

function Get-VSCodeSettingValue {
    param([Parameter(Mandatory = $true)][string]$Key)
    $settingsPath = Join-Path $PSScriptRoot 'settings.json'
    if (-not (Test-Path -LiteralPath $settingsPath)) { return $null }
    try {
        $json = Get-Content -LiteralPath $settingsPath -Raw | ConvertFrom-Json
    } catch { return $null }
    $prop = $json.PSObject.Properties[$Key]
    if ($null -eq $prop) { return $null }
    return $prop.Value
}

function Expand-KnownTokens {
    param([Parameter(Mandatory = $true)][AllowNull()][object]$Value)
    if ($null -eq $Value) { return $null }
    $s = [string]$Value
    if (-not $s) { return $s }
    $homeValue = $HOME
    if (-not $homeValue) { $homeValue = $env:USERPROFILE }
    $s = $s.Replace('$env:USERPROFILE', $env:USERPROFILE)
    $s = $s.Replace('$HOME', $homeValue)
    return $s
}

if (-not $HostName) {
    $HostName = Get-VSCodeSettingValue -Key 'raspi.host'
}
if (-not $UserName -or $UserName -eq 'pi') {
    $u = Get-VSCodeSettingValue -Key 'raspi.user'
    if ($u) { $UserName = $u }
}
if (-not $HostName) {
    throw "RasPi Host fehlt. Setze 'raspi.host' in .vscode/settings.json oder übergib -HostName."
}
$keyFromSettings = Get-VSCodeSettingValue -Key 'raspi.keyPath'
if ($keyFromSettings) {
    $keyFromSettings = Expand-KnownTokens -Value $keyFromSettings
    if ($keyFromSettings) { $KeyPath = $keyFromSettings }
}
if (-not (Test-Path $KeyPath)) {
    $fallback = Join-Path $HOME '.ssh\id_rsa'
    if (Test-Path $fallback) { $KeyPath = $fallback }
}
if (-not (Test-Path $KeyPath)) {
    throw "No SSH key found. Expected e.g. $HOME\.ssh\id_ed25519_raspi or id_rsa."
}

$target = "$UserName@$HostName"

# Target: ToolChains/SysGCC/GCC64RasPi4/gccArmCrossCompiler relative to workspace
$toolchainsRoot = Join-Path $PSScriptRoot '..\..\..\..\.ToolChains\SysGCC\GCC64RasPi4\gccArmCrossCompiler'
$toolchainsRoot = [System.IO.Path]::GetFullPath($toolchainsRoot)
$DestArchive = Join-Path $toolchainsRoot 'sysroot-pi.tar.gz'

# Source on the Pi
$SourceDir = "/var/tmp/sysroot-pi"
$ArchiveOnPi = "/var/tmp/sysroot-pi.tar.gz"

# Prevent standby/screen-off during download (current session only)
Write-Host "Setting power options: Disabling standby and screen-off (current session only) ..." -ForegroundColor Yellow
try {
    powercfg -change -standby-timeout-ac 0
    powercfg -change -monitor-timeout-ac 0
} catch {
    Write-Warning "Could not set power options. Please check manually if needed."
}

# Step 1: Create Sysroot on the Pi (via SSH)
Write-Host "`n[1/3] Creating Sysroot on the Pi at $SourceDir ..." -ForegroundColor Cyan
$createSysrootCmd = @(
    'set -e',
    "sudo rm -rf $SourceDir",
    "sudo mkdir -p $SourceDir/lib",
    "sudo mkdir -p $SourceDir/usr/lib",
    "sudo mkdir -p $SourceDir/usr/include",
    "sudo mkdir -p $SourceDir/usr/local/include",
    "sudo mkdir -p $SourceDir/usr/local/lib",
    "sudo rsync -a /lib/aarch64-linux-gnu $SourceDir/lib/",
    "sudo rsync -a /usr/lib/aarch64-linux-gnu $SourceDir/usr/lib/",
    "sudo rsync -a /usr/include $SourceDir/usr/",
    "sudo rsync -a /usr/include/aarch64-linux-gnu $SourceDir/usr/include/",
    "sudo rsync -a /usr/include/aarch64-linux-gnu/bits $SourceDir/usr/include/aarch64-linux-gnu/",
    "sudo rsync -a /usr/local/include $SourceDir/usr/local/",
    "sudo rsync -a /usr/local/lib $SourceDir/usr/local/",
    "sudo chown -R ${UserName}:${UserName} $SourceDir"
) -join '; '
$sshCreateArgs = @('-i', $KeyPath, '-o', 'BatchMode=yes', '-o', 'PasswordAuthentication=no', $target, $createSysrootCmd)
& ssh @sshCreateArgs
if ($LASTEXITCODE -ne 0) { throw "ERROR: Sysroot creation on the Pi failed (Exit $LASTEXITCODE)." }

# Step 2: Pack Sysroot on the Pi as .tar.gz
Write-Host "`n[2/3] Packing Sysroot on the Pi as archive ..." -ForegroundColor Cyan
$packCmd = @(
    'set -e',
    "cd /var/tmp",
    "tar -czf $ArchiveOnPi -C /var/tmp sysroot-pi",
    "ls -lh $ArchiveOnPi"
) -join '; '
$sshPackArgs = @('-i', $KeyPath, '-o', 'BatchMode=yes', '-o', 'PasswordAuthentication=no', $target, $packCmd)
& ssh @sshPackArgs
if ($LASTEXITCODE -ne 0) { throw "ERROR: Archiving on the Pi failed (Exit $LASTEXITCODE)." }

# Step 3: Copy archive via SCP to Windows
Write-Host "`n[3/3] Copying archive from ${target}:${ArchiveOnPi} to $DestArchive ..." -ForegroundColor Cyan
if (-not (Test-Path $toolchainsRoot)) {
    New-Item -ItemType Directory -Force -Path $toolchainsRoot | Out-Null
}

$scpArgs = @('-i', $KeyPath, '-o', 'BatchMode=yes', '-o', 'PasswordAuthentication=no', "${target}:${ArchiveOnPi}", $DestArchive)
& scp @scpArgs
if ($LASTEXITCODE -ne 0) { throw "SCP download (archive) failed (Exit $LASTEXITCODE)." }

Write-Host "`nArchive successfully copied to: $DestArchive" -ForegroundColor Green

# Instructions for unpacking with WSL
Write-Host "`n===================================================================" -ForegroundColor Yellow
Write-Host "IMPORTANT: Unpack with WSL (not with Windows archive programs!)" -ForegroundColor Yellow
Write-Host "===================================================================" -ForegroundColor Yellow
Write-Host "`nExecute the following commands in WSL to unpack the Sysroot:`n" -ForegroundColor White

# Convert Windows path to WSL path
$wslToolchainsRoot = $toolchainsRoot -replace '\\', '/' -replace '^([A-Z]):', { "/mnt/$($_.Groups[1].Value.ToLower())" }

Write-Host "cd `"$wslToolchainsRoot`"" -ForegroundColor Cyan
Write-Host "rm -rf sysroot-pi" -ForegroundColor Cyan
Write-Host "tar -xzf sysroot-pi.tar.gz" -ForegroundColor Cyan
Write-Host "rm sysroot-pi.tar.gz" -ForegroundColor Cyan
Write-Host ""
Write-Host "After that, the Sysroot is available at $toolchainsRoot\sysroot-pi." -ForegroundColor White
Write-Host "`nNote: You can reset the power options after the sync, e.g. in Windows settings." -ForegroundColor Yellow
