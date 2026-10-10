param(
    [string]$ConfigFile = "$PSScriptRoot\\gdbserver-config.local.json",
    [Alias('Host')]
    [string]$HostName,
    [string]$User,
    [string]$SudoPassword,
    [string[]]$PreDebugCommands,
    [string]$KeyPath,
    [object]$CreateKeyIfMissing = $false,
    [object]$InstallPublicKey = $false,
    [string]$LocalBinary,
    [string]$RemoteBinary,
    [object]$ProgramArgs,
    [string]$RemoteGdbserver,
    [int]$Port,
    [switch]$Yes
)

function ConvertTo-PosixSingleQuotedString {
    param([Parameter(Mandatory = $true)][string]$Value)
    return ("'" + ($Value -replace "'", "'\\''") + "'")
}

function ConvertTo-Boolean {
    param([AllowNull()][object]$Value)

    if ($null -eq $Value) { return $false }
    if ($Value -is [bool]) { return [bool]$Value }
    $s = ([string]$Value).Trim()
    if (-not $s) { return $false }
    return ($s -match '^(?i:true|1|yes|y)$')
}

function Ensure-SshKeyPair {
    param(
        [Parameter(Mandatory = $true)][string]$PrivateKeyPath,
        [Parameter(Mandatory = $true)][bool]$CreateIfMissing
    )

    if (Test-Path -LiteralPath $PrivateKeyPath) {
        return
    }

    if (-not $CreateIfMissing) {
        return
    }

    $sshKeygen = Get-Command ssh-keygen -ErrorAction SilentlyContinue
    if (-not $sshKeygen) {
        Write-Error "ssh-keygen ist nicht verfuegbar. Installiere den Windows 'OpenSSH Client' (Optional Features) oder stelle ssh-keygen in PATH bereit."
        exit 2
    }

    $keyDir = Split-Path -Parent $PrivateKeyPath
    if ($keyDir -and -not (Test-Path -LiteralPath $keyDir)) {
        New-Item -ItemType Directory -Path $keyDir -Force | Out-Null
    }

    $comment = ($env:USERNAME + '@' + $env:COMPUTERNAME)

    Write-Host "SSH-Key fehlt. Erzeuge ed25519 Key: $PrivateKeyPath" -ForegroundColor Yellow
    
    # Use cmd.exe to avoid PowerShell argument parsing issues with empty passphrase
    $cmdLine = "ssh-keygen -t ed25519 -f `"$PrivateKeyPath`" -N `"`" -C `"$comment`""
    $result = cmd.exe /c $cmdLine 2>&1
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Output: $result" -ForegroundColor Red
        Write-Error "ssh-keygen fehlgeschlagen (Exit $LASTEXITCODE)."
        exit $LASTEXITCODE
    }

    $pubPath = ($PrivateKeyPath + '.pub')
    if (-not (Test-Path -LiteralPath $pubPath)) {
        try {
            $pub = & $sshKeygen.Source -y -f $PrivateKeyPath
            if ($LASTEXITCODE -eq 0 -and $pub) {
                $pub | Out-File -FilePath $pubPath -Encoding ascii
            }
        } catch {
            # ignore; the private key is what we need for ssh/scp
        }
    }
}

function Get-PublicKeyLine {
    param([Parameter(Mandatory = $true)][string]$PrivateKeyPath)

    $pubPath = ($PrivateKeyPath + '.pub')
    if (Test-Path -LiteralPath $pubPath) {
        return ((Get-Content -LiteralPath $pubPath -Raw).Trim())
    }

    $sshKeygen = Get-Command ssh-keygen -ErrorAction SilentlyContinue
    if (-not $sshKeygen) {
        return $null
    }

    try {
        $pub = & $sshKeygen.Source -y -f $PrivateKeyPath
        if ($LASTEXITCODE -ne 0) { return $null }
        $line = ([string]$pub).Trim()
        if ($line) {
            $line | Out-File -FilePath $pubPath -Encoding ascii
        }
        return $line
    } catch {
        return $null
    }
}

