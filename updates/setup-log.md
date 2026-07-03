# Setup-Log: Code Coverage für Embedded Software

Laufendes Log aller Entscheidungen und Setup-Schritte für Thesis-Kapitel "Entwicklungsumgebung / Setup".

---

## 30.06.2026 — Hardwareauswahl und Entwicklungsumgebung

**Betreuer-Feedback:** Konzept-Bericht abgesegnet ("Konzept gut, umsetzen") → Start der Umsetzungsphase.

**Hardware festgelegt und bestellt:**
- **STM32:** NUCLEO-F411RE (ARM Cortex-M4, Onboard-ST-LINK + Virtual COM Port, Mini-USB-B) — ~35 € inkl. Versand
- **AVR:** AZDelivery AZ-MEGA2560, USB-C, CH340-Chip (ATmega2560, 8 KB RAM) — ~13 €

**Auswahlbegründung:**
- Zwei verschiedene Architekturen (32-bit ARM vs. 8-bit AVR)
- Beide bare-metal- und RTOS-tauglich
- Beide mit Debugger/USB-Serial onboard, keine Spezialhardware nötig
- Mega bewusst statt Uno gewählt: 8 KB RAM vs. 2 KB beim Uno → Uno wäre für RTOS-Betrieb zu eng
- Noch zu besorgen: Mini-USB-B-Kabel für die Nucleo

**Entwicklungsumgebung:**
- Linux, Dual-Boot Ubuntu neben Windows 11
- Bewusst **kein WSL2** — Begründung: reibungsloser nativer USB-Zugriff auf die Boards (ST-LINK, CH340-Chip), WSL2-USB-Passthrough gilt als potenzielle Fehlerquelle bei seriellen Verbindungen/Debuggern

**Geplante Toolchains:**
- STM32: `gcc-arm-none-eabi`, `gdb-multiarch`, `openocd`
- AVR: `gcc-avr`, `avr-libc`, `avrdude`
- Allgemein: `make`, `python3`, `git`

**Reihenfolge (ohne Hardware-Zeitdruck):**
Ubuntu-Setup → M1 (Toolchains, Minimal-Build) → M2 (PC-Teil der Bibliothek) → Parser; Boards erst ab M2 (Nucleo) bzw. M6 (AVR) benötigt.

---

## 01.07.2026 — Ubuntu-Einrichtung (Ersteinrichtung nach Dual-Boot)

**Ausgangslage:** Dual-Boot war bereits fertig eingerichtet (separate Session). RTX 5070 GPU, NVIDIA-Treiber 595.71.05 bereits vorab installiert und verifiziert.

**Verifikationen:**
- `nvidia-smi` → Treiber 595.71.05 aktiv, RTX 5070 korrekt erkannt, GPU im normalen Idle-Betrieb (41°C, 9% Util) — Grafikkarten-Setup bestätigt funktionsfähig, keine weitere Aktion nötig.

**Setup-Fix 1: Zeitsynchronisation (Windows ↔ Linux)**
- Problem: Windows liest Hardware-Uhr (RTC) als Lokalzeit, Linux standardmäßig als UTC → Zeitsprünge beim Dual-Boot-Wechsel
- Befehl: `sudo timedatectl set-local-rtc 1`
- Verifiziert über `timedatectl`: `RTC in local TZ` von `no` → `yes`, Local time und RTC time stimmen danach überein
- System-Warnung beim Setzen ist erwartetes Standardverhalten (Linux empfiehlt eigentlich UTC in RTC, aber Lokalzeit ist der pragmatische Standard-Fix für Dual-Boot mit Windows)
- Status: ✅ erledigt und verifiziert

