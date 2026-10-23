# GitHub-Copilot-Arbeitsauftrag

## Raspberry Pi AudioControl: Profilwechsel verifizieren und sporadisch fehlende Umschaltung beheben

## 1. Ziel

Überarbeite ausschließlich den Raspberry-Pi-Audiotreiber in:

- `AudioControl.c`
- bei Bedarf `AudioControl.h`
- bei Bedarf ergänzende Tests für `AudioControl`

Ziel ist, dass ein Audioprofil erst dann mit Erfolg abgeschlossen wird, wenn der Audiomixer nach dem Schreiben tatsächlich die angeforderten Kanalwerte zurückmeldet.

Der Fix muss das beobachtete Fehlerbild beseitigen:

```text
Erster Aufruf:
PositionControl | Kanzel | GP
Kamera fährt korrekt
Predigt-Audioprofil wird angefordert
Audioprofil ist akustisch teilweise nicht umgeschaltet
AudioControl meldet trotzdem result=0

Zweiter identischer Aufruf:
Audioprofil wird hörbar korrekt gesetzt
```

Kein großer Architekturumbau. Keine Änderungen an Kamera, ATEM, Named Pipe, Mutex, EventTimer oder PowerPoint-Anbindung.

---

## 2. Nachgewiesener Iststand

### Profilzuordnung

Für `PositionControl` wird das zweite Zeichen des Zweibuchstaben-Codes als Audioprofil verwendet:

```text
D = Diashow     -> Profil 0
G = Gottesdienst -> Profil 1
P = Predigt      -> Profil 2
T = Text         -> Profil 3
B = Band         -> Profil 4
```

Damit gilt beispielsweise:

```text
GG -> Gottesdienst, Profil 1, command index 113
GP -> Predigt,      Profil 2, command index 114
KP -> Predigt,      Profil 2, command index 114
```

Wichtig: Ein Aufruf `PositionControl | Kanzel | GG` ist kein Predigt-Profil. Für Kanzelkamera als Bewegungsbild mit Predigt-Audio ist der passende Code nach aktuellem Mapping `KP`; für GoPro als Bewegungsbild mit Predigt-Audio `GP`.

### Raspberry-Pi-Logik

`AC_Profile()` führt derzeit aus:

```c
AC_Request(index);
AC_GetVolume();

volumeSteps[0] = volumeState[0];
volumeSteps[1] = volumeState[1];
volumeSteps[2] = volumeState[2];
volumeSteps[3] = volumeState[3];

AC_Control();
```

Danach wird unmittelbar Erfolg protokolliert und zurückgegeben:

```c
AC_Log("AC_Profile end: pid=%d profile=%u result=%u", getpid(), index, result);
return result;
```

Es gibt aktuell keine abschließende Rücklesung und keinen Soll-Ist-Vergleich nach `AC_Control()`.

### Relevante Schwachstellen

1. `AC_write()` betrachtet jeden Rückgabewert `>= 0` als Erfolg. Ein partieller Write wird nicht erkannt.
2. `AC_read()` prüft nur, ob mindestens 9 Byte gelesen wurden.
3. `AC_GetVolume()` prüft nicht, ob eine Antwort wirklich zum angefragten Kanal gehört.
4. Der Wert wird mit `atoi(&volString[6])` übernommen, ohne das Antwortformat vollständig zu validieren.
5. `AC_Control()` sendet nur dann Werte, wenn `volumeSteps[i] != volumeRequest[i]` ist.
6. Wenn eine veraltete, verspätete oder kanalverkehrte Antwort bereits den Sollwert meldet, kann das Schreiben übersprungen werden.
7. `AC_Profile result=0` bedeutet derzeit nur, dass die vorhandenen Aufrufe keinen erkannten Softwarefehler geliefert haben. Der physische Mixer-Endzustand ist nicht bestätigt.

---

## 3. Scope und Grenzen

### Ändern

- UART-Schreibprüfung in `AC_write()`
- UART-Antwortvalidierung bei Lautstärkeabfragen
- abschließende Verifikation in `AC_Profile()`
- maximal einen kontrollierten Profil-Retry bei gültiger Kommunikation, aber Soll-Ist-Abweichung
- eindeutige Diagnose-Logs
- testbare Hilfsfunktionen für Parsing und Vergleich

### Nicht ändern

