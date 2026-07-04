# Setup-Log: Code Coverage für Embedded Software

Laufendes Log technischer Entscheidungen und Ergebnisse für die Masterarbeit.

---

## Hardware

- STM32: NUCLEO-F411RE (ARM Cortex-M4, Onboard-ST-LINK)
- AVR: AZDelivery AZ-MEGA2560 (ATmega2560, 8 KB RAM, CH340 USB-Seriell-Chip)

Begründung: zwei architektonisch verschiedene Plattformen (32-bit ARM vs. 8-bit AVR), beide bare-metal- und RTOS-tauglich, keine Spezialhardware nötig. Mega2560 statt Uno gewählt (8 KB statt 2 KB RAM), für RTOS-Betrieb notwendig.

## Entwicklungsumgebung

- Dual-Boot Ubuntu/Windows, kein WSL2. Begründung: nativer USB-Zugriff auf ST-LINK und CH340-Chip ohne Passthrough-Fehlerquelle

Toolchains, verifiziert:

| Tool | Version |
|---|---|
| arm-none-eabi-gcc | 14.2.1 |
| gdb-multiarch | 17.1 |
| openocd | 0.12.0 |
| avr-gcc | 14.3.0 |
| avrdude | 7.1 |
| make | 4.4.1 |
| python3 | 3.14.4 |
| git | 2.53.0 |

---

## M1: Toolchains und Minimal-Build (Task T3)

Cross-Compile für beide Zielarchitekturen verifiziert (file zeigt korrektes ELF-Format: ARM Cortex-M4 bzw. AVR 8-bit). Reales Flashen auf Mega2560 erfolgreich (siehe unten). STM32-Flashen offen, Nucleo-Kabel fehlt noch.

## M2: Static-Library-Grundgerüst, PC-Teil (Task T4)

Erster End-to-End-Beweis des Kernkonzepts (Bit-pro-Probe, Trennung Target/Host, vgl. Proposal Abschnitt 10), zunächst auf dem PC.

- coverage.h/coverage.c: Bitmap-basierte Erfassung, cov_mark(id) setzt gezielt ein Bit, cov_dump() gibt rohe Probe-IDs aus
- Testanwendung mit einer bewusst nie aufgerufenen Funktion, um eine echte Coverage-Lücke zu erzeugen
- Host-Simulation: Probe-IDs zu Datei, manuelle ID zu Datei/Zeile-Map, Python-Skript baut lesbaren Report
- Ergebnis: 3/4 Probes covered (75 Prozent), Lücke korrekt erkannt

## M7: Parser-Auswahl und Prototyp (Task T7)

Werkzeugwahl: tree-sitter (tree-sitter-c) bestätigt. Fehlertoleranter Parser, exakte Byte-Offsets, kein vollständiger Präprozessor-Lauf nötig, robust gegenüber herstellerspezifischen Headern.

- AST-Visualisierung: korrekte Strukturerkennung bestätigt
- Automatische Statement-Erkennung mit exakten Zeilennummern über node.start_point
- Wichtige Erkenntnis: ursprüngliche M2-Instrumentierung lieferte faktisch nur Function Coverage (1 Probe pro Funktion), nicht Statement Coverage. Relevante Abgrenzung für die Arbeit
- Automatisches Instrumentierungs-Tool (instrument.py): findet Statement-Knoten, fügt cov_mark(id) rückwärts ein (vermeidet Byte-Offset-Verschiebung), erzeugt ID-Map automatisch. Ergebnis: echte Statement-Granularität (6 statt 4 Probes)
- Terminierungsproblem live beobachtet (FF3): Position eines manuell platzierten cov_dump() beeinflusst direkt, welche danach liegenden Probes noch erfasst werden. End-to-End-Test: 4/6 (66,7 Prozent) covered
- Offen: Ausschlussregel (cov_dump() selbst nicht instrumentieren), Integration mit Host-Kommando-Terminierung (M4), Anwendung auf echten STM32/AVR-Zielcode

---

## AVR: UART-Transport und Coverage over UART (Task T4/T5, FF1/FF2)

Umsetzung des plattformspezifischen UART-Transports aus Proposal Abschnitt 10.3 für den ATmega2560.

