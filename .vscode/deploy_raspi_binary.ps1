param(
    [Parameter(Mandatory = $true)][string]$HostName,
    [Parameter(Mandatory = $true)][string]$UserName,
    [Parameter(Mandatory = $true)][string]$KeyPath,
    [Parameter(Mandatory = $true)][string]$LocalBinary,
    [Parameter(Mandatory = $true)][string]$RemoteBinary,
    [switch]$Yes
)

$ErrorActionPreference = 'Stop'

function Expand-KnownTokens {
    param([Parameter(Mandatory = $true)][string]$Value)

    $s = $Value
    $homeValue = $HOME
    if (-not $homeValue) { $homeValue = $env:USERPROFILE }

    $s = $s.Replace('$env:USERPROFILE', $env:USERPROFILE)
    $s = $s.Replace('$HOME', $homeValue)
    return $s
}

function Resolve-ExistingPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    $expanded = Expand-KnownTokens -Value $Path
    $resolved = Resolve-Path -LiteralPath $expanded -ErrorAction SilentlyContinue
    if ($resolved) {
        return $resolved.ProviderPath
    }
    return $expanded
}

function ConvertTo-PosixSingleQuotedString {
    param([Parameter(Mandatory = $true)][string]$Value)
    return ("'" + ($Value -replace "'", "'\\''") + "'")
}

function ConvertTo-RemotePosixPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    $p = $Path.Trim() -replace '\\', '/'
    if ($p.EndsWith('/')) {
        $leaf = Split-Path -Leaf $LocalBinary
        if (-not $leaf) { $leaf = 'RasPiAutomation' }
        return ($p + $leaf)
    }
    return $p
}

$KeyPath = Resolve-ExistingPath -Path $KeyPath
$LocalBinary = Resolve-ExistingPath -Path $LocalBinary
$RemoteBinary = ConvertTo-RemotePosixPath -Path $RemoteBinary

if (-not (Test-Path -LiteralPath $KeyPath)) {
    throw "SSH key not found: $KeyPath"
}

if (-not (Test-Path -LiteralPath $LocalBinary)) {
    throw "Local binary not found: $LocalBinary"
}

$remoteTarget = "$UserName@$HostName"
$remoteDir = '/'
$lastSlash = $RemoteBinary.LastIndexOf('/')
if ($lastSlash -gt 0) {
    $remoteDir = $RemoteBinary.Substring(0, $lastSlash)
}

$sshArgs = @('-i', $KeyPath)
$scpArgs = @('-i', $KeyPath, '-O')
if ($Yes) {
    $sshArgs += @('-o', 'BatchMode=yes', '-o', 'PasswordAuthentication=no', '-o', 'StrictHostKeyChecking=accept-new')
    $scpArgs += @('-o', 'BatchMode=yes', '-o', 'PasswordAuthentication=no', '-o', 'StrictHostKeyChecking=accept-new')
}

$remoteDirQ = ConvertTo-PosixSingleQuotedString -Value $remoteDir
$remoteBinQ = ConvertTo-PosixSingleQuotedString -Value $RemoteBinary
$prepareCmd = "set -e; mkdir -p -- $remoteDirQ; rm -f -- $remoteBinQ 2>/dev/null || true"
$verifyCmd = "set -e; chmod +x -- $remoteBinQ; ls -l -- $remoteBinQ"

Write-Host "Deploying binary to $remoteTarget" -ForegroundColor Cyan
Write-Host "Local : $LocalBinary" -ForegroundColor Cyan
Write-Host "Remote: $RemoteBinary" -ForegroundColor Cyan

& ssh @sshArgs $remoteTarget $prepareCmd
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& scp @scpArgs $LocalBinary "$remoteTarget`:$RemoteBinary"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& ssh @sshArgs $remoteTarget $verifyCmd
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host 'Deploy finished.' -ForegroundColor Green