- Profilwerte in `AC_Dat.bin`
- Profilnummern und Mapping
- Fade-Geschwindigkeit von 30 ms pro Schritt
- JokiAutomation-Pipeline-Architektur
- `PositionControl`
- Kamera-/ATEM-Steuerung
- SSH-Aufruf in `RasPi.Async.cs`
- Mutex-/Pipe-/Lifecycle-Code
- keine pauschale doppelte Profilanforderung in JokiAutomation
- kein unendlicher Retry
- kein Reset des Audiomixers als Standardlösung

---

## 4. Aufgabe A: `AC_write()` auf vollständige Übertragung prüfen

### Problem

Aktuell:

```c
streamlength = write(uart0_filestream, buffer, length);
if (streamlength < 0)
{
    result = AC_TX_FAILED;
}
```

Ein partieller Write bleibt unbemerkt.

### Anforderung

Implementiere eine robuste Schreibfunktion, die entweder:

- alle `length` Bytes überträgt, oder
- mit `AC_TX_FAILED` endet.

Dabei berücksichtigen:

- `EINTR`
- partiellen Write
- Rückgabewert `0`
- Gesamtzahl übertragener Bytes

Bevorzugtes Verhalten:

```c
static unsigned char AC_writeAll(const char *buffer, size_t length);
```

Die Funktion soll solange weiterschreiben, bis alle Bytes übertragen wurden oder ein echter Fehler vorliegt.

### Erforderliche Logs

Erfolg:

```text
AC_WRITE_OK requested=<n> written=<n> payload=<command>
```

Fehler:

```text
AC_WRITE_FAIL requested=<n> written=<n> errno=<n> payload=<command>
```

Keine Passwörter oder vertraulichen Daten loggen.

### Akzeptanz

`AC_write()` beziehungsweise die neue interne Implementierung darf nur `AC_SUCCESS` liefern, wenn exakt die angeforderte Bytezahl geschrieben wurde.

---

## 5. Aufgabe B: Lautstärkeantwort streng validieren

### Problem

Aktuell wird beispielsweise nach `VOL2` lediglich ausgeführt:

```c
volumeState[1] = atoi(&volString[6]);
```

Es wird nicht geprüft, ob die Antwort wirklich `VOL2` gehört.

### Anforderung

Führe eine zentrale Parsing-Funktion ein, zum Beispiel:

```c
static unsigned char AC_ParseVolumeResponse(
    const char *response,
    int responseLength,
    int expectedChannel,
    int *parsedVolume);
```

Die Funktion muss mindestens prüfen:

1. Antwortlänge entspricht dem erwarteten Format.
2. Startzeichen ist `(`.
3. Antwort beginnt mit dem erwarteten Kanal:
   - `(VOL1:`
   - `(VOL2:`
   - `(VOL3:`
   - `(VOL4:`
4. Die Lautstärke besteht aus genau zwei Dezimalziffern.
5. Endzeichen ist `)`.
6. Der Wert liegt zwischen `0` und `63`.
7. Es werden keine alten Inhalte aus einem vorherigen Buffer akzeptiert.

Ungültige Antworten dürfen nicht in `volumeState[]` übernommen werden.

### Neuer Fehlercode

Ergänze in `AudioControl.h` einen eindeutigen Fehlercode, falls das bestehende Fehlercodekonzept dies zulässt, beispielsweise:

```c
#define AC_INVALID_RESPONSE 39
#define AC_PROFILE_VERIFY_FAILED 40
```

Falls Fehlercodes derzeit bitweise mit `|=` kombiniert werden, prüfe vorher, ob diese dezimalen Werte dafür geeignet sind. Das bestehende Fehlerkonzept darf nicht stillschweigend beschädigt werden. Wenn die Codes keine echten Bitmasken sind, soll für neue Logik der erste konkrete Fehler erhalten bleiben oder ein klar dokumentiertes Fehleraggregationsverfahren verwendet werden.

### Erforderliche Logs

Gültige Antwort:

```text
AC_VOLUME_RESPONSE_OK channel=2 value=50 raw=(VOL2:50)
```

Ungültige Antwort:

```text
AC_VOLUME_RESPONSE_INVALID expectedChannel=2 bytes=<n> raw=<escaped-response> reason=<reason>
```

### Akzeptanz

Eine Antwort für Kanal 1 darf niemals als Antwort für Kanal 2, 3 oder 4 akzeptiert werden.

---

## 6. Aufgabe C: Kanalabfrage vereinheitlichen

