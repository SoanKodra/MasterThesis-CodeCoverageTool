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

## Offene Punkte

- [ ] STM32/Nucleo: Kabel besorgen, Flashen und UART-Transport analog zu AVR nachziehen
- [ ] Terminierung über Host-Kommando (M4)
- [ ] Automatische Instrumentierung (M7) mit UART-Dump-Pfad verbinden
- [ ] cov_dump()-Aufruf selbst von Instrumentierung ausschließen
