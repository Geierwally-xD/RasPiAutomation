param(
    [string]$OldTag = 'JOKI_T1010_PIGPIO',
    [string]$NewTag = 'JOKI_T1011_PIPES',
    [string]$ReferencePaths = '',
    [string]$Description = '',
    [switch]$GeneralReport,
    [switch]$FromTaskEnvironment,
    [string]$ConfigPath = '',
    [string]$OutputDirectory = ''
)

$ErrorActionPreference = 'Stop'
if ($FromTaskEnvironment) {
    $OldTag = $env:CODEDIFF_OLD
    $NewTag = $env:CODEDIFF_NEW
    $ReferencePaths = [string]$env:CODEDIFF_REFERENCES
    $Description = [string]$env:CODEDIFF_DESCRIPTION
    $GeneralReport = $true
}
if ($OldTag -ne 'JOKI_T1010_PIGPIO' -or $NewTag -ne 'JOKI_T1011_PIPES' -or $ReferencePaths -or $Description) {
    $GeneralReport = $true
}
$root = Split-Path $PSScriptRoot -Parent
$outputDir = Join-Path $root 'Docu\CodeDiff'
$projectName = 'RasPiAutomation'
$extensions = @('.c', '.h')
$generatedSuffix = $GeneralReport
if ($ConfigPath) {
    $configFile = Get-Item -LiteralPath $ConfigPath -ErrorAction Stop
    $config = [IO.File]::ReadAllText($configFile.FullName) | ConvertFrom-Json
    foreach ($required in @('projectName','repositoryPath','outputDirectory','extensions')) {
        if (-not $config.$required) { throw "Missing configuration field: $required" }
    }
    $root = [IO.Path]::GetFullPath((Join-Path $configFile.DirectoryName $config.repositoryPath))
    $outputDir = [IO.Path]::GetFullPath((Join-Path $configFile.DirectoryName $config.outputDirectory))
    $projectName = [string]$config.projectName
    $extensions = @($config.extensions)
    if ($config.PSObject.Properties.Name -contains 'generatedSuffix') { $generatedSuffix = [bool]$config.generatedSuffix }
    foreach ($extension in $extensions) {
        if ($extension -notmatch '^\.[a-zA-Z0-9]+$') { throw "Invalid source extension: $extension" }
    }
    $GeneralReport = $true
}
$scopeLabel = ($extensions | ForEach-Object { '*' + $_ }) -join ';'
if (-not (Test-Path -LiteralPath $root -PathType Container)) { throw "Repository not found: $root" }
if ($OutputDirectory) { $outputDir = [System.IO.Path]::GetFullPath($OutputDirectory) }
foreach ($revision in @($OldTag, $NewTag)) {
    if ($revision -notmatch '^[A-Za-z0-9][A-Za-z0-9_./~^{}@+\-]*$') { throw "Invalid Git revision: $revision" }
}
$safeOld = $OldTag -replace '[^A-Za-z0-9_.-]', '_'
$safeNew = $NewTag -replace '[^A-Za-z0-9_.-]', '_'
$stem = "SourceDiff_${safeOld}-${safeNew}"
if ($generatedSuffix) { $stem += '_generated' }
$reportDate = Get-Date -Format 'yyyy-MM-dd'
$utf8 = New-Object System.Text.UTF8Encoding($false)
$references = @()
$fileCommitMessages = @{}

function Get-GitText([string[]]$Arguments) {
    $start = New-Object System.Diagnostics.ProcessStartInfo
    $start.FileName = 'git'
    $start.WorkingDirectory = $root
    $start.Arguments = (($Arguments | ForEach-Object { '"' + $_ + '"' }) -join ' ')
    $start.UseShellExecute = $false
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    $start.StandardOutputEncoding = $utf8
    $process = [System.Diagnostics.Process]::Start($start)
    $text = $process.StandardOutput.ReadToEnd()
    $errorText = $process.StandardError.ReadToEnd()
    $process.WaitForExit()
    if ($process.ExitCode -ne 0) { throw $errorText }
    return $text.Replace("`r`n", "`n")
}

function Escape-Html([string]$Text) {
    $visible = [regex]::Replace($Text, '[\x00-\x08\x0B\x0C\x0E-\x1F]', { param($match) '\x{0:X2}' -f [int][char]$match.Value })
    return [System.Net.WebUtility]::HtmlEncode($visible)
}