Die vier nahezu identischen Blöcke in `AC_GetVolume()` sollen durch eine interne Hilfsfunktion ersetzt werden, zum Beispiel:

```c
static unsigned char AC_GetVolumeChannel(
    int channel,
    int *volume,
    int maxAttempts);
```

### Verhalten pro Versuch

1. Eingangsbuffer vollständig löschen.
2. Vor der Anfrage nur den UART-Eingangspuffer leeren:

```c
tcflush(uart0_filestream, TCIFLUSH);
```

3. Passenden Befehl senden.
4. Definierte kurze Wartezeit beibehalten.
5. Antwort vollständig lesen.
6. Antwort mit `AC_ParseVolumeResponse()` validieren.
7. Nur bei gültiger Antwort den Wert übernehmen.

### Retry

- maximal zwei Versuche pro Kanal
- nach ungültiger oder unvollständiger Antwort Eingangspuffer leeren
- keine endlose Schleife
- Fehler des letzten Versuchs zurückgeben

### Wichtig

Nicht bei jeder gültigen Antwort erneut flushen. Ein Flush darf nicht dazu führen, dass eine bereits eintreffende korrekte Antwort verworfenenen wird. Die Reihenfolge muss deterministisch sein.

---

## 7. Aufgabe D: Profil nach dem Schreiben verifizieren

### Ziel

`AC_Profile()` darf nur dann `AC_SUCCESS` melden, wenn nach dem Fade alle vier Mixerkanäle den Sollwert zurückmelden.

### Sollablauf

```text
AC_Profile start
  -> AC_Request
  -> AC_GetVolume vor Änderung
  -> AC_Control
  -> kurze Settling-Zeit
  -> AC_GetVolume nach Änderung
  -> Soll-Ist-Vergleich aller vier Kanäle
  -> Success oder kontrollierter Retry
AC_Profile end
```

### Hilfsfunktion

Führe eine Funktion ein, zum Beispiel:

```c
static int AC_IsRequestedProfileReached(void);
```

oder:

```c
static unsigned char AC_VerifyProfile(
    const int requested[4],
    const int actual[4]);
```

### Vergleich

Standardmäßig exakt vergleichen:

```c
actual[i] == requested[i]
```

Keine Toleranz einführen, solange das Protokoll ganzzahlige Werte `0..63` zurückliefert.

### Settling-Zeit

Nach `AC_Control()` eine kurze, klar benannte Konstante verwenden, zum Beispiel:

```c
#define AC_PROFILE_SETTLE_US 100000
```

Keinen verstreuten Magic-Wert einbauen.

### Verifikationslogs

```text
AC_PROFILE_VERIFY profile=2 requested=0,50,0,31 actual=0,50,0,31 match=1 attempt=1
```

Bei Abweichung:

```text
AC_PROFILE_VERIFY profile=2 requested=0,50,0,31 actual=0,31,0,56 match=0 attempt=1
```

---

## 8. Aufgabe E: Ein kontrollierter profilinterner Retry

### Wann wiederholen?

Nur wenn:

- `AC_GetVolume()` nach dem Schreiben formal gültige Antworten für alle vier Kanäle erhalten hat,
- aber mindestens ein Istwert vom Sollwert abweicht.

### Wann nicht wiederholen?

Nicht automatisch wiederholen bei:

- COM-Port konnte nicht geöffnet werden
- vollständiger UART-Ausfall
- ungültigen Antworten nach ausgeschöpftem Kanal-Retry
- Cancellation oder Prozessabbruch
- dauerhaftem partiellen Write

### Maximalzahl

Ein initialer Versuch plus genau ein Profil-Retry:

```c
#define AC_PROFILE_MAX_ATTEMPTS 2
```

### Retry-Ablauf

Beim zweiten Profilversuch nicht blind vom alten `volumeSteps[]`-Zustand ausgehen.

Stattdessen:

1. aktuellen Mixerzustand erneut gültig lesen,
2. `volumeSteps[]` daraus initialisieren,
3. nur abweichende Kanäle erneut fahren,
4. danach erneut alle vier Kanäle verifizieren.

### Endergebnis

Nach zwei fehlgeschlagenen Verifikationen:

```text
AC_PROFILE_VERIFY_FAILED profile=<n> requested=... actual=... attempts=2
```

und Rückgabe ungleich `AC_SUCCESS`.