**Setup-Fix 2: Serieller Zugriff auf die Boards (Gruppe `dialout`)**
- Hintergrund: USB-serielle Geräte (z.B. `/dev/ttyACM0` für die Nucleo) gehören standardmäßig der Gruppe `dialout`; ohne Mitgliedschaft → „Permission denied" beim Zugriff
- Befehl: `sudo usermod -aG dialout $USER`
- Verifiziert über `getent group dialout` → `soani` korrekt in der Systemdatenbank eingetragen
- **Bekanntes Verhalten:** Gruppenänderung wird erst nach vollständigem Neustart (nicht nur Ausloggen) in der aktiven Session wirksam — bei uns reichte ein einfaches Ausloggen/Wiedereinloggen nicht aus, `groups` zeigte `dialout` danach immer noch nicht an, obwohl die Systemdatenbank korrekt war
- Status: ⏳ Änderung gespeichert, aber in aktueller Session noch nicht aktiv. Nicht zeitkritisch, da Board-Zugriff laut Plan erst ab M2 benötigt wird. Wird beim nächsten ohnehin fälligen Neustart automatisch wirksam; vor erstem Board-Anschluss nochmal mit `groups` verifizieren.

---

## 01.07.2026 — M1: Toolchains installieren (STM32-Teil)

- `sudo apt update` ausgeführt — Paketquellen (Ubuntu offiziell, österreichischer Spiegelserver `at.archive.ubuntu.com`, Claude Desktop APT-Repo, Chrome-Repo, boot-repair-PPA) erfolgreich abgefragt
- STM32-Toolchain installiert: `sudo apt install gcc-arm-none-eabi gdb-multiarch openocd -y`
- Verifiziert:
  - `arm-none-eabi-gcc --version` → 14.2.1 (Paket 15:14.2.rel1-1) ✅
  - `gdb-multiarch --version` → 17.1 ✅
  - `openocd --version` → 0.12.0 ✅
- Status: STM32-Toolchain vollständig einsatzbereit für M1 (Minimal-Build)

## 01.07.2026 — M1: Toolchains installieren (AVR-Teil)

- AVR-Toolchain installiert: `sudo apt install gcc-avr avr-libc avrdude -y`
- Verifiziert:
  - `avr-gcc --version` → 14.3.0 ✅
  - `avrdude --version` → kennt das Flag nicht (nur Kurzoptionen wie `-v`), zeigt stattdessen Hilfetext; Versionsnummer war trotzdem am Ende der Ausgabe ersichtlich: 7.1 ✅
  - `avr-libc` ist reine Bibliothek ohne eigenes ausführbares Programm, kein direkter Versions-Check nötig
- Status: AVR-Toolchain vollständig einsatzbereit. Damit sind STM32- und AVR-Toolchain für M1 komplett installiert.

## 01.07.2026 — M1: Toolchains installieren (make, python3, git)

- `python3` bereits vorinstalliert: `3.14.4`
- `make` und `git` fehlten, nachinstalliert: `sudo apt install make git -y`
- Verifiziert: `make 4.4.1`, `git 2.53.0`

**Zusammenfassung M1 — vollständige Toolchain-Übersicht:**

| Tool | Version | Zweck |
|---|---|---|
| arm-none-eabi-gcc | 14.2.1 | ARM Cortex-M Compiler |
| gdb-multiarch | 17.1 | Debugger |
| openocd | 0.12.0 | STM32 Flash/Debug |
| avr-gcc | 14.3.0 | AVR Compiler |
| avrdude | 7.1 | AVR Flash |
| make | 4.4.1 | Build-Automatisierung |
| python3 | 3.14.4 | Parser-Skripte (später, T7) |
| git | 2.53.0 | Versionskontrolle |

Status: M1 (Toolchains) — Installation abgeschlossen. Noch offen: Minimal-Build-Test (lauffähiges Minimalprogramm, siehe Meilensteinplan M1).

## 01.07.2026 — M1: Minimal-Build-Test (Cross-Compile ohne Hardware)

Ziel: Nachweis, dass beide Toolchains funktionsfähigen Maschinencode für die jeweilige Zielarchitektur erzeugen — noch ohne angeschlossene Hardware (Boards erst ab 02.07. verfügbar).

**Projektstruktur angelegt:** `~/master-projekt/m1-test/`

**STM32-Test:**
- Minimalprogramm `main.c` erstellt (einfache Funktion mit `volatile int`, kein Peripheriezugriff)
- Kompiliert mit: `arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -c main.c -o main.o`
- Verifiziert mit `file main.o` → `ELF 32-bit LSB relocatable, ARM, EABI5 version 1 (SYSV), not stripped`
- Bestätigt: echter ARM-Maschinencode für Cortex-M4, Cross-Compiling von x86_64-Host erfolgreich