function Test-SshKeyAuth {
    param(
        [Parameter(Mandatory = $true)][string]$KeyPath,
        [Parameter(Mandatory = $true)][string]$RemoteTarget
    )

    $args = @(
        '-i', $KeyPath,
        '-o', 'BatchMode=yes',
        '-o', 'PasswordAuthentication=no',
        '-o', 'StrictHostKeyChecking=accept-new',
        $RemoteTarget,
        'true'
    )

    & ssh @args | Out-Null
    return ($LASTEXITCODE -eq 0)
}

function Install-PublicKeyOnPi {
    param(
        [Parameter(Mandatory = $true)][string]$PublicKeyLine,
        [Parameter(Mandatory = $true)][string]$RemoteTarget,
        [Parameter(Mandatory = $true)][string]$KeyPath,
        [AllowNull()][string]$SudoPassword,
        [Parameter(Mandatory = $true)][string]$UserName
    )

    if (-not $PublicKeyLine) {
        Write-Error 'PublicKey ist leer; kann nicht installieren.'
        exit 2
    }

    $sudoPrefixLocal = 'sudo -n'
    if ($SudoPassword) {
        $pwQ = ConvertTo-PosixSingleQuotedString -Value $SudoPassword
        $sudoPrefixLocal = "printf '%s\\n' $pwQ | sudo -S -p ''"
    }

    # Read key from stdin (strip CR) and append only if missing.
    # Use a temp file + grep -f to avoid any quoting/word-splitting issues with spaces in key lines.
    # Repair ownership/permissions with sudo (pi's ~/.ssh is often accidentally root-owned).
    $remoteCmd = @(
        'set -e',
        'TMP=${TMPDIR:-/tmp}/authkey.$$; cat | tr -d ''\r'' > "$TMP"',
        'test -s "$TMP"',
        'mkdir -p ~/.ssh 2>/dev/null || true',
        "$sudoPrefixLocal mkdir -p ~/.ssh",
        "$sudoPrefixLocal chown -R ${UserName}:${UserName} ~/.ssh || true",
        "$sudoPrefixLocal chmod 700 ~/.ssh || true",
        "$sudoPrefixLocal touch ~/.ssh/authorized_keys",
        "$sudoPrefixLocal chown ${UserName}:${UserName} ~/.ssh/authorized_keys || true",
        "$sudoPrefixLocal chmod 600 ~/.ssh/authorized_keys || true",
        'grep -qxF -f "$TMP" ~/.ssh/authorized_keys || cat "$TMP" >> ~/.ssh/authorized_keys',
        'rm -f "$TMP" || true'
    ) -join '; '

    Write-Host 'Installing public key into ~/.ssh/authorized_keys (may prompt for SSH password once)...' -ForegroundColor Yellow

    $PublicKeyLine | & ssh -i $KeyPath $RemoteTarget $remoteCmd
    if ($LASTEXITCODE -ne 0) {
        Write-Error "PublicKey-Installation fehlgeschlagen (Exit $LASTEXITCODE)."
        exit $LASTEXITCODE
    }
}

function Read-JsonWithComments {
    param([Parameter(Mandatory = $true)][string]$Path)

    if (-not (Test-Path -LiteralPath $Path)) { return $null }

    $raw = Get-Content -LiteralPath $Path -Raw
    $sb = New-Object System.Text.StringBuilder
    $inString = $false
    $escape = $false
    $inLineComment = $false
    $inBlockComment = $false

    for ($i = 0; $i -lt $raw.Length; $i++) {
        $ch = $raw[$i]
        $next = if ($i + 1 -lt $raw.Length) { $raw[$i + 1] } else { [char]0 }

        if ($inLineComment) {
            if ($ch -eq "`n" -or $ch -eq "`r") {
                $inLineComment = $false
                [void]$sb.Append($ch)
            }
            continue
        }

        if ($inBlockComment) {
            if ($ch -eq '*' -and $next -eq '/') {
                $inBlockComment = $false
                $i++
            }
            continue
        }

        if ($inString) {
            [void]$sb.Append($ch)
            if ($escape) {
                $escape = $false
            } elseif ($ch -eq '\\') {
                $escape = $true
            } elseif ($ch -eq '"') {
                $inString = $false
            }
            continue
        }

        if ($ch -eq '"') {
            $inString = $true
            [void]$sb.Append($ch)
            continue
        }

        if ($ch -eq '/' -and $next -eq '/') {
            $inLineComment = $true
            $i++
            continue
        }

        if ($ch -eq '/' -and $next -eq '*') {
            $inBlockComment = $true
            $i++
            continue
        }

        [void]$sb.Append($ch)
    }

    try {
        return ($sb.ToString() | ConvertFrom-Json)
    } catch {
        return $null
    }
}