Damit propagiert der Fehler über `RasPiAutomation` und SSH bis `RasPiExecuteAsync()`.

---

## 9. Aufgabe F: `activeAudioProfile` konsistent behandeln

Prüfe, wann `aC_Data.activeAudioProfile` gesetzt wird.

Der aktuell angeforderte Profilindex muss im Log korrekt erscheinen. Setze den Wert nicht vorzeitig als erfolgreich aktiv, wenn die abschließende Verifikation noch nicht bestanden ist.

Empfohlen:

- angefordertes Profil lokal halten,
- `activeAudioProfile` erst nach erfolgreicher Verifikation aktualisieren,
- bestehende Persistenzsemantik nicht ungeprüft ändern.

Falls `activeAudioProfile` aktuell nur für Diagnose/Anzeige genutzt wird, dokumentiere dies im Code.

---

## 10. Aufgabe G: Ergebnis- und Fehlerbehandlung bereinigen

### Problem

Im bestehenden Code werden dezimale Fehlercodes mehrfach mit `|=` kombiniert:

```c
result |= AC_GetVolume();
result |= AC_Control();
```

Die definierten Werte `31..38` sind keine eindeutigen Einzelbits. Bitweises OR kann daher einen Wert erzeugen, der keinem tatsächlichen Einzelfehler entspricht.

### Auftrag

Prüfe und bereinige die Fehlerweitergabe für den geänderten Pfad, ohne unnötig das gesamte Projekt umzubauen.

Bevorzugte Minimalregel:

```c
if (result == AC_SUCCESS)
{
    result = NextOperation();
}
```

Oder:

```c
unsigned char stepResult = NextOperation();
if (result == AC_SUCCESS && stepResult != AC_SUCCESS)
{
    result = stepResult;
}
```

Der erste konkrete Fehler soll erhalten bleiben.

Keine globale Umnummerierung bestehender Fehlercodes ohne zwingenden Grund.

---

## 11. Aufgabe H: COM-Port korrekt schließen

Aktuell wird verwendet:

```c
uart0_filestream = close(uart0_filestream);
```

`close()` liefert bei Erfolg `0`. Dadurch wird der globale Descriptor auf `0` gesetzt. Das funktioniert mit der aktuellen Prüfung `<= 0`, ist aber semantisch unsauber.

Ändere kontrolliert auf:

```c
if (uart0_filestream >= 0)
{
    close(uart0_filestream);
    uart0_filestream = -1;
}
```

Passe die Öffnungsprüfung entsprechend auf `< 0` an.

Prüfe alle Stellen, an denen `uart0_filestream` geöffnet, geprüft oder geschlossen wird.

---

## 12. Aufgabe I: Logging vervollständigen

Für jeden Profilwechsel müssen folgende Marker existieren:

```text
AC_PROFILE_BEGIN
AC_PROFILE_REQUEST
AC_PROFILE_READ_BEFORE
AC_PROFILE_WRITE_BEGIN
AC_PROFILE_WRITE_END
AC_PROFILE_READ_AFTER
AC_PROFILE_VERIFY
AC_PROFILE_RETRY
AC_PROFILE_SUCCESS
AC_PROFILE_FAIL
```

Jeder Marker enthält mindestens:

- PID
- Profilindex
- Versuch `1` oder `2`
- Sollwerte
- Istwerte, sofern verfügbar
- konkreten Ergebnis-/Fehlercode

Beispiel:

```text
AC_PROFILE_SUCCESS pid=6481 profile=2 attempt=1 requested=0,50,0,31 actual=0,50,0,31 result=0
```

---

## 13. Tests

Ergänze hardwarefreie Tests für die neuen reinen Hilfsfunktionen.

### Parsing

Müssen akzeptiert werden:

```text
(VOL1:00)
(VOL2:50)
(VOL3:63)
(VOL4:31)
```

Müssen abgelehnt werden:

```text
(VOL1:00) bei expectedChannel=2
(VOL2:99)
(VOL2:5)
(VOL2:AA)
VOL2:50
(VOL2:50
leere Antwort
unvollständige Antwort
Antwort mit Zusatzbytes
```

### Profilvergleich

Testfälle:

```text
Soll 0,50,0,31 / Ist 0,50,0,31 -> Erfolg
Soll 0,50,0,31 / Ist 0,31,0,56 -> Fehler
Soll 0,50,0,31 / Ist 0,50,0,30 -> Fehler
```

