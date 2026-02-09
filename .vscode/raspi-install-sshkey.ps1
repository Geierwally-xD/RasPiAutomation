[CmdletBinding()]
param(
  [Parameter(Mandatory = $true)]
  [string]$Host,

  [Parameter(Mandatory = $true)]
  [string]$User,

  [Parameter(Mandatory = $false)]
  [string]$KeyPath = (Join-Path $env:USERPROFILE ".ssh\id_ed25519_raspi"),

  [Parameter(Mandatory = $false)]
  [string]$Comment = "vscode-raspiautomation"
)

$ErrorActionPreference = "Stop"

function Require-Command([string]$Name) {
  if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
    throw "Missing required command '$Name'. Install Windows OpenSSH Client (Settings -> Optional features -> OpenSSH Client) or ensure '$Name' is on PATH."
  }
}

Require-Command ssh
Require-Command ssh-keygen

$sshDir = Split-Path -Parent $KeyPath
if (-not (Test-Path $sshDir)) {
  New-Item -ItemType Directory -Path $sshDir | Out-Null
}

if (-not (Test-Path $KeyPath)) {
  & ssh-keygen -t ed25519 -f $KeyPath -N "" -C $Comment | Out-Host
}

$pubPath = "$KeyPath.pub"
if (-not (Test-Path $pubPath)) {
  throw "Public key not found: $pubPath"
}

$keyLine = (Get-Content -Path $pubPath -Raw).Trim()
if ([string]::IsNullOrWhiteSpace($keyLine)) {
  throw "Public key file is empty: $pubPath"
}

Write-Host "Installing SSH public key on $User@$Host ..."

$remoteCmd = @'
set -e
umask 077
mkdir -p ~/.ssh
chmod 700 ~/.ssh
touch ~/.ssh/authorized_keys
chmod 600 ~/.ssh/authorized_keys
read -r KEY
if ! grep -qxF "$KEY" ~/.ssh/authorized_keys; then
  echo "$KEY" >> ~/.ssh/authorized_keys
fi
'@

# Pipe the key line to the remote bash script.
"$keyLine`n" | & ssh -o StrictHostKeyChecking=accept-new "$User@$Host" "bash -lc '$remoteCmd'"

Write-Host "Done. Key installed: $pubPath"
Write-Host "Tip: You can verify with: ssh -i `"$KeyPath`" $User@$Host"
