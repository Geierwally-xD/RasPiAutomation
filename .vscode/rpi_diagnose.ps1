param(
    [string]$HostName,
    [string]$UserName = 'pi',
    [string]$KeyPath = "$(Join-Path $env:USERPROFILE '.ssh\id_ed25519_raspi')"
)

$ErrorActionPreference = 'Stop'

function Get-VSCodeSettingValue {
    param([Parameter(Mandatory = $true)][string]$Key)

    $settingsPath = Join-Path $PSScriptRoot 'settings.json'
    if (-not (Test-Path -LiteralPath $settingsPath)) { return $null }
    try {
        $json = Get-Content -LiteralPath $settingsPath -Raw | ConvertFrom-Json
    } catch {
        return $null
    }
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

if (-not $HostName) {
    throw "RasPi Host fehlt. Setze 'raspi.host' in .vscode/settings.json oder übergib -HostName."
}

# Prefer configured key path from .vscode/settings.json if present
$keyFromSettings = Get-VSCodeSettingValue -Key 'raspi.keyPath'
if ($keyFromSettings) {
    $keyFromSettings = Expand-KnownTokens -Value $keyFromSettings
    if ($keyFromSettings) {
        $KeyPath = $keyFromSettings
    }
}

if (-not (Test-Path $KeyPath)) {
    $fallback = Join-Path $env:USERPROFILE '.ssh\id_rsa'
    if (Test-Path $fallback) {
        $KeyPath = $fallback
    }
}

if (-not (Test-Path $KeyPath)) {
    throw "Kein SSH-Key gefunden. Erwartet z.B. $env:USERPROFILE\.ssh\id_ed25519_raspi oder id_rsa."
}

$target = "$UserName@$HostName"

$remoteBinary = Get-VSCodeSettingValue -Key 'raspi.remoteBinary'
if (-not $remoteBinary) {
    $remoteBinary = '/home/pi/JokiAutomation/RasPiAutomation'
}

# Safely embed path into a single-quoted POSIX shell string
$remoteBinaryBash = $remoteBinary.Replace("'", "'\\''")

$cmd = @(
    'set -e',
    'echo "=== OS ==="',
    'cat /etc/os-release || true',
    'echo "=== glibc ==="',
    'getconf GNU_LIBC_VERSION || true',
    'ldd --version 2>/dev/null | head -n 1 || true',
    'echo "=== libc.so.6 strings GLIBC_ ==="',
    'LIBC=$(ls -1 /lib/*-linux-gnu/libc.so.6 /lib/*/libc.so.6 2>/dev/null | head -n 1 || true); echo "libc: ${LIBC:-not_found}"; test -n "$LIBC" && strings "$LIBC" 2>/dev/null | grep -E "^GLIBC_" | sort -V | tail -n 10 || true',
    'echo "=== libcrypt ==="',
    'ls -l /lib/aarch64-linux-gnu/libcrypt.so* /usr/lib/aarch64-linux-gnu/libcrypt.so* 2>/dev/null || true',
    'echo "=== wiringPi ==="',
    'ls -l /usr/local/lib/libwiringPi.so* /usr/lib/*/libwiringPi.so* 2>/dev/null || true',
    'WPI=$(ls -1 /usr/local/lib/libwiringPi.so /usr/lib/*/libwiringPi.so 2>/dev/null | head -n 1 || true); echo "wiringPi: ${WPI:-not_found}"; test -n "$WPI" && readelf -V "$WPI" 2>/dev/null | head -n 80 || true',
    'echo "=== remote binary ==="',
    "echo bin: '$remoteBinaryBash'; ls -l '$remoteBinaryBash' 2>/dev/null || true",
    "test -e '$remoteBinaryBash' && file '$remoteBinaryBash' || true",
    "test -e '$remoteBinaryBash' && echo --- ldd --- && ldd '$remoteBinaryBash' || true",
    "test -e '$remoteBinaryBash' && echo --- wiringPi resolved --- && ldd '$remoteBinaryBash' 2>/dev/null | grep -i wiringpi || true"
) -join '; '

$sshArgs = @(
    '-i', $KeyPath,
    '-o', 'BatchMode=yes',
    '-o', 'PasswordAuthentication=no',
    $target,
    $cmd
)

& ssh @sshArgs
if ($LASTEXITCODE -ne 0) {
    throw "SSH Diagnose fehlgeschlagen (Exit $LASTEXITCODE). Prüfe, ob der Key in ~/.ssh/authorized_keys auf dem Pi installiert ist und ob 'raspi.keyPath' korrekt ist."
}