UART-Grundfunktionen (avr/uart.c, avr/uart.h): direkte Registerprogrammierung (UBRR0H/L, UCSR0B, UCSR0C, UDR0), kein Arduino-Framework. uart_init() (9600 Baud, 8N1), uart_putchar() (Polling auf UDRE0), uart_puts(). Kompiliert mit -Os (Pflicht für korrektes _delay_ms()-Timing). Verifiziert per screen /dev/ttyUSB0 9600, stabile Textausgabe vom Board.

Coverage-Kern erweitert, ohne Kapselung zu brechen: neue Funktion cov_is_covered(id) in coverage.c/coverage.h (reiner Lesezugriff auf die Bitmap, keine Ausgabelogik). Ermöglicht plattformspezifischem Code eigene Dump-Formate zu bauen, ohne dass der portable Kern UART-Wissen braucht.

avr/coverage_dump_uart.c: AVR-Gegenstück zu cov_dump(), sendet covered Probe-IDs über UART statt printf (zu schwergewichtig für AVR, vgl. Proposal Abschnitt 3).

Ergebnis, erster End-to-End-Beweis auf Hardware: Testanwendung mit vier Probes (eine bewusst nie erreicht) auf Mega2560 geflasht, über UART empfangen:

    coverage dump start
    0
    1
    3
    coverage dump end

Probe 2 (unused_function) korrekt als Lücke sichtbar, identisches Muster wie PC-Test aus M2, jetzt real auf dem Target.

Bootloader-Eigenheit: Mega2560-Bootloader unterstützt keinen Chip-Erase, avrdude -D erforderlich, sonst command failed.

Offen: Terminierung noch nicht über Host-Kommando gelöst (M4), STM32-Portierung offen, Integration mit automatischer Instrumentierung aus M7.

---

## Repository

GitHub: MasterThesis-CodeCoverageTool, SSH eingerichtet, aktueller Stand (M1/M2/M7 und AVR) gepusht.

---

## AVR: Interrupt-sicheres cov_mark und Host-Kommando-Terminierung (Task T4, Bezug FF3, funktionale Anforderung 6)

Umsetzung der in Proposal Abschnitt 10.2 vorgesehenen Terminierungsstrategie 1 (Host-Kommando über UART-Interrupt).

- uart.c erweitert um uart_enable_rx_interrupt() (RXEN0, RXCIE0, globale Interrupts über sei())
- cov_mark() in coverage.c interrupt-sicher gemacht: atomares Bit-Setzen über ATOMIC_BLOCK (util/atomic.h), nur auf AVR aktiv (bedingt über Makro __AVR__), PC-Version unverändert
- ISR (USART0_RX_vect) liest empfangenes Byte, setzt nur bei Byte 'D' ein Flag (volatile), Dump-Logik läuft in main(), nicht in der ISR selbst
- Verifiziert: beliebige andere Tasten lösen keinen Dump aus, nur 'D' triggert korrekt

## M7-Integration: automatisch instrumentierter Code auf AVR (Task T7)

Erstmalige Verbindung von Parser-Prototyp (M7) und AVR-Zielcode.

- raw_main.c bereinigt (main entfernt, nur noch Zielfunktionen add, subtract, unused_function), damit instrument.py sauber nur den eigentlichen Zielcode instrumentiert, nicht den AVR-Treiber
- instrument.py auf bereinigten raw_main.c angewendet, 3 Probes automatisch eingefügt, probe_map_auto.csv automatisch erzeugt
- Neuer AVR-Treiber (m7_integration_main.c) ruft die automatisch instrumentierten Funktionen auf, unused_function bewusst nicht aufgerufen
- Ergebnis auf Hardware: IDs 0 und 1 covered, 2 korrekt als Lücke sichtbar, ausgelöst per Host-Kommando 'D'

## Host-Anwendung: automatischer Report (Task T4, funktionale Anforderung 5)

Neuer Ordner host, erste lauffähige CLI-Host-Anwendung (Python, pyserial).

- uart_host_report.py verbindet sich über seriellen Port, sendet 'D', liest die Dump-Ausgabe, ordnet empfangene IDs über probe_map_auto.csv zurück auf Datei, Zeile, Funktion zu
- Ergebnis: automatisch erzeugter, lesbarer Report ohne manuelles Mitlesen über screen
- Bekannte Eigenheit: Board resettet sich beim Öffnen der seriellen Verbindung (CH340-Verhalten), kurze Wartezeit im Skript nötig