function Get-HighlightedLines([string]$Text, [string]$Path) {
    $pattern = '(?<comment>/\*[\s\S]*?\*/|//[^\n]*)|(?<string>"(?:\\.|[^"\\])*"|''(?:\\.|[^''\\])*'')|(?<keyword>\b(?:if|else|for|foreach|while|do|switch|case|default|break|continue|return|goto|static|const|unsigned|signed|char|int|long|short|void|size_t|ptrdiff_t|typedef|enum|struct|sizeof|NULL|true|false|param|function|throw|try|catch|finally|exit|endif|include|define|ifndef|ifdef)\b)|(?<number>\b(?:0x[\da-fA-F]+|\d+(?:\.\d+)?[uUlLfF]*)\b)|(?<function>\b[A-Za-z_]\w*(?=\s*\())|(?<constant>\b[A-Z][A-Z_\d]+\b)'
    if ($Path.EndsWith('.cs')) {
        $pattern = '(?<comment>/\*[\s\S]*?\*/|//[^\n]*)|(?<string>\$*@"(?:""|[^"])*"|@\$"(?:""|[^"])*"|"{3,}[\s\S]*?"{3,}|\$*"(?:\\.|[^"\\])*"|''(?:\\.|[^''\\])*'')|(?<keyword>\b(?:using|namespace|class|record|interface|struct|enum|public|private|protected|internal|static|readonly|const|partial|sealed|abstract|override|virtual|async|await|Task|var|new|return|if|else|switch|case|default|break|continue|for|foreach|while|do|in|out|ref|try|catch|finally|throw|null|true|false|void|bool|string|char|int|long|double|decimal|object|this|base|lock|is|as|typeof|nameof|get|set|init|yield|when|where|params|event|delegate)\b)|(?<number>\b(?:0x[\da-fA-F]+|\d+(?:\.\d+)?[uUlLfFdDmM]*)\b)|(?<function>\b[A-Za-z_]\w*(?=\s*\())'
    }
    if ($Path.EndsWith('.ps1')) {
        $pattern = '(?<comment>#[^\n]*)|(?<string>"(?:`.|[^"`])*"|''(?:''''|[^''])*'')|(?<keyword>\b(?:param|function|if|else|elseif|foreach|in|return|throw|exit|switch|try|catch)\b)|(?<variable>\$[\w:]+)|(?<number>\b\d+\b)|(?<function>\b[A-Za-z]+-[A-Za-z]+\b)'
    } elseif ($Path.EndsWith('.json')) {
        $pattern = '(?<comment>//[^\n]*)|(?<string>"(?:\\.|[^"\\])*")|(?<keyword>\b(?:true|false|null)\b)|(?<number>\b\d+\b)'
    } elseif ($Path.EndsWith('.md')) {
        $pattern = '(?<keyword>^#{1,6}[^\n]*)|(?<string>`[^`\n]+`)|(?<comment>^>[^\n]*)'
    } elseif ($Path.EndsWith('CMakeLists.txt')) {
        $pattern = '(?<comment>#[^\n]*)|(?<string>"(?:\\.|[^"\\])*")|(?<variable>\$\{[^}]+\})|(?<function>\b[A-Za-z_]\w*(?=\s*\())|(?<number>\b\d+\b)'
    } elseif ($Path.EndsWith('.sh')) {
        $pattern = '(?<comment>#[^\n]*)|(?<string>"[^"\n]*"|''[^''\n]*'')|(?<variable>\$[\w{}]+)|(?<keyword>\b(?:if|then|else|fi|case|esac|exit|do|done)\b)'
    }
    $builder = New-Object System.Text.StringBuilder
    $offset = 0
    foreach ($match in [regex]::Matches($Text, $pattern, [System.Text.RegularExpressions.RegexOptions]::Multiline)) {
        [void]$builder.Append((Escape-Html $Text.Substring($offset, $match.Index - $offset)))
        $kind = @('comment','string','keyword','number','function','constant','variable') | Where-Object { $match.Groups[$_].Success } | Select-Object -First 1
        $parts = $match.Value.Split([char]10)
        for ($part = 0; $part -lt $parts.Count; $part++) {
            if ($part -gt 0) { [void]$builder.Append("`n") }
            [void]$builder.Append('<span class="tok-' + $kind + '">' + (Escape-Html $parts[$part]) + '</span>')
        }
        $offset = $match.Index + $match.Length
    }
    [void]$builder.Append((Escape-Html $Text.Substring($offset)))
    return ,$builder.ToString().Split([char]10)
}

function Get-CSharpPurpose([string]$Path, [string]$Code) {
    switch -Regex ($Path) {
        '^JokiAutomation\.Tests/Regression/AudioPipelineLifetimeRegressionTests\.cs$' { return 'Audio-Lifetime-Regressionstest: prueft, dass ein eingereihter Profilauftrag die CLI nicht vor der konkreten Audio-Completion Erfolg melden laesst und dass parallele Bild-/PTZ-Arbeit waehrenddessen moeglich bleibt. Das ist ein Testfall, kein eigener Testlauf.' }
        '^JokiAutomation\.Tests/Regression/AudioProfileRegressionTests\.cs$' { return 'Audio-Profil-Regressionstest: sichert die Profilzuordnung und die Fehler-/Completion-Reaktion gegen erneute Fehler ab. Der konkrete Marker benennt die betroffenen Assertions; Vorhandensein des Tests allein ist kein PASS-Nachweis.' }
        '^JokiAutomation\.Tests/Unit/AudioProfile' { return 'Audio-Profil-Unit-Test: prueft Profilnamen, Indizes oder Dispatch-Verhalten isoliert von echter Hardware. Die geaenderten Assertionen zeigen, welcher Mappingvertrag erhalten bleiben soll.' }
        '^JokiAutomation\.Tests/Hardware/Audio' { return 'Audio-Hardwaretest/-Parser: verarbeitet reale AudioControl-Logs beziehungsweise prueft den Hardwarepfad. Tests koennen Geraetezugriff voraussetzen; dieser Marker belegt keinen ausgefuehrten Hardwaretest.' }
        '^JokiAutomation\.Tests/' { return 'Testcode: Die Aenderung ergaenzt oder aktualisiert automatisierte Pruefungen. Die konkreten Testnamen und Assertions im Marker zeigen das Verhalten; das Hinzufuegen eines Tests ist kein Testergebnis.' }
        '^CanonPtzCommon/AutoTrackingStatus\.cs$' { return 'Canon-PTZ-Datenmodell: buendelt den vom RA-AT001 gemeldeten Tracking-, Autozoom-, Sensitivitaets- und Zielauswahlstatus, damit UI und Controller denselben typisierten Status verwenden.' }
        '^CanonPtzCommon/CameraCommand\.cs$' { return 'Canon-PTZ-Befehlsmodell: benennt die vom Controller unterstuetzten Aktionen als Enum, statt Befehle als freie Zeichenketten zwischen UI und Geraet zu verteilen.' }
        '^CanonPtzCommon/CameraConfig\.cs$' { return 'Canon-PTZ-Konfiguration: kapselt Verbindungs- und Funktionsoptionen der Kamera. Aenderungen wirken auf die gemeinsame Konfigurationsquelle, aus der Controller und UI ihre Betriebswerte beziehen.' }
        '^CanonPtzCommon/CameraControlState\.cs$' { return 'Canon-PTZ-Zustandsmodell: repraesentiert, ob und in welchem Zustand die Kamerasteuerung verfuegbar ist, damit UI-Aktionen nicht allein aus lokalen Button-Zustaenden abgeleitet werden.' }
        '^CanonPtzCommon/CameraPosition\.cs$' { return 'Canon-PTZ-Positionsmodell: transportiert Positions-/Presetwerte zwischen Polling, Controller und Anzeige in einer gemeinsamen typisierten Struktur.' }
        '^CanonPtzCommon/CameraStatus\.cs$' { return 'Canon-PTZ-Statusmodell: gibt den Kamerazustand als gemeinsame Datenstruktur an Aufrufer zurueck.' }
        '^CanonPtzCommon/CommandResult\.cs$' { return 'Canon-PTZ-Ergebnisvertrag: buendelt Erfolg, Fehler und optionale Rueckgabedaten, sodass Aufrufer Controllerfehler auswerten koennen, ohne Exceptions als normale Statuswerte zu missbrauchen.' }
        '^CanonPtzCommon/IAutoTrackingService\.cs$' { return 'Tracking-Abstraktion: legt die Funktionen fuer Statusabfrage und Steuerung fest, damit UI und Tests nicht an eine konkrete Canon-Implementierung gekoppelt sind.' }
        '^CanonPtzCommon/ICanonPtzController\.cs$' { return 'PTZ-Controller-Vertrag: vereinheitlicht Kameraaktionen hinter einer Schnittstelle. So koennen Canon-Controller und alternative/fake Implementierungen dieselben Aufrufstellen bedienen.' }
        '^CanonPtzCommon/NetworkCfgReader\.cs$' { return 'Konfigurationsleser: uebernimmt das Einlesen und Validieren der Netzwerk-/Kameraeinstellungen aus Network.cfg, damit fehlerhafte oder fehlende Werte kontrolliert behandelt werden.' }
        '^CanonPtzCommon/PositionPollingService\.cs$' { return 'Positionsdienst: kapselt das wiederholte asynchrone Abfragen der Kamera und die Weitergabe neuer Positionen. Das trennt Polling-Timing und Abbruch von der Darstellung.' }
        '^CanonPtzCommon/SharedPresetState\.cs$' { return 'Gemeinsamer Preset-Zustand: synchronisiert Presetdaten zwischen Hauptanwendung und separater PTZ-Oberflaeche, statt zwei unabhaengige UI-Kopien zu pflegen.' }
        '^CanonPtzCommon/SharedSessionState\.cs$' { return 'Gemeinsamer Sitzungszustand: stellt Verbindungs-/Steuerungsdaten fuer mehrere Canon-PTZ-Komponenten bereit, damit Remote-Fenster und Hauptanwendung dieselbe Sitzung abbilden.' }
        '^CanonPtzCommon/TrackInfo\.cs$' { return 'Tracking-Datenmodell: traegt Informationen zu einem erkannten/verfolgten Ziel zwischen Canon-Trackingdienst und Aufrufer.' }
        '^CanonRemoteControl/CanonCrn100Controller\.cs$' { return 'Canon-CR-N100-Adapter: uebersetzt die gemeinsame PTZ-Schnittstelle in konkrete Netzwerkbefehle fuer den Canon-Controller und kapselt dessen Protokoll-/Verbindungsdetails.' }
        '^CanonRemoteControl/XcCanonPtzController\.cs$' { return 'Canon-XC-Adapter: bindet den herstellerspezifischen XC-Controller an den gemeinsamen PTZ-Vertrag an, damit die Oberflaeche nicht direkt von SDK-Aufrufen abhaengt.' }
        '^CanonRemoteControl/Services/AutoTrackingService\.cs$' { return 'Canon-Auto-Tracking-Adapter: liest und steuert die Tracking-Funktion ueber die Kameraanbindung und uebersetzt deren Antwort in die gemeinsamen Tracking-Datenmodelle.' }
        '^CanonRemoteControl/GlobalHotKeyManager\.cs$' { return 'Hotkey-Verwaltung: registriert und entfernt systemweite Tastenkombinationen fuer die separate PTZ-Bedienung; Aenderungen beeinflussen Registrierung und Lebensdauer dieser Eingaben.' }
        '^CanonRemoteControl/GlobalKeyboardHook\.cs$' { return 'Tastatur-Hook: vermittelt Tastaturereignisse an die PTZ-Fernbedienung. Der Block bestimmt, welche Eingaben erkannt, weitergereicht oder freigegeben werden.' }
        '^CanonRemoteControl/MainWindow\.xaml\.cs$' { return 'PTZ-Fernbedienungsfenster: verbindet Bedienaktionen mit Canon-Controller, Tracking und gemeinsamem Zustand. Die konkrete Markerzeile zeigt, welche Aktion oder welcher Lebenszykluspfad geaendert wurde.' }
        '^CanonRemoteControl/PositionPollingService\.cs$' { return 'PTZ-Fernbedienungs-Polling: aktualisiert Positionsdaten fuer das separate Fenster. Die Aenderung betrifft Pollingintervall, Abbruch oder Zustandsweitergabe gemaess dem gezeigten Block.' }
        '^CanonRemoteControl/.*Overlay\.xaml\.cs$' { return 'PTZ-Overlay: zeigt Status/Position ueber dem Bedienfenster an. Der Block aendert die Darstellung oder deren Aktualisierung, nicht die Kamera-Protokolllogik selbst.' }
        '^CanonRemoteControl/HelpDialog\.xaml\.cs$' { return 'Hilfe-Dialog der PTZ-Fernbedienung: die Aenderung betrifft Aufbau, Inhalt oder Lebenszyklus des Hilfefensters.' }
        '^CanonRemoteControl/App\.xaml\.cs$' { return 'Einstieg der separaten PTZ-Anwendung: Initialisierung und Beenden bestimmen, wann Fenster, Controller und gemeinsame Sitzungsressourcen angelegt oder freigegeben werden.' }
        '^JokiAutomation/ATEMControl\.cs$' { return 'ATEM-Integration: steuert den Blackmagic-Switcher. Der Block betrifft den konkreten Verbindungs-, Eingangs- oder Umschaltpfad; SDK-Vertragsaenderungen muessen mit dem dazu passenden SDK-Typen konsistent bleiben.' }
        'ATEM_SDK/.+\.cs$' { return 'Mitgefuehrte Blackmagic-ATEM-SDK-Beispieldatei. Die Commits ordnen sie der SDK-/ATEM-Integration zu; es handelt sich nicht um den JoKiAutomation-Szenenablauf. Aenderungen sind als Herstellerbeispiel beziehungsweise SDK-Bestand zu lesen, nicht als projektindividueller Audio-Fix.' }
        '^BMDSwitcherAPI\.cs$' { return 'Entfernte lokale BMDSwitcherAPI-Interop-Definition. Der Versionsverlauf ordnet die Bereinigung der ATEM-SDK-Integration zu; die Anwendung soll nicht gleichzeitig eine lokale Schatten-API und die eingebundene SDK-Definition verwenden. Die passende Ersatzreferenz ist im jeweiligen ATEMControl-/Projektdatei-Diff nachzuverfolgen.' }
        '^JokiAutomation/Form1\.AudioProfile\.cs$' { return 'Audio-Profilpfad der WinForms-Anwendung: ordnet Profilnamen GUI/CLI-Indizes zu, sendet den RasPi-Befehl und protokolliert dessen Abschluss. Der Abnahmebericht passt zu diesem Pfad nur, wenn der Marker Audio-Dispatch, Completion oder Profilabschluss betrifft.' }
        '^JokiAutomation/AudioMix\.cs$' { return 'AudioMix-Adapter: bildet Windows-Aufrufe auf Audio-Kommandos an den Raspberry Pi ab. Der Block ist fuer Profil-ID, Transport und Fehlerweitergabe relevant.' }
        '^JokiAutomation/RasPi\.Async\.cs$' { return 'Asynchroner Raspberry-Aufruf: trennt SSH-Ausfuehrung und Audio-/Szenen-Pipeline vom aufrufenden UI-/CLI-Pfad. Fuer Audio-Lifecycle-Marker ist entscheidend, ob der konkrete Auftrag nur eingereiht oder bis zur Completion abgewartet wird.' }
        '^JokiAutomation/RasPi\.cs$' { return 'Raspberry-Steuerungsadapter: kapselt den Prozess-/SSH-Aufruf und liefert dessen Ergebnis an die Windows-Anwendung zurueck. Der Marker zeigt die konkrete Aenderung an Argumenten, Start, Timeout oder Fehlerbehandlung.' }
        '^JokiAutomation/Program\.cs$' { return 'CLI-/Prozess-Einstieg: verarbeitet Kommandozeilenaufrufe und liefert den Prozessstatus. Aenderungen an Dispatch oder Exitcode bestimmen, ob der Aufrufer Erfolg erst nach den erforderlichen Teilaufgaben sieht.' }
        '^JokiAutomation/Form1(\.Designer)?\.cs$' { return 'Hauptfenster der JoKiAutomation: verbindet Szenenauswahl und UI-Aktionen mit Audio, Kamera/PTZ und ATEM. Der Block zeigt die geaenderte Zuordnung oder Aufrufreihenfolge; bei parallelen Teilaufgaben ist der Abschluss am Command-Ende entscheidend.' }
        '^JokiAutomation/PositionControl\.cs$' { return 'Szenen-/Positionssteuerung: ordnet Positions- oder Szenenkommandos den Video-/PTZ-Aktionen zu. Im Zusammenspiel mit Audio und ATEM ist die Reihenfolge wichtig: unabhaengige Aktionen koennen parallel laufen, bevor der Szenenaufruf abgeschlossen wird.' }
        '^JokiAutomation/PositionMoveResult\.cs$' { return 'Ergebnisvertrag fuer Positionsbewegungen: macht Rueckmeldung wie Erfolg, Abbruch oder Fehler explizit, damit Szenenlogik nicht allein aus einem gestarteten Move auf dessen Abschluss schliesst.' }
        '^JokiAutomation/DropboxApiUploader\.cs$' { return 'Dropbox-Uploader: kapselt Upload und API-Kommunikation fuer Protokoll-/Dateiablage. Der Marker aendert die konkrete Anfrage-, Datei- oder Fehlerbehandlung; er gehoert nicht zum PTZ- oder Audio-Completion-Pfad.' }
        '^JokiAutomation/LogData\.cs$' { return 'Logging-Komponente: formatiert und schreibt Diagnoseinformationen. Aenderungen an diesem Block betreffen Nachvollziehbarkeit und Fehlerprotokollierung, nicht automatisch den fachlichen Ablauf.' }
        '^JokiAutomation/EventTimer\.cs$|^EventTimer/' { return 'EventTimer-Komponente: behandelt zeitgesteuerte Ereignisse beziehungsweise deren Uebergabe an JoKiAutomation. Der Block bestimmt Ausloeser, Startparameter oder Lifecycle; EventTimer-Ergebnisse aus einem Bericht gelten nur fuer die dort protokollierten Laeufe.' }
        '^JokiAutomation/InfraredControl\.cs$' { return 'Infrarotsteuerung: erzeugt oder dispatcht IR-Kommandos. Die Aenderung betrifft den angegebenen Befehlspfad und ist unabhaengig von Canon-PTZ und dem Audio-Profil-Lifecycle.' }
        '^JokiAutomation/ROKU_TV_Remote\.cs$' { return 'Roku-Fernbedienung: bildet TV-Kommandos auf den Roku-Netzwerkpfad ab. Der konkrete Block veraendert Befehl, Zustandsabfrage oder Fehlerbehandlung dieses Geraets.' }
        '^JokiAutomation/Properties/AssemblyInfo\.cs$|^EventTimer/AssemblyInfo\.cs$|^CanonRemoteControl/Properties/' { return 'Projektmetadaten/Assembly-Informationen: versioniert oder konfiguriert das jeweilige .NET-Artefakt. Dieser Block aendert keine Laufzeitsteuerung.' }
        '^JokiAutomation/JokiAutomation/' { return 'Verschobener/entfernter doppelter Quellpfad innerhalb des Projekts. Die Aenderung konsolidiert Dateien im eigentlichen Projektverzeichnis; sie ist keine eigenstaendige Funktionsaenderung.' }
        default { return 'C#-Aenderung im JoKiAutomation-Repository. Zweck und Wirkung sind am geaenderten Member sowie an den darunter genannten dateibezogenen Commits zu erkennen; es wird kein nicht belegter Bezug zum Audio-Abnahmebericht unterstellt.' }
    }
}

function Get-BaseReason([string]$Path, [string]$Code) {
    if ($GeneralReport -and $Path.EndsWith('.cs')) {
        $purpose = Get-CSharpPurpose $Path $Code
        $addedAll = @([regex]::Matches($Code, '(?m)^\+[^\S\r\n]*(.+)$') | ForEach-Object { $_.Groups[1].Value.Trim() } | Where-Object { $_ -and $_ -notmatch '^(//|///|/\*|\*|using\b|namespace\b|\{\}?$|\}$)' })
        $removedAll = @([regex]::Matches($Code, '(?m)^-[^\S\r\n]*(.+)$') | ForEach-Object { $_.Groups[1].Value.Trim() } | Where-Object { $_ -and $_ -notmatch '^(//|///|/\*|\*|using\b|namespace\b|\{\}?$|\}$)' })
        $priorityPattern = '^(?:(?:public|private|protected|internal|static|async|sealed|partial|override|virtual|readonly|abstract|record|new|extern|unsafe|volatile)\s+)*(?:(?:class|interface|enum|struct|record)\s+|[\w<>,?.\[\]]+\s+\w+\s*(?:\(|\{|=>|=|;))'
        $focusPattern = switch -Regex ($Path) {
            'Form1\.AudioProfile\.cs$' { 'ApplyAudioProfile|SendAudioProfile|QueueAudioProfile|AwaitPendingAudio|ExecuteAudioAsync|AUDIO_PROFILE_COMPLETED'; break }
            'RasPi\.Async\.cs$' { 'Audio|Pipeline|Execute.*Async|Command.*Async|RasPi.*Async|Completion'; break }
            'Program\.cs$' { 'CommandInterpreterAsync|AwaitPendingAudio|CommandExitCodes|SingleInstanceCommandServer|TryRunShutdownTestHostMode'; break }
            'AudioPipelineLifetimeRegressionTests\.cs$' { 'PowerPointLike|Audio|Completion|Executor'; break }
            'ATEMControl\.cs$' { 'Semaphore|Transition|Switcher|Input|Connect|Disconnect'; break }
            'PositionControl\.cs$' { 'Position|Preset|PTZ|Camera|Move|Zoom'; break }
            default { '(?!)' }
        }
        $addedFocused = @($addedAll | Where-Object { $_ -match $focusPattern })
        $removedFocused = @($removedAll | Where-Object { $_ -match $focusPattern })
        $focusMembers = switch -Regex ($Path) {
            'Form1\.AudioProfile\.cs$' { @('QueueAudioProfileByNameAsync','QueueAudioProfileDispatch','AwaitPendingAudioCompletionAsync','SendAudioProfileAsync','ApplyAudioProfileByNameAsync'); break }
            'RasPi\.Async\.cs$' { @('RasPiExecuteAsync','ExecuteAudioAsync','RasPiCommandExecutor','RunRasPiCommandAsync'); break }
            'Program\.cs$' { @('CommandInterpreterAsync','AwaitPendingAudioCompletionAsync','SingleInstanceCommandServer','TryRunShutdownTestHostMode'); break }
            'AudioPipelineLifetimeRegressionTests\.cs$' { @('PowerPointLike_PredigtRequest_SurvivesFormAndCliCompletion','FakeRasPiCommandExecutor','AwaitPendingAudioCompletionAsync'); break }
            default { @() }
        }
        if ($focusMembers.Count) {
            $addedFocused = @()
            $removedFocused = @()
            foreach ($member in $focusMembers) {
                $addedMember = $addedAll | Where-Object { $_ -match ('\b' + [regex]::Escape($member) + '\b') } | Select-Object -First 1
                $removedMember = $removedAll | Where-Object { $_ -match ('\b' + [regex]::Escape($member) + '\b') } | Select-Object -First 1
                if ($addedMember) { $addedFocused += $addedMember }
                if ($removedMember) { $removedFocused += $removedMember }
            }
        }
        $added = @(@($addedFocused) + @($addedAll | Where-Object { $_ -match $priorityPattern }) + @($addedAll) | Select-Object -Unique | Select-Object -First 3)
        $removed = @(@($removedFocused) + @($removedAll | Where-Object { $_ -match $priorityPattern }) + @($removedAll) | Select-Object -Unique | Select-Object -First 3)
        if (-not $added.Count -and $addedAll.Count) { $added = @($addedAll | Select-Object -First 2) }
        if (-not $removed.Count -and $removedAll.Count) { $removed = @($removedAll | Select-Object -First 2) }
        $detail = @()
        if ($added.Count) { $detail += 'Neu: ' + (($added | ForEach-Object { if ($_.Length -gt 170) { $_.Substring(0,167) + '...' } else { $_ } }) -join ' | ') }
        if ($removed.Count) { $detail += 'Entfernt/ersetzt: ' + (($removed | ForEach-Object { if ($_.Length -gt 170) { $_.Substring(0,167) + '...' } else { $_ } }) -join ' | ') }
        $reason = 'Zweck: ' + $purpose
        if ($detail.Count) { $reason += ' Konkrete Markerzeilen: ' + ($detail -join '. ') + '.' }
        $messages = $fileCommitMessages[$Path]
        if ($messages) { $reason += ' Aenderungsnachweis in dateibezogenen Commits: ' + $messages + '.' }
        $audioRelated = $Path -match 'AudioProfile|AudioMix|AudioPipelineLifetime|AudioProfileRegression|RasPi\.Async|RasPi\.cs|Program\.cs|Form1\.cs' -and ($Code -match 'QueueAudioProfile|AwaitPendingAudio|_pendingAudioCompletionTask|_audioPipelineTail|ExecuteAudioAsync|AUDIO_PROFILE_COMPLETED|AUDIO_DISPATCH_QUEUED|AudioPipelineLifetime|AudioProfileRegression')
        if ($audioRelated) {
            $reason += ' Abnahmebezug: Der Bericht vom 22.10.2026 beschreibt den Fehler queued statt completed und bestaetigt den konkreten Profil-2-Lauf; er belegt nicht automatisch jeden Marker oder alle Szenen.'
        } else {
            $reason += ' Abnahmegrenze: Der vorgelegte Audio-Pipeline-Abnahmebericht ist fuer diesen Marker nicht unmittelbar einschlaegig.'
        }
        return $reason
    }
    if ($GeneralReport -and $Path -notmatch '\.(c|h)$') {
        $reason = if ($Description.Trim()) { 'Angegebener Aenderungsgrund: ' + $Description.Trim() } else { 'Ein fachlicher Aenderungsgrund wurde nicht manuell angegeben.' }
        $messages = $fileCommitMessages[$Path]
        if ($messages) { $reason += ' Commit-Kontext fuer diese Datei: ' + $messages }
        return $reason + ' Referenz- und Abnahmeaussagen werden nur dort zugeordnet, wo Dateipfad und Marker fachlich passen.'
    }
    switch -Regex ($Path) {
        '^AudioControl/AudioControl\.c$' {
            if ($Code -match 'AC_RotateLogIfNeeded|\.log\.1|MAX.*LOG|LOG.*SIZE') { return 'Die Audio-Logdatei wird vor dem Schreiben groessenbegrenzt rotiert. So bleiben aktuelle Diagnosen verfuegbar, ohne dass wiederholte Mixeraufrufe die Logdatei unbegrenzt wachsen lassen.' }
            if ($Code -match 'sys/file\.h|sys/stat\.h|stdarg\.h') { return 'Die zusaetzlichen Systemheader stellen Dateisperren, Dateistatus und variadische Logformatierung fuer den abgesicherten UART-Zugriff und die strukturierte Diagnose bereit.' }
            if ($Code -match 'void AC_Log\(') { return 'AC_Log fuehrt zeitgestempelte AudioControl-Meldungen an einer Stelle zusammen. Damit lassen sich Prozess, Profilanforderung und Kommunikationsfehler im Raspberry-Log gemeinsam verfolgen.' }
            if ($Code -match 'flock\(') { return 'Der Audiomixer-Zugriff wird mit flock exklusiv und nicht blockierend geschuetzt. Ein zweiter Prozess kann waehrend eines laufenden UART-Kommandos weder Antwortbytes abholen noch Profilbefehle dazwischen schreiben; der Lock-Fehler wird als Kommunikationsfehler behandelt.' }
            if ($Code -match 'REQUESTED_PROFILE') { return 'Profilindex und vier angeforderte Sollwerte werden als strukturierter Datensatz protokolliert. Damit ist im RasPi-Log sichtbar, welches Profil der Aufrufer tatsaechlich angefordert hat, bevor der Mixerzustand gelesen wird.' }
            if ($Code -match 'MUTESTATE') { return 'Der Mutestate-Befehl wird nur nach erfolgreichem Oeffnen des UART gesendet. Anschliessend wird AC_close aufgerufen, damit ein Portfehler nicht durch einen Folge-Write verdeckt und der Deskriptor kontrolliert freigegeben wird.' }
            if ($Code -match 'AC_PROFILE_BEGIN|AC_PROFILE_READ_BEFORE|AC_PROFILE_READ_AFTER|AC_PROFILE_VERIFY|AC_PROFILE_SUCCESS') { return 'AC_Profile protokolliert Anforderung, Vorher-/Nachher-Ruecklesung und Verifikation getrennt. Erfolg und aktives Profil werden erst nach dem Vierkanal-Soll-Ist-Abgleich gesetzt; die Abnahme belegt einen dokumentierten Profil-2-Lauf, nicht alle Profile.' }
            if ($Code -match 'AC_WriteAllWith|AC_WRITE_OK|AC_WRITE_FAIL|EINTR') { return 'Der UART-Write wird durch eine vollstaendige Schreibschleife ersetzt: Teilwrites werden fortgesetzt, EINTR wiederholt, Null-Write und echte Fehler schlagen fehl. AC_SUCCESS gilt erst, wenn alle angeforderten Bytes uebertragen sind.' }
            if ($Code -match 'AC_CLOSE_FAIL|close\(uart0_filestream\)|uart0_filestream = -1') { return 'AC_close zentralisiert die UART-Freigabe: der Deskriptor wird geprueft, Schliessfehler werden protokolliert und der globale Streamstatus auf geschlossen gesetzt.' }
            if ($Code -match 'AUDIO RESET|AC_write\(&RESET') { return 'Der Reset wird nur nach erfolgreichem Oeffnen des UART gesendet; ein Open-Fehler verhindert den Write. AC_close gibt den Port frei und das Reset-Ergebnis wird als eigener Status protokolliert.' }
            if ($Code -match 'volumeSteps\[[0-3]\].*AC_SUCCESS|AC_SUCCESS.*volumeSteps\[[0-3]\]') { return 'AC_Control sendet jeden Kanal-Fadeschritt nur, solange der vorangegangene UART-Write erfolgreich war. Nach einem Fehler wird die Fade-Schleife beendet; weitere Kanal-Kommandos koennen den urspruenglichen Fehler nicht ueberdecken.' }
            if ($Code -match 'AUDIO RESET|AC_write\(&RESET') { return 'Der Reset wird nur nach erfolgreichem Oeffnen des UART gesendet; der Fehlerpfad schliesst den Port und gibt den urspruenglichen Kommunikationsfehler zurueck, statt auf einem ungueltigen Deskriptor weiterzuschreiben.' }
            if ($Code -match 'AC_close\(|close\(uart0_filestream\)|uart0_filestream = -1') { return 'Der neue AC_close-Helfer zentralisiert das Schliessen des UART: er prueft den Deskriptor, protokolliert einen Schliessfehler und setzt den globalen Streamstatus auf geschlossen. Aufrufer muessen dadurch die Deskriptor- und close-Logik nicht mehr duplizieren.' }
            if ($Code -match 'uart0_filestream < 0|uart0_filestream >= 0|uart0_filestream == -1') { return 'Die UART-Gueltigkeitspruefung verwendet < 0 als Fehlerbedingung. Damit bleibt Deskriptor 0 gueltig; dies verhindert, dass der seltene, aber zulaessige Fall eines auf 0 geoeffneten Ports faelschlich als nicht initialisiert behandelt wird.' }
            if ($Code -match 'AC_GetVolumeChannel|AC_ParseVolumeResponse|AC_VOLUME_RESPONSE|tcflush|volumeState\[channel') { return 'Vor jeder Kanalabfrage werden alte UART-Daten verworfen; nur eine vollstaendige und zum angefragten VOL-Kanal passende Antwort wird in volumeState uebernommen. Ungueltige Antworten werden begrenzt erneut gelesen und sonst als Fehler gemeldet.' }
            if ($Code -match 'AC_PROFILE_READ_AFTER|AC_PROFILE_VERIFY|AC_PROFILE_BEGIN|execute and verify') { return 'AC_Profile liest nach dem Fade alle vier Mixer-Kanaele erneut aus und vergleicht Ist- mit Sollwerten. Erfolg und activeAudioProfile werden erst nach match=1 gesetzt; ein Kommunikationsfehler loest keinen Profil-Retry aus. Abnahmebezug: Der Bericht bestaetigt einen konkreten Profil-2-Lauf, nicht alle Hardwarepfade.' }
            if ($Code -match 'AC_WriteAllWith|AC_WRITE_OK|AC_WRITE_FAIL|AC_SystemWrite|EINTR') { return 'UART-Schreiben behandelt Teiluebertragungen, EINTR und Null-Write. AC_SUCCESS wird erst nach Uebertragung aller angeforderten Bytes geliefert, damit ein partiell gesendeter Mixerbefehl nicht als vollstaendig gilt.' }
            if ($Code -match 'AC_Log\(|REQUESTED_PROFILE|AC_PROFILE_|AC_VOLUME_') { return 'Die geaenderten AudioControl-Meldungen protokollieren Profilanforderung, Kanalantwort und Abschluss mit Prozessbezug. Das dient der Diagnose, ersetzt aber nicht den tatsaechlichen Soll-Ist-Vergleich.' }
            if ($Code -match 'AC_Request\(|volumeRequest|AC_PROFILE') { return 'Der Profilablauf bewahrt den ersten Fehler aus Sollwertanforderung, Mixer-Ruecklesung oder Fade-Schreiben und beendet die Folgeoperationen kontrolliert. Erst ein vollstaendiger Ablauf kann anschliessend in AC_Profile verifiziert werden.' }
            if ($Code -match 'AC_PROFILE_READ_AFTER|AC_PROFILE_VERIFY|AC_PROFILE_BEGIN|execute and verify') { return 'Der Profilabschluss wird vom blossen Sendeerfolg auf einen bestaetigten Mixerzustand umgestellt. Alle vier Ruecklesewerte muessen exakt dem angeforderten Profil entsprechen; nur bei gueltiger Kommunikation und Abweichung ist ein zweiter Versuch erlaubt. Das aktive Profil wird erst nach erfolgreicher Verifikation fortgeschrieben. Bezug: Abnahme S. 5, Profil 2 mit 0,50,0,31 und match=1.' }
            if ($Code -match 'AC_GetVolumeChannel|AC_ParseVolumeResponse|AC_VOLUME_RESPONSE|tcflush|volumeState\[channel') { return 'Alte, unvollstaendige oder kanalverkehrte UART-Antworten duerfen keinen gueltigen Mixerzustand vortaeuschen. Der neue Empfangspfad leert den Eingang, liest begrenzt wiederholt und uebernimmt nur streng gepruefte Antworten fuer den angefragten Kanal. Kommunikationsfehler bleiben Fehler, statt als Profilabweichung einen pauschalen Szenen-Retry auszuloesen.' }
            if ($Code -match 'AC_WriteAllWith|AC_WRITE_OK|AC_WRITE_FAIL|AC_SystemWrite') { return 'Ein einzelnes write() kann weniger Bytes als angefordert uebertragen. Der ausgelagerte Schreibhelfer behandelt Teiluebertragungen und EINTR; Erfolg wird erst nach vollstaendiger Uebertragung protokolliert. Damit ist Sendeerfolg praezise, aber weiterhin kein Ersatz fuer die nachfolgende Mixer-Verifikation.' }
            if ($Code -match 'flock|uartLock|LOCK_EX|LOCK_UN|AC_Lock|AC_Unlock|lockPath|lockFile') { return 'Der serielle Mixerzugriff erhaelt einen prozessuebergreifenden Lock. Oeffnen, Fehlerpfade und Schliessen geben die Ressource kontrolliert frei, damit parallele Aufrufer ihre Kommandos und Antworten nicht vermischen. Dies ist RasPi-seitige Absicherung, nicht die im Abnahmebericht beschriebene Windows-Task-Completion.' }
            if ($Code -match 'rotate|Rotate|MAX.*LOG|LOG.*SIZE|stat\(|\.log\.1|rename\(') { return 'Audio-Diagnoselogs werden groessenabhaengig rotiert. So bleiben aktuelle und vorherige Fehlerablaeufe nachvollziehbar, ohne die aktive Logdatei unbegrenzt wachsen zu lassen. Die Rotation ist eine Betriebsverbesserung und kein eigenstaendiger Nachweis der Fehlerbehebung.' }
            if ($Code -match 'AC_Request|REQUESTED_PROFILE|activeAudioProfile|AC_OUT_OF_RANGE') { return 'Das angeforderte Profil und seine vier Sollwerte werden nachvollziehbar protokolliert. Die Sollwertanforderung wird vom bestaetigten Istzustand getrennt, damit ein bloss angefordertes Profil nicht bereits als erfolgreich aktiv erscheint. Profilnummern und gespeicherte Profilwerte werden dadurch nicht neu definiert.' }
            if ($Code -match 'result == AC_SUCCESS|result != AC_SUCCESS|AC_TX_FAILED|AUDIO RESET') { return 'Der erste konkrete Kommunikationsfehler bleibt erhalten; weitere Ausgaben werden danach begrenzt oder abgebrochen. Ein Mixer-Reset bleibt ein explizites Kommando und wird nicht als pauschale Reparatur eines Profilwechsels eingesetzt.' }
            return 'AudioControl erhaelt die benoetigten Abhaengigkeiten und Hilfslogik fuer vollstaendiges Schreiben, validierte Ruecklesung, Portschutz und nachvollziehbare Profilabschluesse. Anlass ist das Fehlerbild eines scheinbar erfolgreichen ersten Audioprofil-Aufrufs; der Code wird unten unverkuerzt im Tag-Diff gezeigt.'
        }
        'AudioControlValidation\.c$' { return 'Parser, exakter Vierkanalvergleich und begrenzte Retry-Entscheidung werden als hardwarefreie Funktionen isoliert. Das macht falsche Kanalantworten, ungueltige Werte und die Grenze von maximal zwei Profilversuchen gezielt pruefbar, ohne einen Mixer anschliessen zu muessen.' }
        'AudioControlValidation\.h$' { return 'Die Parser- und Verifikationsschnittstellen werden fuer Produktion und Tests gemeinsam deklariert. Ein expliziter Entscheidungswert trennt Erfolg, zulaessigen Retry und endgueltigen Fehler.' }
        'AudioControlWrite\.c$' { return 'Der Schreibhelfer vervollstaendigt partielle Writes, wiederholt EINTR und behandelt Null-Write oder echte Fehler als AC_TX_FAILED. Ein injizierbarer Write-Aufruf erlaubt reproduzierbare Tests der zuvor unbeachteten Teiluebertragung.' }
        'AudioControlWrite\.h$' { return 'Die Write-Schnittstelle liefert neben dem Ergebnis auch uebertragene Bytezahl und errno. Der Funktionszeiger entkoppelt die Tests vom echten UART-Dateideskriptor.' }
        'AudioControl\.h$' { return 'Eindeutige Fehlercodes fuer ungueltige Antworten und fehlgeschlagene Profilverifikation sowie Grenzen fuer Versuche und Einschwingzeit machen das neue Verhalten explizit. Codes 39/40 werden nicht als unabhaengige Bitmasken interpretiert; der Abnahmebericht nennt die Fehlerweitergabe 38/39/40 auf Windows-Seite.' }
        '^tests/' { return 'Die neuen Regressionstests pruefen gueltige und fehlerhafte VOL-Antworten, exakten Profilvergleich, die Retry-Grenze sowie partielle, unterbrochene, fehlerhafte und Null-Writes. Das sind RasPi-C-Tests; die 65/65 Tests aus dem Abnahmebericht gehoeren zur getrennten Windows-Suite und werden hier nicht als eigene Ausfuehrung ausgewiesen.' }
        '^CMakeLists' {
            if ($Code -match 'AC_BUILD_HARDWARE_FREE_TESTS|add_test') { return 'Ein eigener CMake-Modus baut ausschliesslich die Audio-Validierungs- und Write-Tests. Der fruehe return vermeidet die Hardware- und Cross-Compile-Abhaengigkeiten der eigentlichen RasPi-Anwendung.' }
            return 'PTZ-Modus und konfigurierte IP werden beim Build aus network.cfg beziehungsweise der IP-Umgebung uebernommen und als Header bereitgestellt. Das verbindet Kameramodus und Startdiagnose mit der Konfiguration. Pruefgrenze: Die gezeigten Regexe sind mit ^ verankert; mehrzeilige Konfigurationsdateien benoetigen eine gesonderte Pruefung.'
        }
        '^PositionControl/' { return 'Bei aktiviertem PTZ-Kameramodus wird die historische lokale Bewegung in PC_Move uebersprungen. So soll die externe PTZ-Steuerung nicht zusaetzlich den alten Relais-/Zoom-Pfad ausloesen. Dies liegt ausserhalb des reinen Audio-Arbeitsauftrags und ist durch den Audio-Abnahmebericht nicht separat abgenommen.' }
        '^System_Raspberry/pigpio_adapter' { return 'Der Servo-Pfad setzt vor der Ausgabe den Pinmodus und begrenzt positive Pulsbreiten auf 500 bis 2500 Mikrosekunden. Das betrifft die Legacy-Hardwareansteuerung, nicht die Audio-Pipeline. Pruefhinweis: Pulsbreite 0 kehrt vor gpioServo() zurueck; ein Abschalten bereits laufender Pulse ist damit nicht belegt.' }
        '^System_Raspberry/system' { return 'Die konfigurierte IP wird ueber eine gemeinsame, puffergepruefte Systemschnittstelle bereitgestellt. Ungueltige Zielpuffer und abgeschnittene Ausgaben liefern einen Fehler statt eines scheinbar gueltigen Ergebnisses.' }
        '^main\.c$' { return 'Die beim Build hinterlegte IP wird beim regulaeren Programmstart ausgegeben. Das erleichtert die Zuordnung von Binary und Zielkonfiguration; es konfiguriert weder die Netzwerkschnittstelle noch prueft es die Erreichbarkeit des Raspberry.' }
        '^Command_Interpreter/' { return 'Der Kommandoeingang protokolliert PID, command und index. Damit laesst sich die Kette Windows-Aufruf zu Raspberry-Profil eindeutig verfolgen; genau diese COMMAND-Zeile mit command=30 index=114 ist im Abnahmebericht S. 5 als Ankunftsnachweis verwendet.' }
        'cmake_clean_rpi' { return 'Debug- und Release-Buildverzeichnisse werden getrennt ueber ihr jeweiliges CMake-Clean-Ziel bereinigt. Fehler brechen ab; ein nicht vorhandenes Verzeichnis wird uebersprungen.' }
        'cmake_configure_rpi' { return 'Der Buildtyp wird explizit auf Debug oder Release begrenzt und an CMake weitergereicht. Damit erhalten die getrennten Buildverzeichnisse eine nachvollziehbare Konfiguration statt eines impliziten Standardzustands.' }
        'deploy_raspi_binary' { return 'Ein eigener SSH/SCP-Ablauf uebertraegt das gebaute Binary, setzt Ausfuehrungsrechte und prueft den Dateieintrag. Schluessel- und Binary-Pfade werden vorher geprueft. Pruefgrenze: Das Ziel wird vor SCP geloescht; atomarer Austausch, Rollback oder Hardware-Funktionstest sind hier nicht implementiert.' }
        'start_gdbserver' { return 'Das bisherige Loeschen des GDBServer-Logs wird durch Rotation ab 5 MiB mit zwei Altdateien ersetzt. Damit bleiben vorherige Debug-Laeufe fuer die Fehleranalyse erhalten.' }
        'launch\.json|gdbserver-config' { return 'Remote-Debugging verwendet nun ausdruecklich das Debug-Binary aus build-rpi-debug. Die Startaufgabe wird an die neue Taskbezeichnung angepasst; stopAtEntry=false laesst den normalen Kommandoablauf ohne automatischen Einstiegsstopp laufen.' }
        'tasks\.json' { return 'Build, Clean, Release-Deployment und Remote-Debugging werden in getrennte, geordnete Tasks aufgeteilt. Debug und Release haben eigene Verzeichnisse; Update baut vor der Uebertragung. Das reduziert die Verwechslung von Debug-Binary, Release-Binary und Startaufgabe.' }
        'GitHub_Copilot_' { return 'Der Arbeitsauftrag dokumentiert das urspruengliche Audiofehlerbild, Akzeptanzkriterien und die Grenzen der beabsichtigten Korrektur. Er erklaert das Warum, ist aber weder Ausfuehrungsprotokoll noch eigenstaendige Abnahme; sein Audio-only-Scope deckt die weiteren Tag-Aenderungen nicht ab.' }
        '^Docu/' { return 'Historische Installations-, Debug- oder Laufzeitdateien werden aus dem Repository entfernt. Im Report bleibt die Loeschung nachvollziehbar; ein technischer Grund je Altdatei ist aus dem Tag-Diff und dem Audio-Abnahmebericht nicht belegt. Binaerinhalte werden nicht als Quelltext ausgegeben.' }
        default { return 'Diese Aenderung ist Bestandteil des exakten Tag-Vergleichs. Eine weitergehende fachliche Abnahme ist in den vorliegenden Audio-Unterlagen nicht belegt.' }
    }
}

function Get-Reason([string]$Path, [string]$Code) {
    $reason = Get-BaseReason $Path $Code
    if ($Path -match '\.(c|h)$') {
        $addedAll = @([regex]::Matches($Code, '(?m)^\+[^\S\r\n]*(.+)$') | ForEach-Object { $_.Groups[1].Value.Trim() } | Where-Object { $_ -and $_ -notmatch '^(//|/\*|\*|\{|\}|$)' })
        $removedAll = @([regex]::Matches($Code, '(?m)^-[^\S\r\n]*(.+)$') | ForEach-Object { $_.Groups[1].Value.Trim() } | Where-Object { $_ -and $_ -notmatch '^(//|/\*|\*|\{|\}|$)' })
        $focusPattern = if ($Path -match '^tests/') { '\b(test_|assert_|AC_ParseVolumeResponse|AC_GetProfileDecision|AC_WriteAllWith)' } else { '\b(AC_Log|AC_close|AC_WriteAllWith|AC_ParseVolumeResponse|AC_ProfileValuesMatch|AC_GetProfileDecision|AC_PROFILE_VERIFY|AC_PROFILE_READ_AFTER|SYSTEM_GetConfiguredIPAddress|PC_Move|RPI_ServoPulse|gpioSetMode|REQUESTED_PROFILE|flock|AC_Control|AC_Profile)\b' }
        $addedFocused = @($addedAll | Where-Object { $_ -match $focusPattern })
        $removedFocused = @($removedAll | Where-Object { $_ -match $focusPattern })
        $added = @(@($addedFocused) + @($addedAll | Where-Object { $_ -match '^\s*(?:extern\s+)?(?:static\s+)?(?:unsigned\s+)?(?:char|int|void|long|size_t|AC_[A-Za-z_]+|SYSTEM_[A-Za-z_]+|PC_[A-Za-z_]+)\s+[A-Za-z_]\w*\s*\(' }) + $addedAll | Select-Object -Unique | Select-Object -First 3)
        $removed = @(@($removedFocused) + @($removedAll | Where-Object { $_ -match '^\s*(?:extern\s+)?(?:static\s+)?(?:unsigned\s+)?(?:char|int|void|long|size_t|AC_[A-Za-z_]+|SYSTEM_[A-Za-z_]+|PC_[A-Za-z_]+)\s+[A-Za-z_]\w*\s*\(' }) + $removedAll | Select-Object -Unique | Select-Object -First 3)
        foreach ($entry in @(@{label='Eingefuegt';lines=$added},@{label='Ersetzt/entfernt';lines=$removed})) {
            if ($entry.lines.Count) {
                $snippets = @($entry.lines | ForEach-Object { if ($_.Length -gt 135) { $_.Substring(0,132) + '...' } else { $_ } })
                $reason += ' Konkrete Diffzeilen (' + $entry.label + '): ' + ($snippets -join ' | ') + '.'
            }
        }
    }
    return $reason
}

$oldCommit = (Get-GitText @('rev-parse','--verify','--end-of-options', "$OldTag^{commit}")).Trim()
$newCommit = (Get-GitText @('rev-parse','--verify','--end-of-options', "$NewTag^{commit}")).Trim()
foreach ($referencePath in $ReferencePaths.Split(';', [System.StringSplitOptions]::RemoveEmptyEntries)) {
    $candidate = $referencePath.Trim().Trim('"')
    if (-not [System.IO.Path]::IsPathRooted($candidate)) { $candidate = Join-Path $root $candidate }
    $resolved = (Get-Item -LiteralPath $candidate -ErrorAction Stop)
    if ($resolved.PSIsContainer) { throw "Reference must be a file: $candidate" }
    $extension = $resolved.Extension.ToLowerInvariant()
    if ($extension -eq '.pdf') {
        $pdfCommand = Get-Command pdftotext.exe -ErrorAction SilentlyContinue
        $pdfTool = if ($pdfCommand) { $pdfCommand.Source } else { Join-Path $env:ProgramFiles 'Git\mingw64\bin\pdftotext.exe' }
        if (-not (Test-Path -LiteralPath $pdfTool)) { throw 'PDF references require pdftotext.exe (PATH or Git installation).' }
        $referenceText = (& $pdfTool -enc UTF-8 -layout $resolved.FullName -) -join "`n"
        if ($LASTEXITCODE -ne 0) { throw "Cannot extract PDF: $candidate" }
    } elseif ($extension -in @('.txt','.md','.log')) {
        $referenceText = [System.IO.File]::ReadAllText($resolved.FullName, $utf8)
    } else { throw "Unsupported reference type: $extension. Use PDF, TXT, MD or LOG." }
    if (-not $referenceText.Trim()) { throw "Reference has no extractable text: $candidate" }
    $references += [pscustomobject]@{Id=('R{0:d2}' -f ($references.Count + 1));Path=$resolved.FullName;Text=$referenceText;Hash=(Get-FileHash -LiteralPath $resolved.FullName -Algorithm SHA256).Hash}
}
$stats = Get-GitText @('-c','core.quotepath=false','diff','--no-renames','--numstat',$oldCommit,$newCommit)
$document = New-Object System.Xml.XmlDocument
$document.PreserveWhitespace = $true
$report = $document.CreateElement('sourceDiffReport')
[void]$document.AppendChild($report)
foreach ($entry in @{oldTag=$OldTag;newTag=$NewTag;oldCommit=$oldCommit;newCommit=$newCommit;date=$reportDate}.GetEnumerator()) { $report.SetAttribute($entry.Key, $entry.Value) }
$highlighted = @{}
$fileIndex = 0
$hunkTotal = 0
$addedTotal = 0
$deletedTotal = 0
foreach ($stat in ($stats.TrimEnd().Split([char]10))) {
    if (-not $stat) { continue }
    $columns = $stat.Split([char]9)
    if ($columns.Count -ne 3) { throw "Unexpected numstat: $stat" }
    if ([IO.Path]::GetExtension($columns[2]) -notin $extensions) { continue }
    $fileIndex++
    $path = $columns[2]
    if ($GeneralReport) {
        $fileCommitMessages[$path] = (Get-GitText @('log','--format=%h %s',"${oldCommit}..${newCommit}",'--',$path)).Trim() -replace '\n', ' | '
    }
    $fileId = 'f{0:d2}' -f $fileIndex
    $file = $document.CreateElement('file')
    foreach ($entry in @{id=$fileId;path=$path;added=$columns[0];deleted=$columns[1]}.GetEnumerator()) { $file.SetAttribute($entry.Key, $entry.Value) }
    [void]$report.AppendChild($file)
    $diff = Get-GitText @('-c','core.quotepath=false','diff','--no-ext-diff','--no-textconv','--no-renames','--unified=5',$oldCommit,$newCommit,'--',$path)
    $file.SetAttribute('status', $(if ($diff -match '(?m)^deleted file mode') { 'geloescht' } elseif ($diff -match '(?m)^new file mode') { 'neu' } else { 'geaendert' }))
    if ($columns[0] -eq '-') {
        $file.SetAttribute('binary','true')
        $file.SetAttribute('reason',(Get-Reason $path ''))
        continue
    }
    $addedTotal += [int]$columns[0]
    $deletedTotal += [int]$columns[1]
    foreach ($side in @('old','new')) {
        $tag = if ($side -eq 'old') { $oldCommit } else { $newCommit }
        if (($side -eq 'old' -and $file.status -eq 'neu') -or ($side -eq 'new' -and $file.status -eq 'geloescht')) { continue }
        $source = Get-GitText @('show', "${tag}:$path")
        $highlighted["$fileId-$side"] = Get-HighlightedLines $source $path
    }
    $hunk = $null
    $hunkIndex = 0
    $oldLine = 0
    $newLine = 0
    foreach ($line in $diff.Split([char]10)) {
        if ($line -match '^@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@(.*)$') {
            $oldLine = [int]$Matches[1]
            $newLine = [int]$Matches[3]
            $hunkIndex++
            $hunkTotal++
            $hunk = $document.CreateElement('hunk')
            $hunk.SetAttribute('id',"$fileId-h$hunkIndex")
            $hunk.SetAttribute('marker',('M{0:d3}' -f $hunkTotal))
            $hunk.SetAttribute('header',$line)
            [void]$file.AppendChild($hunk)
            continue
        }
        if ($null -eq $hunk -or $line.Length -eq 0) { continue }
        $prefix = $line.Substring(0,1)
        if ($prefix -notin @(' ','+','-')) { continue }
        $row = $document.CreateElement('line')
        $row.SetAttribute('kind', $(if ($prefix -eq '+') { 'add' } elseif ($prefix -eq '-') { 'del' } else { 'context' }))
        if ($prefix -ne '+') { $row.SetAttribute('old',[string]$oldLine); $oldLine++ }
        if ($prefix -ne '-') { $row.SetAttribute('new',[string]$newLine); $newLine++ }
        $rawLine = $line.Substring(1)
        if ($rawLine -match '[\x00-\x08\x0B\x0C\x0E-\x1F]') {
            $row.SetAttribute('encoding','base64-utf8')
            $row.InnerText = [Convert]::ToBase64String($utf8.GetBytes($rawLine))
        } else {
            $row.InnerText = $rawLine
        }
        [void]$hunk.AppendChild($row)
    }
    foreach ($block in $file.SelectNodes('hunk')) {
        $code = ($block.SelectNodes('line') | Where-Object kind -ne 'context' | ForEach-Object {
            $lineText = if ($_.encoding -eq 'base64-utf8') { $utf8.GetString([Convert]::FromBase64String($_.InnerText)) } else { $_.InnerText }
            if ($_.kind -eq 'add') { '+ ' + $lineText } else { '- ' + $lineText }
        }) -join "`n"
        $block.SetAttribute('reason',(Get-Reason $path $code))
    }
}

$acceptance = $document.CreateElement('acceptance')
$acceptance.SetAttribute('source','I:\MyGitRepos\JoKiAutomation\Testreports\JokiAutomation_Audio_Pipeline_Lifetime_Fix_Abnahmebericht.pdf')
$acceptance.SetAttribute('date','2026-10-22')
$acceptance.SetAttribute('status','PASS im vorgelegten System-Abnahmebericht; keine pauschale Abnahme dieses Tag-Diffs')
$acceptance.InnerText = 'S. 1-2: Root Cause ist queued statt completed im Windows-Lifecycle. Konkrete Audio-Completion wird am Command-Ende abgewartet; Audio/PTZ bleiben parallel. S. 4: 65/65 hardwarefreie Windows-Tests und Debug/Release x64 erfolgreich; Raspberry-Code bei diesem Fix unveraendert. S. 5: 17:12:43 COMMAND command=30 index=114; Profil 2 requested=0,50,0,31; 17:13:22 READ_AFTER actual=0,50,0,31, VERIFY match=1, SUCCESS result=0; weiterer Erfolg 17:18:26. S. 6: zwei EventTimer-Handoffs erfolgreich. Rohlogs wurden fuer diesen Diff-Report nicht erneut ausgewertet; der Abnahmebericht bindet diese Laeufe nicht an die hier aufgeloesten Git-Commit-Hashes.'
[void]$report.AppendChild($acceptance)
$report.SetAttribute('files',[string]$fileIndex)
$report.SetAttribute('hunks',[string]$hunkTotal)
$report.SetAttribute('added',[string]$addedTotal)
$report.SetAttribute('deleted',[string]$deletedTotal)
[void][System.IO.Directory]::CreateDirectory($outputDir)
$xmlPath = Join-Path $outputDir "$stem.xml"
$settings = New-Object System.Xml.XmlWriterSettings
$settings.Indent = $true
$settings.Encoding = $utf8
$writer = [System.Xml.XmlWriter]::Create($xmlPath, $settings)
try { $document.Save($writer) } finally { $writer.Dispose() }
$document.Load($xmlPath)
$report = $document.DocumentElement

$html = New-Object System.Text.StringBuilder
[void]$html.Append(@'
<!doctype html>
<html lang="de"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>RasPiAutomation | Source-Diff T1010 zu T1011</title>
<style>
:root{--ink:#232b2d;--muted:#596469;--line:#cbd4d4;--accent:#006d65;--red:#a52c3c;--paper:#fafcfc}
*{box-sizing:border-box}html{scroll-padding-top:18px}body{margin:0;background:var(--paper);color:var(--ink);font:15px/1.6 "Segoe UI",sans-serif;letter-spacing:0}a{color:var(--accent);text-underline-offset:3px;overflow-wrap:anywhere}a:focus-visible{outline:3px solid #bf6900;outline-offset:3px}aside{position:fixed;inset:0 auto 0 0;width:265px;overflow:auto;padding:26px 18px;border-right:1px solid var(--line);background:#edf3f2}aside strong{font:700 23px/1.2 Georgia,serif}nav a{display:block;padding:5px 0;font-size:12px}main{margin-left:265px;padding:34px 36px;max-width:1690px;min-width:0}header{border-bottom:4px solid var(--accent);padding-bottom:22px}h1{font:700 36px/1.15 Georgia,serif;margin:8px 0 18px}h2{font:700 27px/1.3 Georgia,serif;margin:32px 0 16px}h3{font-size:18px;overflow-wrap:anywhere;margin-top:24px}h4{font-size:15px}p{max-width:1050px}.eyebrow{font-size:12px;color:var(--accent);text-transform:uppercase;font-weight:700}.meta{font-size:13px;color:var(--muted);overflow-wrap:anywhere}.notice{border-left:4px solid #b57111;background:#fff4dc;padding:12px 18px}.table-wrap{overflow:auto}table{width:100%;border-collapse:collapse;font-size:13px}th,td{text-align:left;vertical-align:top;padding:9px 12px;border-bottom:1px solid var(--line)}th{background:#e8efed}td:first-child{overflow-wrap:anywhere}code{font:13px Consolas,monospace}section{border-bottom:1px solid var(--line);padding-bottom:24px}.listing{border:1px solid #b8c8c5;margin:18px 0;background:#fff}.hunk-title{margin:0;padding:8px 12px;background:#e9efee;color:#3c5550;font:12px Consolas,monospace;overflow-wrap:anywhere}.code-scroll{overflow:auto}.code-grid{display:table;min-width:100%;width:max-content;font:12px/1.65 Consolas,monospace;tab-size:4}.code-row{display:table-row;scroll-margin-top:24px}.code-row>span,.code-row>code{display:table-cell;padding:0 7px}.code-row .ln{width:52px;text-align:right;color:#60706e;border-right:1px solid #d9e0de;user-select:none}.code-row .mark{width:68px;min-width:68px;text-align:center}.code-row .sign{width:20px;color:#52645d}.code-row code{white-space:pre;min-width:420px}.code-row.add{background:#e0f3e5}.code-row.del{background:#fde6e9}.code-row:target{outline:2px solid #b87505;outline-offset:-2px}.marker{font-weight:700;background:#fff5c4;color:#664700;padding:1px 4px;text-decoration:none}.tok-comment{color:#536b61;font-style:italic}.tok-string{color:#925000}.tok-keyword{color:#124db0;font-weight:600}.tok-number{color:#8d247a}.tok-function{color:#00605b}.tok-constant{color:#934023}.tok-variable{color:#704887}.comment{padding:10px 0 14px;border-top:1px solid var(--line);scroll-margin-top:20px}.comment:target{background:#fff4d9}.comment p{margin:7px 0}.back{font-size:13px}.state{font-weight:600;color:var(--muted)}footer{font-size:12px;color:var(--muted);padding-top:24px}.file-summary{font-size:13px;color:var(--muted)}.columns{font-size:12px;color:var(--muted);padding:6px 12px;border-bottom:1px solid var(--line)}
@media(max-width:850px){aside{position:static;width:auto;max-height:230px;border-right:0;border-bottom:1px solid var(--line)}main{margin:0;padding:22px 16px}h1{font-size:29px}.code-row code{min-width:280px}.code-grid{font-size:11px}th,td{padding:7px}}
@media print{aside{display:none}main{margin:0;padding:0;max-width:none}body{font-size:10pt;background:white}h1{font-size:26pt}h2{break-after:avoid}h3,h4{break-after:avoid}.comment{break-inside:avoid}.code-scroll{overflow:visible}.code-grid{display:block;width:100%;font-size:7.5pt}.code-row{display:flex}.code-row>span{display:block;flex:0 0 auto}.code-row .ln{width:35px}.code-row .mark{width:45px;min-width:45px}.code-row code{display:block;white-space:pre-wrap;overflow-wrap:anywhere;min-width:0;flex:1}.listing{break-inside:auto}a{color:inherit}thead{display:table-header-group}}
</style></head><body><aside><strong>RasPiAutomation</strong><p class="meta">Source Code Difference Report<br>T1010 zu T1011 / 25.10.2026</p><nav aria-label="Inhaltsverzeichnis"><a href="#scope">1 Geltungsbereich</a><a href="#overview">2 Aenderungsuebersicht</a><a href="#acceptance">3 Abnahmebezug</a><a href="#components">4 Komponenten</a>
'@)
foreach ($file in $report.SelectNodes('file')) { [void]$html.Append('<a href="#' + $file.id + '">' + $file.id.Substring(1) + ' ' + (Escape-Html $file.path) + '</a>') }
[void]$html.Append('</nav></aside><main><header id="top"><p class="eyebrow">Technischer Aenderungsbericht / Git-Tag-Vergleich</p><h1>RasPiAutomation<br>Source Code Difference Report</h1><p><strong>Alt:</strong> ' + (Escape-Html $OldTag) + '<br><strong>Neu:</strong> ' + (Escape-Html $NewTag) + '</p><p class="meta">Stand 25.10.2026 | Status: dokumentierter Diff mit Abnahmebezug, keine neue Freigabe<br>Alt-Commit: ' + $report.oldCommit + '<br>Neu-Commit: ' + $report.newCommit + '</p></header>')
[void]$html.Append('<section id="scope"><h2>1 Geltungsbereich und Zweck</h2><p>Dieser Bericht zeigt saemtliche geaenderten Dateien zwischen den genannten Git-Tags, unabhaengig vom aktuellen Arbeitsverzeichnis. Quelltext und Konfiguration erscheinen als Unified Diff mit fuenf Kontextzeilen, Alt-/Neu-Zeilennummern, Syntaxhervorhebung und deutschen Begruendungen. Neue Textdateien sind vollstaendig enthalten; geloeschte Binaerdateien werden im Inventar ausgewiesen.</p><p>Die Gliederung folgt dem bereitgestellten PDF-Muster: Geltungsbereich, Aenderungsuebersicht sowie je Komponente Source-Code-Listing und Kommentare. Fremde Projektdaten, Logos und Freigaben werden nicht uebernommen. Jeder Marker Mxxx sitzt an der ersten geaenderten Codezeile seines Blocks und verlinkt auf den zugehoerigen Kommentar; dessen Ruecksprung fuehrt exakt zu dieser Zeile. Minus ist Altbestand, Plus ist Neubestand; unmarkierte Zeilen sind Kontext.</p><p class="notice">Der Tagname PIPES bedeutet nicht, dass dieser RasPi-Diff eine Windows-Named-Pipe- oder Task-Lifetime-Implementierung enthaelt. Der Abnahmebericht bewertet einen separaten Windows-Fix. Die hier erfassten RasPi-Aenderungen ergaenzen insbesondere Audio-Protokollpruefung und Mixer-Verifikation.</p></section>')
[void]$html.Append('<section id="overview"><h2>2 Aenderungsuebersicht</h2><p><strong>' + $report.files + ' Dateien</strong> | ' + $report.hunks + ' kommentierte Diff-Bloecke | +' + $report.added + ' / -' + $report.deleted + ' Textzeilen</p><div class="table-wrap"><table><thead><tr><th>Bereich</th><th>Warum die Aenderung aufgenommen wurde</th></tr></thead><tbody><tr><td>Audio-Profilabschluss</td><td>Scheinbaren Erfolg verhindern: vollstaendig senden, kanalrichtig zuruecklesen und alle vier Sollwerte bestaetigen.</td></tr><tr><td>Fehler und Nachverfolgung</td><td>Portzugriff schuetzen, Fehler erhalten und Windows-Auftrag mit RasPi-Logs korrelieren; Logs begrenzen.</td></tr><tr><td>Hardwarefreie C-Tests</td><td>Parser-, Vergleichs-, Retry- und Write-Sonderfaelle ohne reale UART-Hardware pruefbar machen.</td></tr><tr><td>Build und Betrieb</td><td>Debug/Release und Deployment trennen; PTZ-Modus und IP beim Build bereitstellen.</td></tr><tr><td>Legacy und Bereinigung</td><td>Servo-/Positionspfad anpassen und historische Distributionsdateien entfernen; nicht vom Audio-PASS pauschal gedeckt.</td></tr></tbody></table></div><h3>Dateiinventar</h3><div class="table-wrap"><table><thead><tr><th>Datei</th><th>Status</th><th>+ / -</th><th>Abdeckung</th></tr></thead><tbody>')
foreach ($file in $report.SelectNodes('file')) {
    $coverage = if ($file.binary -eq 'true') { 'Binaerdatei: kein Textdiff' } else { [string]$file.SelectNodes('hunk').Count + ' kommentierte Bloecke' }
    [void]$html.Append('<tr><td><a href="#' + $file.id + '">' + (Escape-Html $file.path) + '</a></td><td>' + $file.status + '</td><td>' + $file.added + ' / ' + $file.deleted + '</td><td>' + $coverage + '</td></tr>')
}
[void]$html.Append('</tbody></table></div></section><section id="acceptance"><h2>3 Bezug zum Abnahmebericht</h2><p><strong>Quelle:</strong> JokiAutomation_Audio_Pipeline_Lifetime_Fix_Abnahmebericht.pdf, Pruefdatum 22.10.2026, Gesamtstatus PASS.</p><div class="table-wrap"><table><thead><tr><th>Nachweis im Abnahmebericht</th><th>Zuordnung und Grenze dieses Reports</th></tr></thead><tbody><tr><td>S. 1-2: Windows setzte queued mit completed gleich.</td><td>Die CLI konnte enden, bevor der SSH-Auftrag ausgefuehrt war. Der finale Windows-Fix wartet am Command-Ende auf den konkreten Audio-Task; Audio und PTZ bleiben parallel. Dieser C#-Code ist nicht Bestandteil des RasPi-Tag-Diffs.</td></tr><tr><td>S. 4: 65/65 Tests, Debug/Release x64 ohne Fehler.</td><td>Extern berichtete Windows-Ergebnisse, hier nicht erneut ausgefuehrt. Dort steht ausdruecklich: Raspberry-Code unveraendert. Die hier neu enthaltenen C-Tests sind ein anderer Testumfang.</td></tr><tr><td>S. 5: 17:12:43 COMMAND command=30 index=114, Profil 2.</td><td>Passt zur neuen Kommando- und Profilprotokollierung. Der erste dokumentierte Predigt-Aufruf erreicht den Raspberry.</td></tr><tr><td>S. 5: 17:13:22 requested=0,50,0,31; actual=0,50,0,31; match=1; result=0.</td><td>Belegt im dort untersuchten Lauf die reale Mixer-Ruecklesung und einen bestaetigten Profilabschluss. Weiterer Erfolg um 17:18:26. Kein Beweis fuer alle Profile, Fehlerpfade oder Legacy-/Deployment-Aenderungen.</td></tr><tr><td>S. 6: zwei erfolgreiche EventTimer-Handoffs.</td><td>Systemweiter Nebenbefund, keine EventTimer-Implementierung in diesen RasPi-Dateien.</td></tr></tbody></table></div><p class="notice">Bewertungsgrenze: PASS wird als Aussage des vorgelegten Abnahmeberichts wiedergegeben. Dessen Rohlogs wurden hier nicht erneut geprueft; eine Bindung an die beiden Git-Commit-Hashes ist dort nicht angegeben. Dieser Bericht erteilt keine neue Gesamtfreigabe.</p><h3>Offene Pruefpunkte ausserhalb des Audio-Nachweises</h3><ul><li>PTZ/IP-Konfiguration: Wirkung der mit ^ verankerten CMake-Regexe bei mehrzeiligem network.cfg pruefen.</li><li>Servo: Pinmodus sowie Verhalten bei Pulsbreite 0 am Ziel pruefen; der neue Nullpfad ruft gpioServo(0) nicht auf.</li><li>Deployment: Loeschen vor SCP ist kein atomarer Austausch; Rueckfall bei Transferfehler ist nicht enthalten.</li><li>Release: Die vorhandenen expliziten Compileroptionen -O0/-g3 bleiben trotz neuem Release-Buildtyp bestehen; daraus folgt keine zugesicherte Optimierung.</li><li>Die Loeschung historischer Binaer- und Konfigurationsdateien hat keinen Einzel-Abnahmenachweis in den Audio-Unterlagen.</li></ul></section><section id="components"><h2>4 Komponenten und Codekommentare</h2>')
foreach ($file in $report.SelectNodes('file')) {
    [void]$html.Append('<section id="' + $file.id + '"><h3>4.' + [int]$file.id.Substring(1) + ' ' + (Escape-Html $file.path) + '</h3><p class="file-summary">' + $file.status + ' | +' + $file.added + ' / -' + $file.deleted + ' | <a href="#overview">Zum Inventar</a></p>')
    if ($file.binary -eq 'true') {
        [void]$html.Append('<p class="state">Binaerinhalt: kein Source-Code-Listing.</p><p>' + (Escape-Html $file.reason) + '</p></section>')
        continue
    }
    [void]$html.Append('<h4>Source-Code-Listing</h4><p class="meta">Unified Diff: links Alt-Zeile, daneben Neu-Zeile. Die hervorgehobenen Codeinhalte stammen direkt aus den Tag-Blobs.</p>')
    foreach ($hunk in $file.SelectNodes('hunk')) {
        [void]$html.Append('<div class="listing"><p class="hunk-title">' + (Escape-Html $hunk.header) + '</p><div class="columns">Marker | Alt | Neu | Diff | Code</div><div class="code-scroll" tabindex="0" aria-label="Diff-Block ' + $hunk.marker + '"><div class="code-grid">')
        $marked = $false
        $rowIndex = 0
        foreach ($row in $hunk.SelectNodes('line')) {
            $rowIndex++
            $isMarker = -not $marked -and $row.kind -ne 'context'
            $id = "$($hunk.id)-r$rowIndex"
            $markerText = ''
            if ($isMarker) { $id = "$($hunk.id)-code"; $marked = $true; $markerText = '<a class="marker" href="#' + $hunk.id + '-comment" aria-label="Kommentar zu ' + $hunk.marker + '">' + $hunk.marker + '</a>' }
            $side = if ($row.kind -eq 'del') { 'old' } else { 'new' }
            $number = if ($side -eq 'old') { [int]$row.old } else { [int]$row.new }
            $codeHtml = $highlighted["$($file.id)-$side"][$number - 1]
            $sign = if ($row.kind -eq 'add') { '+' } elseif ($row.kind -eq 'del') { '-' } else { ' ' }
            [void]$html.Append('<div class="code-row ' + $row.kind + '" id="' + $id + '"><span class="mark">' + $markerText + '</span><span class="ln">' + $row.old + '</span><span class="ln">' + $row.new + '</span><span class="sign">' + $sign + '</span><code>' + $codeHtml + '</code></div>')
        }
        [void]$html.Append('</div></div></div>')
    }
    [void]$html.Append('<h4>Kommentare und Begruendung</h4>')
    foreach ($hunk in $file.SelectNodes('hunk')) {
        [void]$html.Append('<article class="comment" id="' + $hunk.id + '-comment"><strong>' + $hunk.marker + '</strong> <span class="meta">' + (Escape-Html $hunk.header) + '</span><p>' + (Escape-Html $hunk.reason) + '</p><a class="back" href="#' + $hunk.id + '-code">Zurueck zur Codezeile ' + $hunk.marker + '</a></article>')
    }
    [void]$html.Append('</section>')
}
[void]$html.Append('</section><footer><p>Grundlage: exakte Git-Tag-Blobs und Diff; Audio-Arbeitsauftrag im neuen Tag; vorgelegter Abnahmebericht. Fuehrende Datenbasis: <a href="' + $stem + '.xml">XML-Report</a>. Das HTML ist daraus abgeleitet und enthaelt alle Codeinhalte lokal, ohne CDN oder externe Skripte.</p><p>PDF-Strukturvorlage: SourceCodeDifferenceReport_V230_5-V230_6_review.pdf; nur die Gliederungs- und Markerkonvention wird uebernommen. Keine fremden Projektinhalte oder Freigaben.</p><a href="#top">Zum Berichtskopf</a></footer></main></body></html>')
$acousticLimit = 'Abnahmebericht S. 7: Die protokollierte Mixer-Ruecklesung weist den eingestellten Sollzustand nach. Eine akustische Bewertung des gesamten Signalwegs ist davon getrennt.'
$acceptanceNode = $document.SelectSingleNode('/sourceDiffReport/acceptance')
$acceptanceNode.InnerText += ' ' + $acousticLimit
$writer = [System.Xml.XmlWriter]::Create($xmlPath, $settings)
try { $document.Save($writer) } finally { $writer.Dispose() }
[void]$html.Replace('</style>', 'section,header,footer{min-width:0;max-width:100%}p,li,h1,h2,h3,h4{overflow-wrap:anywhere}.listing{max-width:100%;min-width:0}.table-wrap,.code-scroll{max-width:100%;min-width:0}</style>')
[void]$html.Replace('<h3>Offene Pruefpunkte', '<p class="notice">' + $acousticLimit + '</p><h3>Offene Pruefpunkte')
[void]$html.Replace('Dieser Bericht zeigt saemtliche geaenderten Dateien zwischen den genannten Git-Tags, unabhaengig vom aktuellen Arbeitsverzeichnis.', 'Dieser Bericht zeigt ausschliesslich geaenderte C-Quelltexte und Header (*.c, *.h) zwischen den genannten Git-Tags, unabhaengig vom aktuellen Arbeitsverzeichnis. Andere Dateitypen sind weder im Inventar noch in den Listings oder Codekommentaren enthalten.')
[void]$html.Replace('Quelltext und Konfiguration erscheinen als Unified Diff', 'C-Quelltexte und Header erscheinen als Unified Diff')
[void]$html.Replace('Neue Textdateien sind vollstaendig enthalten; geloeschte Binaerdateien werden im Inventar ausgewiesen.', 'Neue C-Quelltexte und Header sind vollstaendig enthalten.')
[void]$html.Replace('<tr><td>Build und Betrieb</td><td>Debug/Release und Deployment trennen; PTZ-Modus und IP beim Build bereitstellen.</td></tr>', '<tr><td>Konfiguration und Diagnose</td><td>Die konfigurierte IP ueber die Systemschnittstelle ausgeben und im PTZ-Modus den historischen lokalen Bewegungspfad ueberspringen.</td></tr>')
[void]$html.Replace('<tr><td>Legacy und Bereinigung</td><td>Servo-/Positionspfad anpassen und historische Distributionsdateien entfernen; nicht vom Audio-PASS pauschal gedeckt.</td></tr>', '<tr><td>Legacy-Hardware</td><td>Servo-/Positionspfad anpassen; nicht vom Audio-PASS pauschal gedeckt.</td></tr>')
[void]$html.Replace('<li>Deployment: Loeschen vor SCP ist kein atomarer Austausch; Rueckfall bei Transferfehler ist nicht enthalten.</li>', '')
[void]$html.Replace('<li>Release: Die vorhandenen expliziten Compileroptionen -O0/-g3 bleiben trotz neuem Release-Buildtyp bestehen; daraus folgt keine zugesicherte Optimierung.</li>', '')
[void]$html.Replace('<li>Die Loeschung historischer Binaer- und Konfigurationsdateien hat keinen Einzel-Abnahmenachweis in den Audio-Unterlagen.</li>', '')
$report.SetAttribute('scope',$scopeLabel)
$report.SetAttribute('project',$projectName)
$report.SetAttribute('repository',$root)
if ($GeneralReport) {
    [void]$report.RemoveChild($document.SelectSingleNode('/sourceDiffReport/acceptance'))
    $referenceRoot = $document.CreateElement('references')
    $referenceRoot.SetAttribute('description',$Description)
    foreach ($reference in $references) {
        $referenceNode = $document.CreateElement('reference')
        $referenceNode.SetAttribute('id',$reference.Id)
        $referenceNode.SetAttribute('path',$reference.Path)
        $referenceNode.SetAttribute('sha256',$reference.Hash)
        $referenceNode.InnerText = [regex]::Replace($reference.Text, '[\x00-\x08\x0B\x0C\x0E-\x1F]', '')
        [void]$referenceRoot.AppendChild($referenceNode)
    }
    [void]$report.AppendChild($referenceRoot)
    $referenceHtml = '<section id="acceptance"><h2>3 Referenzen und Aenderungsgrund</h2><p>' + (Escape-Html $Description) + '</p><p>Commit-Nachrichten und automatisch gefundene Referenzstellen sind Kontext fuer die fachliche Beschreibung, keine automatisch bestaetigte Ursache oder Abnahme. PDF-Zeilennummern beziehen sich auf den extrahierten Text, nicht auf Quelltextzeilen. Es wurden keine Tests ausgefuehrt und keine Freigaben abgeleitet.</p>'
    if (-not $references.Count) { $referenceHtml += '<p>Keine Referenzdateien angegeben.</p>' }
    foreach ($reference in $references) {
        $referenceHtml += '<h3>' + $reference.Id + ' ' + (Escape-Html ([IO.Path]::GetFileName($reference.Path))) + '</h3><p class="meta">' + (Escape-Html $reference.Path) + '<br>SHA256: ' + $reference.Hash + '</p><details><summary>Referenztext</summary><pre style="white-space:pre-wrap;overflow-wrap:anywhere">' + (Escape-Html $reference.Text) + '</pre></details>'
    }
    $referenceHtml += '</section>'
    $rendered = $html.ToString()
    $rendered = [regex]::Replace($rendered, '<section id="acceptance">[\s\S]*?(?=<section id="components">)', [System.Text.RegularExpressions.MatchEvaluator]{ param($match) $referenceHtml })
    $rendered = [regex]::Replace($rendered, '<div class="table-wrap"><table><thead><tr><th>Bereich</th>[\s\S]*?<h3>Dateiinventar</h3>', '<p>Umfang: ausschliesslich geaenderte *.c und *.h. Begruendungen enthalten den angegebenen Kontext, dateibezogene Commit-Nachrichten und gegebenenfalls Suchtreffer aus den Referenzen.</p><h3>Dateiinventar</h3>')
    $rendered = [regex]::Replace($rendered, '<p class="notice">Der Tagname PIPES[\s\S]*?</p>', '')
    $rendered = $rendered.Replace('3 Bezug zum Abnahmebericht','3 Referenzen und Aenderungsgrund').Replace('dokumentierter Diff mit Abnahmebezug, keine neue Freigabe','automatisch erzeugter Diff, fachliche Pruefung erforderlich')
    $rendered = $rendered.Replace('RasPiAutomation | Source-Diff T1010 zu T1011', 'RasPiAutomation | Source-Diff ' + (Escape-Html $OldTag) + ' zu ' + (Escape-Html $NewTag)).Replace('T1010 zu T1011 / 25.10.2026', (Escape-Html $OldTag) + ' zu ' + (Escape-Html $NewTag) + ' / ' + $reportDate).Replace('Stand 25.10.2026', 'Stand ' + $reportDate)
    $rendered = $rendered.Replace('Audio-Arbeitsauftrag im neuen Tag; vorgelegter Abnahmebericht','dateibezogene Commit-Nachrichten; optional angegebene Referenzdateien').Replace('Git-Tags','Git-Staenden').Replace('Tag-Blobs','Git-Blobs')
    $escapedProject = Escape-Html $projectName
    $rendered = $rendered.Replace('<title>RasPiAutomation |', '<title>' + $escapedProject + ' |').Replace('<aside><strong>RasPiAutomation</strong>', '<aside><strong>' + $escapedProject + '</strong>').Replace('<h1>RasPiAutomation<br>', '<h1>' + $escapedProject + '<br>')
    $rendered = $rendered.Replace('C-Quelltexte und Header (*.c, *.h)', 'Quelltexte (' + $scopeLabel + ')').Replace('C-Quelltexte und Header', 'Quelltexte (' + $scopeLabel + ')').Replace('*.c und *.h', $scopeLabel)
    [void]$html.Clear()
    [void]$html.Append($rendered)
}
$writer = [System.Xml.XmlWriter]::Create($xmlPath, $settings)
try { $document.Save($writer) } finally { $writer.Dispose() }
$htmlPath = Join-Path $outputDir "$stem.html"
[System.IO.File]::WriteAllText($htmlPath, $html.ToString(), $utf8)
$ids = @([regex]::Matches($html.ToString(), '\bid="([^"]+)"') | ForEach-Object { $_.Groups[1].Value })
$links = @([regex]::Matches($html.ToString(), 'href="#([^"]+)"') | ForEach-Object { $_.Groups[1].Value })
if (@($ids | Group-Object | Where-Object Count -gt 1).Count -ne 0) { throw 'Duplicate HTML IDs' }
foreach ($link in $links) { if ($link -notin $ids) { throw "Broken internal link: $link" } }
if ($report.SelectNodes('file/hunk').Count -ne $hunkTotal) { throw 'Hunk coverage mismatch' }
foreach ($file in $report.SelectNodes('file')) { if ([IO.Path]::GetExtension($file.path) -notin $extensions) { throw "Non-source file in report: $($file.path)" } }
Write-Output "HTML: $htmlPath"
Write-Output "XML:  $xmlPath"
Write-Output "Validated: $fileIndex files; $hunkTotal markers with return links; +$addedTotal / -$deletedTotal lines; $($links.Count) internal links."