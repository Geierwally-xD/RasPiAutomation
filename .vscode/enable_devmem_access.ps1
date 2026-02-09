param(
    [Parameter(Mandatory = $true)][string]$HostName,
    [Parameter(Mandatory = $true)][string]$UserName,
    [Parameter(Mandatory = $false)][string]$KeyPath,
    [Parameter(Mandatory = $false)][string]$SudoPassword,
    [Parameter(Mandatory = $false)][string]$GroupName = 'devmem'
)

function ConvertTo-PosixSingleQuotedString {
    param([Parameter(Mandatory = $true)][AllowNull()][string]$Value)
    if ($null -eq $Value) { return "''" }
    return "'" + ($Value -replace "'", "'\\''") + "'"
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

if (-not $KeyPath) {
    $settingsPath = Join-Path $PSScriptRoot 'settings.json'
    if (Test-Path -LiteralPath $settingsPath) {
        try {
            $json = Get-Content -LiteralPath $settingsPath -Raw | ConvertFrom-Json
            $KeyPath = $json.'raspi.keyPath'
            $SudoPassword = $json.'raspi.sudoPassword'
        } catch {
            # ignore
        }
    }
}

$KeyPath = Expand-KnownTokens -Value $KeyPath
if (-not $KeyPath) { $KeyPath = "$env:USERPROFILE\.ssh\id_ed25519" }

$remoteTarget = "$UserName@$HostName"

$sshArgs = @(
    '-i', $KeyPath,
    '-o', 'BatchMode=yes',
    '-o', 'PasswordAuthentication=no'
)

# udev rule: make /dev/mem group-accessible
$ruleLine = 'KERNEL=="mem", GROUP="' + $GroupName + '", MODE="0660"'
$rulePath = '/etc/udev/rules.d/99-devmem.rules'

$sudoPwB64 = ''
if ($SudoPassword) {
    $sudoPwB64 = [Convert]::ToBase64String([System.Text.Encoding]::UTF8.GetBytes($SudoPassword))
}

$remoteScript = @(
    'set -e',
    "GROUP=$GroupName",
    "USER=$UserName",
    "RULE_PATH=$rulePath",
    'RULE_LINE="KERNEL==\"mem\", GROUP=\"$GROUP\", MODE=\"0660\""',
    "SUDO_PW_B64=$sudoPwB64",
    'if [ -n "$SUDO_PW_B64" ]; then',
    '  SUDOPW="$(echo "$SUDO_PW_B64" | base64 -d)"',
    '  sudo_run() { printf "%s\\n" "$SUDOPW" | sudo -S -p "" "$@"; }',
    'else',
    '  sudo_run() { sudo -n "$@"; }',
    'fi',
    'sudo_run getent group "$GROUP" >/dev/null 2>&1 || sudo_run groupadd "$GROUP"',
    'sudo_run usermod -a -G "$GROUP" "$USER"',
    'sudo_run sh -c ''printf "%s\\n" "$1" > "$2"'' sh "$RULE_LINE" "$RULE_PATH"',
    'sudo_run cat "$RULE_PATH" || true',
    'sudo_run udevadm control --reload-rules',
    'sudo_run udevadm trigger --sysname-match=mem || true',
    'ls -l /dev/mem || true',
    'id "$USER" || true',
    'echo "NOTE: You must log out/in (or reboot) for group membership to apply to existing sessions."'
) -join "`n"

$remoteScriptB64 = [Convert]::ToBase64String([System.Text.Encoding]::UTF8.GetBytes($remoteScript))

Write-Host "Configuring persistent /dev/mem permissions via udev on $remoteTarget ..." -ForegroundColor Cyan
& ssh @sshArgs $remoteTarget "bash -lc 'echo $remoteScriptB64 | base64 -d | bash'"
if ($LASTEXITCODE -ne 0) {
    throw "Remote command failed with exit code $LASTEXITCODE"
}

Write-Host 'Done.' -ForegroundColor Green