**Status:** komplette Kette erstmals End-to-End auf Hardware funktionsfähig. Quellcode, automatische Instrumentierung, Compile, Flash, Host-Kommando-Terminierung, interrupt-sicheres cov_mark, automatischer Report.

## M7: Ausschlussregel fuer cov_dump (Bugfix)

instrument.py instrumentiert jetzt keine Aufrufe von cov_dump() oder cov_dump_uart() mehr selbst. Verifiziert mit Testfall (eigene Funktion mit cov_dump()-Aufruf, blieb korrekt unangetastet, alle anderen Statements weiterhin instrumentiert).

## M8: Intrusiveness auf AVR gemessen (Bezug FF2)

Flash und RAM Overhead per avr-size, Vergleich Baseline (keine Instrumentierung) gegen instrumentierte Version. Erste Messung stark verzerrt durch ungenutzten printf-Pfad in cov_dump() (1708 Bytes Differenz). Mit -ffunction-sections, -fdata-sections und -Wl,--gc-sections bereinigt (entfernt nicht erreichbaren Code):

Flash: 370 auf 498 Bytes, plus 128 Bytes fuer 3 Probes.
RAM (bss): 0 auf 8 Bytes, entspricht exakt der Bitmap-Groesse bei COV_MAX_PROBES=64.

Laufzeit-Overhead von cov_mark() zyklengenau simuliert mit simulavr (Ziel atmega2560, 16 MHz), nicht nur aus dem Datenblatt geschaetzt.

Normalfall (gueltige ID, Beispiel id=3): 54 Taktzyklen, ca. 3,4 Mikrosekunden. Davon 44 Zyklen (ca. 2,75 Mikrosekunden) mit gesperrten Interrupts (ATOMIC_BLOCK), relevant fuer den probe effect aus Proposal Abschnitt 3.

Kurzschluss-Pfad (ungueltige ID): 10 Zyklen, ca. 0,6 Mikrosekunden.

Beobachtung: RET braucht auf dem ATmega2560 5 statt 4 Zyklen wegen 3-Byte-Ruecksprungadresse (grosses Flash), relevant fuer den Architekturvergleich in FF4.

cov_dump_uart() nicht sinnvoll zyklengenau messbar, Dauer fast vollstaendig durch UART-Uebertragungszeit dominiert, nicht durch eigenen Code. Bei 9600 Baud rechnerisch ca. 1,04 ms pro Byte, ein typischer Dump mit 3 Probes und Kopf/Fusszeile ca. 65 Zeichen, also ca. 68 ms Gesamtdauer. Fuer den probe effect nicht relevant, da Dump erst nach Terminierung laeuft, nicht waehrend der eigentlichen Messung.

**Status:** Intrusiveness-Metriken fuer AVR vollstaendig erfasst (Flash, RAM, Zyklen best/worst case). Referenzmessung fuer spaeteren Vergleich mit Gcov (M8, spaeter) und mit STM32.

## M8: Gcov-Vergleich auf AVR und PC (Bezug FF2, Proposal Abschnitt 3)

Direkter Test von GCCs eingebauter Coverage-Instrumentierung (--coverage Flag) auf beiden Plattformen, mit identischem Testcode (add, subtract, unused_function).

| | PC (x86_64) | AVR (ATmega2560) |
|---|---|---|
| gcc/avr-gcc --coverage | laeuft durch, .gcda/.gcno erzeugt (156/790 Bytes) | Linker-Fehler: undefined reference to __gcov_exit |
| Grund | Dateisystem und definiertes Programmende vorhanden | keine libgcov-Runtime fuer AVR, kein Dateisystem, Endlosschleifen ohne Programmende |

**Ergebnis:** Gcov ist auf AVR ohne erhebliche Zusatzarbeit (eigene __gcov_exit-Implementierung, die z.B. ueber UART sendet statt Datei zu schreiben) gar nicht einsetzbar, nicht nur umstaendlich. Bestaetigt empirisch die in Proposal Abschnitt 3 und 6 beschriebene Forschungsluecke. Eigenes Tool (128 Bytes Flash, 8 Bytes RAM, ca. 3,4 Mikrosekunden pro Probe, siehe oben) loest genau dieses Problem mit vertretbarem Overhead.