**AVR-Test:**
- Minimalprogramm `avr_main.c` erstellt (inkl. `avr/io.h`, Endlosschleife als typisches Embedded-Muster)
- Kompiliert mit: `avr-gcc -mmcu=atmega2560 -c avr_main.c -o avr_main.o`
- Verifiziert mit `file avr_main.o` → `ELF 32-bit LSB relocatable, Atmel AVR 8-bit, version 1 (SYSV), not stripped`
- Bestätigt: echter AVR-Maschinencode für ATmega2560

**Status M1:** Toolchain-Installation und Cross-Compile-Nachweis für beide Zielarchitekturen (ARM Cortex-M4 und AVR 8-bit) abgeschlossen. Offen: tatsächliches Flashen und Ausführen auf realer Hardware, sobald Boards verfügbar sind (Nucleo + Mega2560 erwartet 02.07.2026).

---

## 01.07.2026 — M2: Static-Library-Grundgerüst auf dem PC (Task T4)

Ziel: erster End-to-End-Beweis des Kernkonzepts aus Abschnitt 10 des Proposals (Bit-pro-Probe-Erfassung, Trennung Target/Host), noch komplett auf dem PC, ohne STM32-Linking.

**Projektstruktur:** `~/master-projekt/m2-static-lib/`

**Coverage-Bibliothek (`coverage.h` / `coverage.c`):**
- Bitmap-basierte Erfassung: 1 Bit pro Probe statt vollwertiger Zähler (`uint8_t bitmap[(COV_MAX_PROBES+7)/8]`)
- `cov_mark(id)` setzt per Bit-Shift und bitweisem ODER genau ein Bit, ohne andere Bits zu beeinflussen
- `cov_dump()` gibt nur rohe Probe-IDs aus (maschinenlesbar) — simuliert das, was später über UART zum Host ginge
- Beide Dateien mit ausführlichen Kommentaren versehen (Zweck jeder Funktion, Bezug zu Proposal-Abschnitten)

**Testanwendung (`main.c`):**
- Drei Funktionen (`add`, `subtract`, `unused_function`) je mit einem `cov_mark()`-Aufruf, plus `main` selbst
- `unused_function()` bewusst nie aufgerufen, um eine echte Coverage-Lücke zu erzeugen

**Host-Simulation:**
- `covered_ids.txt`: rohe Probe-IDs, per Shell-Umleitung erzeugt (`./cov_test > covered_ids.txt`)
- `probe_map.csv`: manuell angelegte ID→Datei/Zeile/Funktion-Zuordnung (simuliert späteren Parser-Output aus M7)
- `report.py`: Python-Skript, führt beide Quellen zusammen zu lesbarem Coverage-Report mit Prozentangabe, ebenfalls kommentiert

**Ergebnis:** Report zeigte korrekt 3/4 Probes covered (75%), `unused_function` klar als NOT COVERED erkennbar — bestätigt das Gesamtkonzept aus Abbildung 1 des Proposals funktioniert durchgängig.

**Status M2 (PC-Teil):** Grundgerüst der Bibliothek fertig, kommentiert und verifiziert. Alle vier Dateien (`coverage.h`, `coverage.c`, `main.c`, `report.py`) mit erklärenden Kommentaren versehen — Code dokumentiert sich damit selbst (Funktionszweck, Bezug zu Proposal-Abschnitten), unabhängig von diesem Log. Offen (2. Hälfte M2, laut Meilensteinplan): Linking gegen echte STM32-Anwendung, UART-Transport statt Datei-Umleitung — folgt sobald Board verfügbar ist.

---

## 01.07.2026 — M7: Erste tree-sitter Parser-Experimente (Task T7)

Ziel: praktisch erproben, ob die im Proposal (Abschnitt 10.4) getroffene Vorauswahl von tree-sitter als Parser für die automatische Instrumentierung trägt — noch ohne Hardware, rein als Python-Experiment.

**Projektstruktur:** `~/master-projekt/m7-parser/`