function Get-VSCodeSettingValue {
    param([Parameter(Mandatory = $true)][string]$Key)

    $settingsPath = Join-Path $PSScriptRoot 'settings.json'
    if (-not (Test-Path -LiteralPath $settingsPath)) { return $null }

    $json = Read-JsonWithComments -Path $settingsPath
    if ($null -eq $json) { return $null }

    $prop = $json.PSObject.Properties[$Key]
    if ($null -eq $prop) { return $null }
    return $prop.Value
}

function Get-WorkspaceFolder {
    $root = Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')
    if ($root -and $root.Count -gt 0) {
        return $root[0].ProviderPath
    }
    return (Get-Location).Path
}

function ConvertTo-StringArray {
    param([AllowNull()][object]$Value)

    if ($null -eq $Value) { return @() }
    if ($Value -is [string]) {
        $s = ([string]$Value).Trim()
        if (-not $s) { return @() }
        # If the user provides a single string, treat it as ONE argument.
        # For multiple arguments, provide an array in settings/json: ["--a", "b c"].
        return @($s)
    }

    if ($Value -is [System.Collections.IEnumerable]) {
        $out = @()
        foreach ($v in $Value) {
            if ($null -eq $v) { continue }
            $s = ([string]$v).Trim()
            if ($s) { $out += $s }
        }
        return $out
    }

    $single = ([string]$Value).Trim()
    if (-not $single) { return @() }
    return @($single)
}

function Expand-KnownTokens {
    param(
        [Parameter(Mandatory = $true)][AllowNull()][object]$Value,
        [Parameter(Mandatory = $true)][string]$WorkspaceFolder
    )

    if ($null -eq $Value) { return $null }
    $s = [string]$Value
    if (-not $s) { return $s }

    $homeValue = $HOME
    if (-not $homeValue) { $homeValue = $env:USERPROFILE }

    # Literal token replacements (no regex escaping needed)
    $s = $s.Replace('${workspaceFolder}', $WorkspaceFolder)
    $s = $s.Replace('$env:USERPROFILE', $env:USERPROFILE)
    $s = $s.Replace('$HOME', $homeValue)

    return $s
}

function Resolve-PathIfPossible {
    param(
        [Parameter(Mandatory = $true)][AllowNull()][string]$Path,
        [Parameter(Mandatory = $true)][string]$BaseFolder
    )

    if (-not $Path) { return $Path }
    $p = $Path

    # If it's a relative path, interpret relative to the workspace.
    if (-not [System.IO.Path]::IsPathRooted($p)) {
        $p = Join-Path $BaseFolder $p
    }

    try {
        $rp = Resolve-Path -LiteralPath $p -ErrorAction Stop
        if ($rp -and $rp.Count -gt 0) { return $rp[0].ProviderPath }
    } catch {
        return $p
    }
    return $p
}

function Find-FirstExistingPath {
    param([Parameter(Mandatory = $true)][string[]]$Candidates)

    foreach ($c in $Candidates) {
        if (-not $c) { continue }
        if (Test-Path -LiteralPath $c) { return $c }
    }
    return $null
}

$configWasProvided = $PSBoundParameters.ContainsKey('ConfigFile')
if (-not $configWasProvided) {
    $ConfigFile = Find-FirstExistingPath -Candidates @(
        (Join-Path $PSScriptRoot 'gdbserver-config.local.json'),
        (Join-Path $PSScriptRoot 'gdbserver-config.json'),
        (Join-Path $PSScriptRoot 'gdbserver-config.example.json')
    )

    if (-not $ConfigFile) {
        $ConfigFile = Join-Path $PSScriptRoot 'gdbserver-config.local.json'
    }
}

# Load defaults from JSON if present
if (Test-Path -LiteralPath $ConfigFile) {
    $cfg = Get-Content -LiteralPath $ConfigFile -Raw | ConvertFrom-Json
} else {
    $cfg = @{}
}