### Retry-Zustandsautomat

Testfälle:

1. Erster Versuch erreicht Soll -> kein Retry.
2. Erster Versuch gültig, aber nicht im Soll; zweiter erreicht Soll -> Erfolg.
3. Beide Versuche gültig, aber nicht im Soll -> `AC_PROFILE_VERIFY_FAILED`.
4. Kanalantwort bleibt ungültig -> kein blinder Profil-Retry, konkreter Kommunikationsfehler.
5. Partieller Write -> `AC_TX_FAILED`.

Wenn der C-Code aktuell kein Test-Target besitzt, lagere Parsing und Soll-Ist-Vergleich in kleine reine Funktionen aus, die von einem einfachen nativen Testprogramm aufgerufen werden können. Keine Hardwarezugriffe in diesen Tests.

---

## 14. Produktiver Abnahmetest

Nach Build und Deployment auf dem Raspberry Pi:

### Testsequenz

Mehrfach abwechselnd ausführen:

```text
Gottesdienst -> Predigt -> Gottesdienst -> Predigt
```

Danach über JokiAutomation:

```text
PositionControl | Kanzel | GP
```

beziehungsweise den tatsächlich gewünschten Kamera-/Audiocode verwenden.

### Erwartetes AudioControl-Log

```text
COMMAND command=30 index=114
AC_PROFILE_BEGIN profile=2
AC_PROFILE_REQUEST requested=0,50,0,31
AC_PROFILE_READ_BEFORE actual=<vorheriger Zustand>
AC_PROFILE_WRITE_BEGIN
AC_PROFILE_WRITE_END
AC_PROFILE_READ_AFTER actual=0,50,0,31
AC_PROFILE_VERIFY match=1
AC_PROFILE_SUCCESS result=0
```

### Fehlerfall

Wenn der Mixer das Profil nach dem ersten Schreiben nicht übernommen hat:

```text
AC_PROFILE_VERIFY match=0 attempt=1
AC_PROFILE_RETRY attempt=2
...
AC_PROFILE_SUCCESS attempt=2
```

oder, wenn auch der zweite Versuch fehlschlägt:

```text
AC_PROFILE_FAIL result=AC_PROFILE_VERIFY_FAILED
```

JokiAutomation muss dann über den SSH-ExitCode einen Fehler erhalten und darf den Profilwechsel nicht mehr als erfolgreich darstellen.

---

## 15. Abnahmekriterien

Der Auftrag ist nur abgeschlossen, wenn alle folgenden Punkte erfüllt sind:

- [ ] `AC_write()` erkennt partielle Writes.
- [ ] Lautstärkeantworten werden kanalgenau und formatstreng validiert.
- [ ] Ungültige Antworten werden nicht in `volumeState[]` übernommen.
- [ ] `AC_GetVolume()` verwendet höchstens zwei Versuche je Kanal.
- [ ] Nach `AC_Control()` erfolgt eine vollständige Rücklesung aller vier Kanäle.
- [ ] Erfolg wird nur bei vollständig erreichtem Sollprofil gemeldet.
- [ ] Es gibt maximal einen kontrollierten Profil-Retry.
- [ ] Nach zwei Soll-Ist-Abweichungen wird ein Fehler zurückgegeben.
- [ ] Der konkrete Fehler propagiert bis zum Prozess-Exitcode.
- [ ] `uart0_filestream` wird sauber auf `-1` zurückgesetzt.
- [ ] Neue Logs unterscheiden Übertragung, Rücklesung, Verifikation und Retry.
- [ ] Parser- und Profilvergleichstests sind grün.
- [ ] Die bestehende Fade-Logik und die gespeicherten Profilwerte bleiben unverändert.
- [ ] Keine Änderungen außerhalb des Raspberry-AudioControl-Scopes.

---

## 16. Erwartete Ausgabe von GitHub Copilot

Nach Umsetzung bitte liefern:

1. Liste aller geänderten Dateien.
2. Präzise Zusammenfassung jeder Änderung.
3. Diff der geänderten Funktionen.
4. Ergebnis des Builds.
5. Ergebnis der hardwarefreien Tests.
6. Beispiel eines erfolgreichen Verifikationslogs.
7. Noch offene Punkte, die nur mit realem Audiomixer getestet werden können.

Keine weiteren Architekturänderungen vorschlagen oder selbständig durchführen.