**Installation:**
- `pip install tree-sitter tree-sitter-c --break-system-packages`
- Verifiziert über direkten Python-Import beider Pakete, Installation unter `~/.local/lib/python3.14/site-packages/`

**Experiment 1 — AST-Visualisierung (`parse_test.py`):**
- C-Grammatik geladen (`Language(tree_sitter_c.language())`), Parser instanziiert
- `main.c` aus M2 eingelesen und geparst
- Rekursive Ausgabe des kompletten Syntaxbaums (Knotentyp + zugehöriger Quelltextausschnitt)
- Ergebnis: Parser erkennt Struktur korrekt bis auf Ausdrucksebene (z.B. `function_definition` → `compound_statement` → `expression_statement` → `call_expression`), inkl. sauberer Trennung von Code und Kommentaren

**Experiment 2 — automatische Statement-Erkennung mit Zeilennummern (`find_statements.py`):**
- Rekursiver Tree-Walk, der `expression_statement`- und `return_statement`-Knoten identifiziert
- Verfolgt nebenbei die aktuell umschließende Funktion (`function_declarator` → `identifier`)
- Nutzt `node.start_point` (0-indiziert, Zeile+Spalte) für exakte Zeilenangaben
- **Ergebnis:** Alle 11 Anweisungen in main.c korrekt mit exakter Zeile und Funktionszuordnung gefunden — deutlich präziser als die manuell geschätzten Zeilennummern in der `probe_map.csv` aus M2

**Bewertung:** Bestätigt die Eignung von tree-sitter für T7. Der nächste konzeptionelle Schritt (noch nicht umgesetzt) wäre, an den gefundenen Statement-Positionen automatisch `cov_mark(id)`-Aufrufe einzufügen (Textmanipulation an den `start_byte`-Positionen) und parallel dazu automatisch eine `probe_map.csv`-äquivalente Zuordnung zu erzeugen — das würde die manuelle Instrumentierung aus M2 vollständig ablösen.

**Status M7:** Erste Machbarkeits-Experimente erfolgreich, Vorauswahl tree-sitter praktisch bestätigt. Vollständiges Instrumentierungs-Tool (Textinsertion + ID-Map-Generierung) ist noch offen für eine spätere Session.

---

## 01.07.2026 — M7: Automatische Instrumentierung, echte Statement Coverage (Task T7)

Ziel: den Übergang von Function-Coverage-artiger, manueller Instrumentierung (M2) zu echter, automatischer Statement Coverage vollziehen — kompletter End-to-End-Beweis, noch ohne Hardware.

**Vorbereitung (`find_uninstrumented.py`):** Erweiterung des Statement-Finders um Erkennung, ob ein `expression_statement` bereits selbst ein `cov_mark()`-Aufruf ist (Baummuster `expression_statement → call_expression → identifier == "cov_mark"`). Ergebnis: deckte auf, dass die M2-Instrumentierung faktisch nur Function Coverage lieferte (1 Probe pro Funktion), nicht echte Statement Coverage (7 von 11 Anweisungen ohne eigene Probe) — wichtige konzeptionelle Erkenntnis für Abgrenzung Function- vs. Statement-Coverage.

**`raw_main.c` angelegt:** unberührte, nicht instrumentierte Version der Testanwendung (kein `cov_mark`, kein `#include`, kein `cov_dump`) — repräsentiert echten Zielcode vor Werkzeug-Einsatz.

**`instrument.py` — automatisches Instrumentierungs-Tool:**
- Findet alle Statement-Knoten per tree-sitter, sortiert nach Position im Quellcode
- Fügt **rückwärts** (letzte Position zuerst) `cov_mark(id)`-Aufrufe ein, um Byte-Offset-Verschiebungen durch frühere Einfügungen zu vermeiden
- Nutzt `bytearray` für In-Place-Texteinfügung an exakten `start_byte`-Positionen
- Erzeugt automatisch `probe_map_auto.csv` (ersetzt die manuelle Map-Erstellung aus M2 vollständig)
- Ergebnis: 6 Probes korrekt eingefügt (1× add, 1× subtract, 1× unused_function, 3× main) statt vorheriger 4 (M2) — echte Statement-Granularität erreicht