# Merge parameters, prefer explicit args
if (-not $HostName) { $HostName = $cfg.Host }
if (-not $User) { $User = $cfg.User }
if (-not $SudoPassword) { $SudoPassword = $cfg.SudoPassword }
if (-not $PreDebugCommands) { $PreDebugCommands = $cfg.PreDebugCommands }
if (-not $KeyPath) { $KeyPath = $cfg.KeyPath }
if (-not $LocalBinary) { $LocalBinary = $cfg.LocalBinary }
if (-not $RemoteBinary) { $RemoteBinary = $cfg.RemoteBinary }
if (-not $ProgramArgs) { $ProgramArgs = $cfg.ProgramArgs }
if (-not $RemoteGdbserver) { $RemoteGdbserver = $cfg.RemoteGdbserver }
if (-not $Port) { $Port = $cfg.Port }

# Workspace defaults from VS Code settings
if (-not $HostName) { $HostName = Get-VSCodeSettingValue -Key 'raspi.host' }
if (-not $User) { $User = Get-VSCodeSettingValue -Key 'raspi.user' }
if (-not $KeyPath) { $KeyPath = Get-VSCodeSettingValue -Key 'raspi.keyPath' }
if (-not $SudoPassword) { $SudoPassword = Get-VSCodeSettingValue -Key 'raspi.sudoPassword' }
if (-not $PreDebugCommands) { $PreDebugCommands = Get-VSCodeSettingValue -Key 'raspi.preDebugCommands' }
if (-not $RemoteBinary) { $RemoteBinary = Get-VSCodeSettingValue -Key 'raspi.remoteBinary' }
if (-not $ProgramArgs) { $ProgramArgs = Get-VSCodeSettingValue -Key 'raspi.programArgs' }

$createWasProvided = $PSBoundParameters.ContainsKey('CreateKeyIfMissing')
if (-not $createWasProvided) {
    $CreateKeyIfMissing = Get-VSCodeSettingValue -Key 'raspi.createKeyIfMissing'
}
$CreateKeyIfMissing = ConvertTo-Boolean $CreateKeyIfMissing

$installWasProvided = $PSBoundParameters.ContainsKey('InstallPublicKey')
if (-not $installWasProvided) {
    $InstallPublicKey = Get-VSCodeSettingValue -Key 'raspi.installPublicKey'
}
$InstallPublicKey = ConvertTo-Boolean $InstallPublicKey

$WorkspaceFolder = Get-WorkspaceFolder

if (-not $KeyPath) {
    $homeValue = $HOME
    if (-not $homeValue) { $homeValue = $env:USERPROFILE }
    $KeyPath = Join-Path $homeValue '.ssh\id_ed25519_raspi'
}

$KeyPath = Expand-KnownTokens -Value $KeyPath -WorkspaceFolder $WorkspaceFolder
$LocalBinary = Expand-KnownTokens -Value $LocalBinary -WorkspaceFolder $WorkspaceFolder

$KeyPath = Resolve-PathIfPossible -Path $KeyPath -BaseFolder $WorkspaceFolder
$LocalBinary = Resolve-PathIfPossible -Path $LocalBinary -BaseFolder $WorkspaceFolder

$ProgramArgsList = ConvertTo-StringArray -Value $ProgramArgs

if (-not $HostName -or -not $User -or -not $KeyPath -or -not $LocalBinary -or -not $RemoteBinary -or -not $RemoteGdbserver -or -not $Port) {
    Write-Error "Missing required parameters. Provide via arguments or edit $ConfigFile"
    exit 2
}

if (-not (Test-Path -LiteralPath $KeyPath)) {
    Ensure-SshKeyPair -PrivateKeyPath $KeyPath -CreateIfMissing $CreateKeyIfMissing

    if (Test-Path -LiteralPath $KeyPath) {
        # created successfully
    } else {
    $candidates = @(
        $KeyPath,
        (Join-Path $HOME '.ssh\id_ed25519_raspi'),
        (Join-Path $env:USERPROFILE '.ssh\id_ed25519_raspi'),
        (Join-Path $HOME '.ssh\id_ed25519'),
        (Join-Path $env:USERPROFILE '.ssh\id_ed25519'),
        (Join-Path $HOME '.ssh\id_rsa'),
        (Join-Path $env:USERPROFILE '.ssh\id_rsa')
    )

    $found = Find-FirstExistingPath -Candidates $candidates
    if ($found) {
        $KeyPath = $found
    } else {
        Write-Error "SSH-Key nicht gefunden: $KeyPath"
        Write-Error "Gepruefte Alternativen:"
        $candidates | ForEach-Object { if ($_) { Write-Error "  $_" } }
        exit 2
    }
    }
}

