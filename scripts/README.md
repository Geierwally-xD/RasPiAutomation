# Scripts

This directory contains helper scripts for building (Windows/PowerShell) and portable reconfiguration when changing computers/drives.

## Source-Code-Diff als VS-Code-Task

Es gibt zwei Tasks im aktuellen RasPiAutomation-Workspace:

- **CodeDiff: Source-Report (C/H)** verwendet [codediff.raspi.json](codediff.raspi.json).
- **CodeDiff: JoKiAutomation Source-Report (C#)** verwendet [codediff.csharp.json](codediff.csharp.json) und liest das benachbarte Repository `../../JokiAutomation` relativ zum Profil.

Die JSON-Profile enthalten `projectName`, `repositoryPath`, `outputDirectory`
und `extensions`. Pfade im Profil sind relativ zur JSON-Datei. Der C#-Task
filtert ausschliesslich `.cs`; der C/H-Task weiterhin `.c` und `.h`.
Der C#-Report wird getrennt unter `Docu/CodeDiff/CSharp/` abgelegt und erhaelt
`_generated` im Dateinamen. Der RasPi-Task aktualisiert den Report ohne diesen
Zusatz unter `Docu/CodeDiff/`.
Das fremde Repository wird nicht veraendert. Beide Tasks fragen dieselben vier
Felder ab; relative Referenzpfade beziehen sich auf das jeweils verglichene
Repository. Tags und Commit-IDs werden ebenfalls dort aufgeloest.

Beispiel fuer den C#-Report:

```powershell
.\scripts\generate_source_diff_report.ps1 -ConfigPath .\scripts\codediff.csharp.json -OldTag JOKI_T1010_PIGPIO -NewTag JOKI_T1011_PIPES -ReferencePaths '..\Testreports\JokiAutomation_Audio_Pipeline_Lifetime_Fix_Abnahmebericht.pdf'
```

Unter **Terminal > Run Task** den Task **CodeDiff: Source-Report (C/H)** starten.
Die Eingaben erscheinen in dieser Reihenfolge:

1. Neuer Git-Stand: Tag oder Commit-ID.
2. Alter Git-Stand: Tag oder Commit-ID.
3. Optional: Referenzdateien als absolute oder workspace-relative Pfade, mit Semikolon getrennt. Unterstuetzt werden PDF, TXT, MD und LOG. Leer lassen, wenn keine Referenzen vorliegen.
4. Optional: kurze fachliche Begruendung, warum die Aenderung eingebaut wurde.

Der Task erzeugt ausschliesslich fuer geaenderte `.c`- und `.h`-Dateien einen
HTML-Report und eine fuehrende XML-Datenbasis im konfigurierten Ausgabeordner.
Ein erneuter Lauf mit denselben Eingaben fuer Alt/Neu ersetzt die Reportdateien
dieses Profils. Der RasPi-Report verwendet keinen Suffix; C# verwendet
`_generated`, um die generierte Ausgabe zu kennzeichnen. Ausgabepfade werden
im Task-Terminal ausgegeben.

Syntaxhervorhebung, deutsche Kontextkommentare und Marker mit Ruecksprung zur
Codezeile sind enthalten. Die eingegebene Begruendung gilt fuer den Vergleich;
dateibezogene Commit-Nachrichten und maximal drei Referenzstellen je Diff-Block
ergaenzen sie. Die Referenzsuche verwendet gleiche Codebezeichner. Treffer sind
keine automatische fachliche Bewertung oder Abnahme und muessen geprueft werden.
Ohne passende Referenz wird dies im Kommentar kenntlich gemacht.

Referenzen werden nur gelesen, mit SHA256 dokumentiert und als extrahierter Text
in XML und HTML eingebettet. Daher nur Dokumente verwenden, deren Inhalt im
Report weitergegeben werden darf. PDF benoetigt `pdftotext.exe` im PATH oder unter
`C:\Program Files\Git\mingw64\bin`; gescannte PDFs ohne Text benoetigen vorher OCR.
Es werden weder Build noch Tests oder fremde Berichtsgeneratoren gestartet.

Direkter Aufruf:

```powershell
.\scripts\generate_source_diff_report.ps1 -OldTag JOKI_T1010_PIGPIO -NewTag JOKI_T1011_PIPES -GeneralReport -ReferencePaths 'C:\Reports\Abnahme.pdf;README.md' -Description 'Fehler beim ersten Profilwechsel beheben.'
```

## build4newDrive.ps1

Purpose:
- "Computer migration / new drive": Clean CMake cache and reconfigure from scratch (roughly equivalent to "CMake: Delete Cache and Reconfigure").
- Optional: Completely delete build folder (if possible).

Parameters (excerpt):
- `-Fresh` (Default: `1`): attempts to delete `build-rpi/`; if locked, automatically falls back to `cmake --fresh` during configure.
- `-DeleteCache` (Default: `1`): best-effort deletion of `build-rpi\CMakeCache.txt` and `build-rpi\CMakeFiles\`.
- `-ToolchainsRoot` (optional): Path to `ToolChains` root (otherwise auto-find / settings).

Examples:
```powershell
# Delete cache + reconfigure from scratch (without deleting build-rpi)
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build4newDrive.ps1 -Fresh 0 -DeleteCache 1

# Maximum fresh (attempt to delete build-rpi + fallback)
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build4newDrive.ps1 -Fresh 1 -DeleteCache 1
```


## clean_and_build_rpi.ps1
**Purpose:**
- Performs a clean CMake configuration (via the robust wrapper) followed by `clean` + build.

**Example:**
```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\clean_and_build_rpi.ps1
```

## cmake_configure_clean.ps1
**Purpose:**
- Deletes the build directory (`build-rpi/`) if the CMake cache points to a different source directory (e.g., after computer migration or path change).
- Then starts CMake configuration via the robust wrapper script (`.vscode/cmake_configure_rpi.ps1`).

**Parameters:**
- `-ToolchainsRoot` (optional): Path to toolchain root.

**Example:**
```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\cmake_configure_clean.ps1
```



## Sysroot-Sync (Raspberry Pi → Cross-Toolchain)

### sync_sysroot_from_pi.ps1
**Purpose:**
- Copies the sysroot as a tarball from the Pi and unpacks it locally (older variant, may cause problems with long paths/symlinks on Windows).

**Process:**
1. Creates a tarball archive of the sysroot on the Pi.
2. Copies the archive via SCP to Windows.
3. Unpacks the archive locally into the toolchain sysroot directory.

**Note:** This method is less robust on Windows (symlinks, long paths).

### sync_sysroot_rsync_from_pi.ps1
**Purpose:**
- **Recommended variant:** Creates the sysroot on the Pi, packs it as a `.tar.gz` archive, and copies it to Windows. The archive must then be unpacked with WSL to avoid symlink and permission problems.

**Process:**
1. Creates the sysroot directory on the Pi (via rsync of all relevant system directories)
2. Packs the sysroot on the Pi as a `.tar.gz` archive
3. Copies the archive via SCP to the `gccArmCrossCompiler` directory on Windows
4. Outputs WSL commands for unpacking (copy-paste ready with automatic path conversion)

**Important:**
- The archive **must be unpacked with WSL** (not with Windows pack programs like 7-Zip or WinRAR)
- The script outputs the exact WSL commands after download
- Reason: Only WSL preserves symlinks and file permissions correctly

**Note:** The scripts are located in the `.vscode/` directory.

## Technical Background

The actual portable configure logic (Toolchain/Ninja/Compiler auto-find + fresh reconfigure) is contained in the wrapper:
- `.vscode\cmake_configure_rpi.ps1`

If `build-rpi/` is "locked" by a process, the scripts use `cmake --fresh` so no manual deletion is necessary.

## Portability: Any Drive

The build system is configured to work on **any drive** (D:, E:, F:, etc.):

- The toolchain file (`cmake/toolchains/aarch64-linux-gnu.cmake`) uses **relative paths** for automatic toolchain detection
- No hardcoded drive letters in CMakeLists.txt or PowerShell scripts
- The complete `MyGitRepos` directory can be copied to any drive

**Prerequisite:** The directory structure must be maintained:
```
<any drive>/
  MyGitRepos/
    ToolChains/SysGCC/GCC64RasPi4/gccArmCrossCompiler/
    JoKiAutomation/RasPiAutomation/
```

If the toolchain is located elsewhere, `TOOLCHAIN_BIN_DIR` can be set manually.