**Gefundener und behobener Bug (Zeilennummern-Verschiebung):**
- Erste Version verwies im `file`-Feld der CSV fälschlich auf `instrumented_main.c`, während die Zeilennummern aus `raw_main.c` stammten (vor Einfügung der Include-Zeilen berechnet) → Diskrepanz von 2 Zeilen
- Fix: `file`-Feld korrekt auf `raw_main.c` gesetzt, da die berechneten Zeilennummern sich konsistent auf diese unveränderte Originaldatei beziehen — konzeptionell auch der sinnvollere Bezug (Coverage-Report soll auf den von Menschen geschriebenen Quellcode verweisen, nicht auf eine interne Zwischendatei)

**Terminierungsproblem live erlebt (Bezug FF3):**
- Erster Testlauf lieferte leeren `covered_ids_v2.txt` — `raw_main.c` enthielt bewusst keinen `cov_dump()`-Aufruf (echter Zielcode "weiß" nichts von Coverage-Reporting)
- Als einfacher Test-Fix manuell `cov_dump()` vor der letzten Anweisung in `main()` platziert
- Beobachtung: Position des Dump-Aufrufs beeinflusst direkt, welche danach liegenden Probes noch erfasst werden — konkrete, kleine Illustration des allgemeinen Terminierungsproblems, das M4 strukturell lösen soll

**Ergebnis End-to-End-Test:** Kompiliert mit `gcc instrumented_main.c ../m2-static-lib/coverage.c -I../m2-static-lib -o cov_test_v2`. Report zeigte korrekt 4/6 (66.7%) — `unused_function` sowie die nach dem Dump liegende letzte `main`-Anweisung als NOT COVERED, exakt wie erwartet.

**Status M7:** Funktionierender Prototyp einer automatischen, statement-genauen Instrumentierung mit tree-sitter steht. Bewusst noch offen: Ausschlussregeln (z.B. `cov_dump()` selbst nicht instrumentieren), Integration mit echtem Host-Kommando-Terminierungsmechanismus aus M4, Anwendung auf STM32-Zielcode statt PC-Testcode.

---

## 01.07.2026 — Git-Repository initialisiert

Ziel: gesamten bisherigen Fortschritt (M1, M2, M7) unter Versionskontrolle bringen, bevor am Folgetag mit der echten Hardware weitergearbeitet wird.

- `git config --global Soan Kodra`/`soankodra@gmail.com` gesetzt
- `git init` in `~/master-projekt/` (Stolperfall unterwegs: erster Versuch versehentlich im Unterordner `updates/` ausgeführt, mit `rm -rf .git` rückgängig gemacht und am richtigen Ort neu initialisiert)
- `.gitignore` angelegt für generierte/kompilierte Artefakte (`*.o`, ausführbare Testprogramme, Programmausgaben wie `covered_ids*.txt`) — nur handgeschriebener Quellcode und Konfigurationsdateien werden versioniert
- Erster Commit: `git add .` + `git commit -m "Initial commit: M1 Toolchains, M2 PC-Bibliothek, M7 Parser-Prototyp"` — 16 Dateien, 665 Zeilen erfasst

**Status:** Kompletter Stand von M1/M2/M7 als Ausgangspunkt (root commit) gesichert. Ab jetzt: nach jedem sinnvollen Fortschritt (z.B. erstes Board-Flashen morgen) erneut `git add .` + `git commit -m "..."`.

## Offene Punkte für später

- [ ] `dialout`-Gruppenmitgliedschaft nach nächstem Reboot final verifizieren (vor erstem Nucleo-Anschluss, M2)
- [ ] Mini-USB-B-Kabel für Nucleo besorgen
- [ ] Sobald Hardware da ist (erwartet 02.07.2026): Boards anschließen, Geräteerkennung prüfen (`/dev/ttyACM0` bzw. `/dev/ttyUSB0`), erstes Flashen mit `openocd` (STM32) und `avrdude` (AVR) testen, M1 damit vollständig abschließen
- [x] ~~Windows-Schnellstart deaktivieren~~ — geprüft: war bereits deaktiviert. War nicht die Ursache für den beobachteten 2h-Zeitsprung, hat sich seither von selbst korrigiert.