$remoteTarget = $User + '@' + $HostName

if ($InstallPublicKey) {
    $pubLine = Get-PublicKeyLine -PrivateKeyPath $KeyPath
    if (-not $pubLine) {
        Write-Error "Public Key konnte nicht gelesen/erzeugt werden fuer: $KeyPath"
        exit 2
    }

    if (-not (Test-SshKeyAuth -KeyPath $KeyPath -RemoteTarget $remoteTarget)) {
        if ($Yes) {
            Write-Error 'SSH-Key ist noch nicht auf dem Pi autorisiert. Setze `raspi.installPublicKey=true` und starte das Skript einmal INTERAKTIV (ohne -Yes), um das Pi-Passwort eingeben zu koennen; danach laufen Tasks non-interactive.'
            exit 2
        }

        Install-PublicKeyOnPi -PublicKeyLine $pubLine -RemoteTarget $remoteTarget -KeyPath $KeyPath -SudoPassword $SudoPassword -UserName $User

        if (-not (Test-SshKeyAuth -KeyPath $KeyPath -RemoteTarget $remoteTarget)) {
            Write-Error 'SSH Key Auth funktioniert nach Installation weiterhin nicht. Pruefe Benutzer/Host und ~/.ssh Rechte.'
            exit 2
        }
    }
}

Write-Host "Using SSH identity: $KeyPath" -ForegroundColor Cyan
$keyAuthOk = Test-SshKeyAuth -KeyPath $KeyPath -RemoteTarget $remoteTarget
if (-not $keyAuthOk) {
    if ($Yes) {
        Write-Error "SSH PublicKey Auth ist nicht aktiv fuer $remoteTarget mit Key '$KeyPath'. Setze in .vscode/settings.json: raspi.installPublicKey=true und starte das Skript EINMAL ohne -Yes (damit du das Pi-Passwort eingeben kannst). Danach wieder -Yes/Tasks nutzen."
        exit 255
    }
    Write-Warning "SSH PublicKey Auth ist nicht aktiv fuer $remoteTarget mit Key '$KeyPath' (du wirst nach dem Pi-Passwort gefragt)."
}

if (-not (Test-Path -LiteralPath $LocalBinary)) {
    Write-Error "LocalBinary nicht gefunden: $LocalBinary"
    exit 2
}

function Format-CommandLine {
    param(
        [Parameter(Mandatory = $true)][string]$Exe,
        [Parameter(Mandatory = $true)][object[]]$Args
    )

    $parts = @($Exe)
    foreach ($arg in $Args) {
        $s = [string]$arg
        if ($s -match '\s') {
            $parts += ('"' + ($s -replace '"', '\\"') + '"')
        } else {
            $parts += $s
        }
    }
    return ($parts -join ' ')
}

# Build the commands/args

function Normalize-RemotePosixPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    $p = $Path.Trim()
    if (-not $p) { return $p }

    $p = $p -replace '\\', '/'
    if ($p.StartsWith('\')) {
        $p = '/' + $p.TrimStart('\')
    }
    return $p
}

function Ensure-RemoteBinaryFilePath {
    param(
        [Parameter(Mandatory = $true)][string]$RemotePath,
        [Parameter(Mandatory = $true)][string]$LocalBinaryPath
    )

    $leaf = Split-Path -Leaf $LocalBinaryPath
    if (-not $leaf) { $leaf = 'RasPiAutomation' }

    if ($RemotePath.EndsWith('/')) {
        return ($RemotePath + $leaf)
    }

    return $RemotePath
}

$RemoteBinary = Normalize-RemotePosixPath -Path $RemoteBinary
$RemoteBinary = Ensure-RemoteBinaryFilePath -RemotePath $RemoteBinary -LocalBinaryPath $LocalBinary

if (-not $RemoteBinary) {
    Write-Error "RemoteBinary ist leer. Setze es in $ConfigFile oder in .vscode/settings.json (raspi.remoteBinary)."
    exit 2
}

$remoteDir = '/'
$lastSlash = $RemoteBinary.LastIndexOf('/')
if ($lastSlash -gt 0) {
    $remoteDir = $RemoteBinary.Substring(0, $lastSlash)
}

$scpDest = $remoteTarget + ':' + $RemoteBinary
$sshCommonArgs = @('-i', $KeyPath)
$scpCommonArgs = @('-i', $KeyPath)
if ($Yes) {
    $sshCommonArgs += @('-o', 'BatchMode=yes', '-o', 'PasswordAuthentication=no', '-o', 'StrictHostKeyChecking=accept-new')
    $scpCommonArgs += @('-o', 'BatchMode=yes', '-o', 'PasswordAuthentication=no', '-o', 'StrictHostKeyChecking=accept-new')
}

$scpArgs = @()
$scpArgs += $scpCommonArgs
# Use legacy SCP protocol (-O) to avoid "subsystem request failed" when
# the Pi's SFTP subsystem is not enabled (Windows OpenSSH scp defaults to SFTP).
$scpArgs += @('-O')
$scpArgs += @($LocalBinary, $scpDest)

$sudoPrefix = 'sudo -n'
if ($SudoPassword) {
    $pw = ConvertTo-PosixSingleQuotedString -Value $SudoPassword
    $sudoPrefix = "printf '%s\\n' $pw | sudo -S -p ''"
}

$remoteGdbserverLeaf = Split-Path -Leaf $RemoteGdbserver
$invTokens = @()
if ($remoteGdbserverLeaf -eq 'gdbserver') {
    $invTokens = @($RemoteGdbserver, ":$Port", $RemoteBinary)
} elseif ($RemoteGdbserver -match '-(\d+)$') {
    if ([int]$Matches[1] -eq $Port) {
        $invTokens = @($RemoteGdbserver, $RemoteBinary)
    } else {
        $invTokens = @($RemoteGdbserver, $Port, $RemoteBinary)
    }
} else {
    $invTokens = @($RemoteGdbserver, $Port, $RemoteBinary)
}

if ($ProgramArgsList -and $ProgramArgsList.Count -gt 0) {
    $invTokens += $ProgramArgsList
}

$remoteGdbserverInvocation = (($invTokens | ForEach-Object { ConvertTo-PosixSingleQuotedString -Value $_ }) -join ' ')

function Remove-LeadingSudo {
    param([Parameter(Mandatory = $true)][string]$Command)
    $c = $Command.Trim()
    if ($c -match '^(?i)sudo\s+') {
        return ($c -replace '^(?i)sudo\s+', '')
    }
    return $c
}

$preParts = @()
if ($PreDebugCommands) {
    foreach ($cmd in $PreDebugCommands) {
        if (-not $cmd) { continue }
        $c = Remove-LeadingSudo -Command ([string]$cmd)
        if ($c) {
            $wrapped = ConvertTo-PosixSingleQuotedString -Value ($c + ' || true')
            $preParts += ($sudoPrefix + ' sh -c ' + $wrapped)
        }
    }
}

$preChain = ($preParts -join '; ')
$killLine = "command -v fuser >/dev/null 2>&1 && fuser -k $Port/tcp >/dev/null 2>&1 || true"
$killCmd = ConvertTo-PosixSingleQuotedString -Value $killLine
$remoteLogMaxBytes = 5242880
$rotateLine = "if [ -f /tmp/gdbserver-$Port.log ]; then if [ `$(stat -c%s /tmp/gdbserver-$Port.log 2>/dev/null || echo 0) -ge $remoteLogMaxBytes ]; then rm -f /tmp/gdbserver-$Port.log.2; mv -f /tmp/gdbserver-$Port.log.1 /tmp/gdbserver-$Port.log.2 2>/dev/null || true; mv -f /tmp/gdbserver-$Port.log /tmp/gdbserver-$Port.log.1; fi; fi"
$rotateCmd = ConvertTo-PosixSingleQuotedString -Value $rotateLine

# Start gdbserver as root (sudo). This avoids /dev/mem permission issues for hardware PWM.
$startPrefix = $sudoPrefix
if ($preChain) {
    $remoteCmd = "set -e; $preChain; $sudoPrefix sh -c $killCmd; $sudoPrefix sh -c $rotateCmd; $startPrefix nohup $remoteGdbserverInvocation >/tmp/gdbserver-$Port.log 2>&1 < /dev/null &"
} else {
    $remoteCmd = "set -e; $sudoPrefix sh -c $killCmd; $sudoPrefix sh -c $rotateCmd; $startPrefix nohup $remoteGdbserverInvocation >/tmp/gdbserver-$Port.log 2>&1 < /dev/null &"
}

$remoteDirQ = ConvertTo-PosixSingleQuotedString -Value $remoteDir
$remoteBinQ = ConvertTo-PosixSingleQuotedString -Value $RemoteBinary

$remoteOwnerQ = ConvertTo-PosixSingleQuotedString -Value ($User + ':' + $User)

$remotePrepareCmd = "set -e; mkdir -p -- $remoteDirQ 2>/dev/null || true; $sudoPrefix mkdir -p -- $remoteDirQ; $sudoPrefix chown -- $remoteOwnerQ $remoteDirQ; $sudoPrefix chmod 775 -- $remoteDirQ; $sudoPrefix rm -f -- $remoteBinQ 2>/dev/null || true"
$remoteChmodCmd = "set -e; chmod +x -- $remoteBinQ; test -x /usr/sbin/setcap && $sudoPrefix /usr/sbin/setcap cap_sys_rawio+ep $remoteBinQ || true; test -x /usr/sbin/getcap && /usr/sbin/getcap $remoteBinQ || true; ls -l -- $remoteBinQ; test -f $remoteBinQ"

$sshPrepareArgs = @()
$sshPrepareArgs += $sshCommonArgs
$sshPrepareArgs += @($remoteTarget, $remotePrepareCmd)

$sshChmodArgs = @()
$sshChmodArgs += $sshCommonArgs
$sshChmodArgs += @($remoteTarget, $remoteChmodCmd)

$sshArgs = @()
$sshArgs += $sshCommonArgs
$sshArgs += @($remoteTarget, $remoteCmd)

Write-Host "SCP command:" -ForegroundColor Cyan
Write-Host (Format-CommandLine -Exe 'scp' -Args $scpArgs)
Write-Host "SSH command (prepare remote dir):" -ForegroundColor Cyan
Write-Host (Format-CommandLine -Exe 'ssh' -Args $sshPrepareArgs)
Write-Host "SSH command (chmod +x + verify remote binary):" -ForegroundColor Cyan
Write-Host (Format-CommandLine -Exe 'ssh' -Args $sshChmodArgs)
Write-Host "SSH command to start gdbserver:" -ForegroundColor Cyan
Write-Host (Format-CommandLine -Exe 'ssh' -Args $sshArgs)

if (-not $Yes) {
    $confirm = Read-Host "Execute these commands now? (y/N)"
    if ($confirm -ne 'y' -and $confirm -ne 'Y') {
        Write-Host "Aborted by user."
        exit 0
    }
}

Write-Host "Preparing remote directory..." -ForegroundColor Green
& ssh @sshPrepareArgs
if ($LASTEXITCODE -ne 0) {
    Write-Error "Remote prepare failed with exit code $LASTEXITCODE"
    exit $LASTEXITCODE
}

Write-Host "Running SCP..." -ForegroundColor Green
& scp @scpArgs
if ($LASTEXITCODE -ne 0) {
    Write-Error "SCP failed with exit code $LASTEXITCODE"
    exit $LASTEXITCODE
}

Write-Host "Marking remote binary executable + verify..." -ForegroundColor Green
& ssh @sshChmodArgs
if ($LASTEXITCODE -ne 0) {
    Write-Error "Remote chmod/verify failed with exit code $LASTEXITCODE"
    exit $LASTEXITCODE
}

Write-Host "Starting gdbserver on remote host..." -ForegroundColor Green
& ssh @sshArgs
if ($LASTEXITCODE -ne 0) {
    Write-Error "Remote start failed with exit code $LASTEXITCODE"
    exit $LASTEXITCODE
}

Write-Host ("gdbserver started on " + $HostName + ":" + $Port) -ForegroundColor Green
