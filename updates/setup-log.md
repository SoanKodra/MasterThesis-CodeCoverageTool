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

## M5: RTOS-Variante auf AVR (Bezug FF4, funktionale Anforderung 6)

FreeRTOS-Kernel (feilipu/avrfreertos, MIT-lizenziert, explizite Mega2560-Unterstuetzung) eingebunden. Nur Kernkomponenten uebernommen (tasks.c, list.c, queue.c, port.c, heap_1.c), eigene schlanke FreeRTOSConfig.h geschrieben statt Beispielprojekt-Konfiguration zu uebernehmen.

Zwei Stolpersteine beim Einrichten:
- Board-Konfigurationsdatei (FreeRTOSBoardDefs.h) waehlte standardmaessig Timer2 mit externem 32,768-Hz-Quarz als Tick-Quelle, den das Board nicht hat. Umgestellt auf Timer1 (interner Timer, kein Zusatzhardware noetig).
- Fehlender Header (timers.h) trotz deaktivierter Timer-Funktionalitaet (configUSE_TIMERS 0), nachtraeglich ergaenzt.

Zwei Tasks umgesetzt: TaskApplication (ruft alle 500ms add/subtract auf, unused_function bewusst nicht, Tick-Rate ueber vTaskDelayUntil), TaskDumpHandler (pollt alle 50ms auf das per UART-Interrupt gesetzte Dump-Flag, fuehrt cov_dump_uart aus).

Task-Sicherheit der Bitmap (funktionale Anforderung 6, jetzt auch fuer Task-Konkurrenz statt nur ISR-Konkurrenz): bestehendes ATOMIC_BLOCK in cov_mark() deaktiviert global alle Interrupts. Da der FreeRTOS-Scheduler auf AVR Tasks ausschliesslich ueber einen Timer-Interrupt (hier Timer1) wechselt, kann waehrend eines cov_mark()-Aufrufs kein Task-Wechsel stattfinden. Der bestehende Interrupt-Schutz aus M4 reicht daher ohne Aenderung auch fuer Task-zu-Task-Konkurrenz aus.

**Ergebnis auf Hardware:** Dump ueber Host-Kommando funktioniert identisch zur Bare-Metal-Version (0 und 1 covered, 2 korrekt als Luecke), jetzt erzeugt durch zwei nebenlaeufige Tasks statt einer linearen main()-Schleife.

**Vergleich Bare-Metal vs. RTOS (Flash/RAM, Bezug FF2/FF4):**

| | Bare-Metal (M8, instrumentiert) | RTOS (M5, instrumentiert) |
|---|---|---|
| Flash | 498 Bytes | 5448 Bytes |
| RAM | 8 Bytes | 4704 Bytes |

RTOS-Grundoverhead ist massiv groesser als die eigentliche Coverage-Instrumentierung (Scheduler, Task-Stacks, Heap-Verwaltung). Bei 256 KB Flash und 8 KB RAM auf dem Mega2560 bleibt Flash unproblematisch (ca. 2%), RAM wird mit ca. 57% Belegung durch configTOTAL_HEAP_SIZE (4608 Bytes fuer zwei Task-Stacks) zum knappen Gut, relevant fuer die Skalierbarkeit bei mehr/groesseren Tasks in einer echten Zielanwendung.

**Status:** M5 fuer AVR abgeschlossen. STM32-RTOS-Variante (FreeRTOS, offizieller Cortex-M-Port, deutlich besser unterstuetzt) folgt sobald Kabel verfuegbar.

## M1: STM32 Bare-Metal-Setup von Grund auf (Task T3)

Verwendete Quellen (Commit-Stand zum Zeitpunkt des Downloads):
- CMSIS-Device (STM32F4): github.com/STMicroelectronics/cmsis-device-f4
- CMSIS-Core: github.com/ARM-software/CMSIS_5

Nucleo-F411RE per ST-LINK erkannt (lsusb, openocd Verbindungstest: Cortex-M4 r0p1 erkannt, 6 Breakpoints, 4 Watchpoints). Kein STM32CubeIDE oder vorgefertigtes Projektgeruest verwendet, komplettes Setup manuell zusammengebaut.

CMSIS-Header von offiziellen Quellen geholt (STMicroelectronics/cmsis-device-f4 fuer Chip-spezifische Register, ARM-software/CMSIS_5 fuer Cortex-M4-Core-Header), gezielt benoetigte Dateien kopiert statt volles Repo einzubinden (in .gitignore).

Eigenes Linker-Script geschrieben (Flash 512KB ab 0x08000000, RAM 128KB ab 0x20000000, Vector Table, .data/.bss-Sektionen).

Zwei Stolpersteine beim ersten Kompilieren/Linken:
- __libc_init_array undefined reference mit -nostdlib: gefixt durch -nostartfiles und --specs=nosys.specs statt -nostdlib (schliesst nur Programmstart-Dateien aus, nicht die komplette C-Bibliothek).
- _init undefined reference (newlib erwartet OS-Unterbau): gefixt durch leeren _init()-Stub in main.c.

**Ergebnis:** Minimalprogramm (LED-Blink auf PA5) erfolgreich kompiliert (788 Bytes Flash), geflasht und verifiziert ueber openocd (program/verify/reset), LED blinkt sichtbar auf echter Hardware.

**Status:** M1 fuer STM32 abgeschlossen (Cross-Compile-Nachweis von vorher plus jetzt reales Flashen/Ausfuehren). Naechster Schritt: UART-Transport analog zu AVR.

## STM32: UART-Transport (Task T4)

USART2 auf Nucleo-F411RE aktiviert (PA2=TX, PA3=RX, werkseitig mit ST-LINK Virtual COM Port verbunden, kein externer USB-Seriell-Chip noetig, 115200 Baud). Ablauf analog zu AVR-Register-Programmierung, aber mit STM32-spezifischem Dreischritt: Takt einschalten (GPIOA und USART2 separat), Pins auf Alternate Function AF7 umschalten, dann USART konfigurieren.

Verifiziert per screen /dev/ttyACM0 115200, stabile Textausgabe vom Board.

## STM32: Coverage-Bitmap ueber UART (Task T4/T5, Bezug FF1/FF2)

coverage_dump_uart.c fuer STM32 geschrieben, identisches Prinzip wie bei AVR: sendet covered Probe-IDs ueber UART statt printf. Portabler Kern (coverage.c/coverage.h) komplett unveraendert wiederverwendet, keine Anpassung fuer STM32 noetig - bestaetigt die in Proposal Abschnitt 10.3 vorgesehene Trennung zwischen portablem Kern und Plattform-Schicht.

Erster Build-Versuch scheiterte mit undefined reference to 'end' (sprintf benoetigt malloc/Heap, Linker-Script hatte keinen Heap-Bereich definiert). Gefixt durch Ergaenzung eines end/_end-Symbols am Ende der .bss-Sektion im Linker-Script.

Ergebnis (Testablauf mit fester Wartezeit statt Host-Kommando): 0 und 1 covered, 2 korrekt als Luecke. Verifiziert per GDB/OpenOCD-Debugging (Breakpoint auf cov_dump_uart, Bitmap-Inhalt direkt im RAM inspiziert: korrekt 0x03 nach den zwei cov_mark-Aufrufen).

## STM32: Host-Kommando-Terminierung ueber USART2-Interrupt (Task T4, Bezug FF3, funktionale Anforderung 6)

Analog zu AVR: USART2 RX-Interrupt aktiviert (RXNEIE-Bit, NVIC_EnableIRQ), USART2_IRQHandler ueberschreibt den weak Default-Handler aus dem Startup-Code, prueft auf Kommando 'D', setzt volatile Flag, Dump-Logik laeuft in der Hauptschleife.

Drei Bugs beim ersten Durchlauf, alle per GDB/OpenOCD auf Register-Ebene systematisch eingegrenzt statt geraten:

1. Verschachtelter Funktionsklammerfehler: fehlende schliessende Klammer nach uart_puts() liess uart_enable_rx_interrupt() als verschachtelte (in C ungueltige) Funktion kompilieren. Erkannt durch leere nm-Ausgabe fuer das erwartete Symbol trotz vorhandenem Quellcode.

2. OpenOCD-Prozess-Konflikt: zweite OpenOCD-Instanz konnte nicht starten, weil eine vorherige Debug-Session (GDB) den ST-LINK noch exklusiv belegte. Gefixt durch gezieltes Beenden des Hintergrundprozesses (pkill openocd).

3. Eigentlicher Funktionsfehler: USART_CR1_RE (Receiver Enable) fehlte in der CR1-Konfiguration. NVIC korrekt aktiviert (ISER1 Bit 6 gesetzt, verifiziert per direktem Registerzugriff auf 0xE000E104), RXNEIE korrekt gesetzt, aber der Empfaenger selbst war nie eingeschaltet, weshalb nie ein Byte empfangen und folglich nie ein Interrupt ausgeloest wurde. Gefixt durch Ergaenzung von USART_CR1_RE in der CR1-Initialisierung.

**Debugging-Methode:** GDB (gdb-multiarch) verbunden ueber OpenOCD-GDB-Server (Terminal-Trennung: OpenOCD als Server in einem Terminal, GDB-Client in einem zweiten, screen-Test in einem dritten). Systematische Eingrenzung durch Registerwerte direkt im Speicher inspiziert (print/x auf rohe Peripherie-Adressen), statt am Anwendungscode zu raten - identifiziert das fehlende Bit exakt und ohne Mehrdeutigkeit.

**Ergebnis:** Host-Kommando-Terminierung ueber USART2-Interrupt funktioniert identisch zu AVR (Tastendruck 'D' loest Dump aus, andere Zeichen werden ignoriert).

**Status:** STM32 hat jetzt denselben End-to-End-Stand wie AVR: UART-Transport, Coverage-Bitmap-Dump, Host-Kommando-Terminierung ueber Interrupt.

## STM32: Gcov-Vergleich (Bezug FF2, Proposal Abschnitt 3)

Gleicher Test wie bei AVR (siehe oben): arm-none-eabi-gcc --coverage auf identischem Testcode (add, subtract, unused_function).

**Ergebnis:** identischer Fehler wie bei AVR - undefined reference to __gcov_exit beim Linken. Bestaetigt, dass das Problem nicht an einer einzelnen schwachen Toolchain liegt (z.B. AVR als 8-bit-Architektur mit wenig Ressourcen), sondern strukturell fuer Bare-Metal-Embedded-Targets generell gilt, unabhaengig von Rechenleistung/Architektur - fehlendes Dateisystem und fehlende Betriebssystem-Anbindung fuer libgcov's Exit-Handler betreffen sowohl das einfache 8-bit AVR als auch das deutlich potentere 32-bit Cortex-M4 gleichermassen.

**Vergleichstabelle (aktualisiert, ergaenzt um STM32):**

| | PC (x86_64) | AVR (ATmega2560) | STM32 (Cortex-M4) |
|---|---|---|---|
| gcc/avr-gcc/arm-none-eabi-gcc --coverage | laeuft durch, .gcda/.gcno erzeugt | Linker-Fehler: undefined reference to __gcov_exit | Linker-Fehler: undefined reference to __gcov_exit (identisch) |
| Grund | Dateisystem und definiertes Programmende vorhanden | keine libgcov-Runtime, kein Dateisystem, Endlosschleifen ohne Programmende | dasselbe strukturelle Problem, unabhaengig von Architektur/Rechenleistung |

**Status:** Gcov-Vergleich fuer beide Zielarchitekturen abgeschlossen. Bestaetigt die Forschungsluecke aus Proposal Abschnitt 6 architekturuebergreifend, nicht nur fuer eine einzelne Plattform.

## STM32: Intrusiveness-Messung (Bezug FF2, Task T4)

Flash/RAM-Overhead per arm-none-eabi-size, Vergleich Baseline (keine Instrumentierung) gegen instrumentierte Version (drei cov_mark-Aufrufe), mit -ffunction-sections/-fdata-sections/--gc-sections wie bei AVR.

Flash: 628 auf 680 Bytes, plus 52 Bytes fuer 3 Probes.
RAM (bss): 0 auf 8 Bytes, identisch zu AVR (exakt die Bitmap-Groesse bei COV_MAX_PROBES=64, architekturunabhaengig).

Laufzeit-Overhead von cov_mark() gemessen mit dem eingebauten DWT-Zykluszaehler (Cortex-M-Hardware-Feature, CYCCNT-Register, ueber TRCENA/CYCCNTENA aktiviert) - kein Simulator noetig wie bei AVR (simulavr), direkte Hardware-Messung auf dem echten Chip.

Erste Messung waehrend aktiver GDB-Debug-Session ergab 31 Zyklen (durch Single-Step-Overhead leicht verfaelscht), im freien Lauf ohne Debugger korrekt 32 Zyklen, konstant reproduzierbar ueber viele Wiederholungen.

**Vergleich AVR vs. STM32 (identischer Testfall: 3 Probes, id=3 fuer Zyklenmessung):**

| | AVR (ATmega2560) | STM32 (Cortex-M4) |
|---|---|---|
| Flash-Overhead (3 Probes) | +128 Bytes | +52 Bytes |
| RAM-Overhead | +8 Bytes | +8 Bytes (identisch) |
| Zyklen (cov_mark, id=3) | 54 | 32 |
| Zeit bei 16 MHz | 3,4 Mikrosekunden | 2 Mikrosekunden |

STM32 durchgehend guenstiger in Flash und Zyklen bei identischer Funktionalitaet - erklaerbar durch den kompakteren/effizienteren Thumb-2-Instruktionssatz gegenueber dem AVR-Befehlssatz. RAM-Overhead architekturunabhaengig identisch, da rein von der Bitmap-Groesse bestimmt, nicht von der Instruktionsarchitektur.

**Status:** Intrusiveness-Metriken fuer STM32 vollstaendig erfasst (Flash, RAM, Zyklen), direkt vergleichbar mit AVR-Werten aus M8. Bestaetigt FF4 (Generalitaet) mit konkreten Zahlen: gleiches Konzept funktioniert auf beiden Architekturen, mit messbar unterschiedlichem aber in beiden Faellen vertretbarem Overhead.

## STM32: RTOS-Variante (Task T5, Bezug FF4, funktionale Anforderung 6)

Offizieller FreeRTOS-Kernel (github.com/FreeRTOS/FreeRTOS-Kernel, ARM_CM4F-Port) eingebunden, im Gegensatz zum AVR-Community-Fork (feilipu/avrfreertos) diesmal die autoritative Quelle direkt vom FreeRTOS-Projekt. Kernkomponenten kopiert (tasks.c, list.c, queue.c, timers.c, heap_1.c, portable/GCC/ARM_CM4F/port.c), eigene schlanke FreeRTOSConfig.h geschrieben (analog zu AVR, aber mit Cortex-M-spezifischen NVIC-Prioritaetseinstellungen configKERNEL_INTERRUPT_PRIORITY/configMAX_SYSCALL_INTERRUPT_PRIORITY, die der AVR-Port nicht braucht).

ARM_CM4F-Port verlangt explizit aktivierte Hardware-FPU (STM32F411RE ist Cortex-M4F, hat eine FPU) - neue Compiler-Flags -mfpu=fpv4-sp-d16 -mfloat-abi=hard erstmals genutzt.

Erster Kompilierversuch lief ueberraschend reibungslos durch (nur harmlose newlib-Warnungen wie gewohnt) - deutlich unkomplizierter als der AVR-Community-Fork mit seinen Board-Defs-Fallen.

**Zentraler Bug beim ersten Hardware-Test:** Programm haengte sich beim Scheduler-Start komplett auf (per GDB Single-Stepping durch xPortStartScheduler() bis prvPortStartFirstTask() verfolgt - der SVC-Aufruf zum Starten des ersten Tasks landete faelschlich im WWDG_IRQHandler statt im erwarteten SVC-Handler).

**Ursache:** neuere FreeRTOS-Kernel-Versionen nutzen "Direct Routing" - der Port erwartet, dass die Interrupt-Vector-Table direkt auf die FreeRTOS-eigenen Funktionsnamen zeigt (vPortSVCHandler, xPortPendSVHandler, xPortSysTickHandler), nicht auf die generischen CMSIS-Standardnamen (SVC_Handler, PendSV_Handler, SysTick_Handler), die der Startup-Code von ST als weak-Platzhalter bereitstellt. Ohne Anpassung ruft der Chip beim SVC-Interrupt weiterhin den leeren Default-Handler auf statt der FreeRTOS-Logik.

**Fix:** drei Zeilen im Startup-Assembler (cmsis/startup_stm32f411xe.s) direkt in der Vector-Table geaendert: SVC_Handler zu vPortSVCHandler, PendSV_Handler zu xPortPendSVHandler, SysTick_Handler zu xPortSysTickHandler. Gefunden durch systematisches GDB-Single-Stepping bis zur exakten Fehlstelle, dann gezielte Codesuche (grep) nach den tatsaechlich vom Port bereitgestellten Funktionsnamen statt der erwarteten Standardnamen.

**Ergebnis:** zwei nebenlaeufige Tasks (TaskApplication ruft alle 500ms add/subtract auf, TaskDumpHandler pollt auf Host-Kommando 'D' und dumpt), identisches Verhalten zur AVR-RTOS-Variante, mehrfach wiederholbar getestet.

**Flash/RAM (RTOS-Grundoverhead):** text=42104, bss=4668 Bytes. Bei 512KB Flash und 128KB RAM auf der F411RE deutlich entspannter als bei AVR (dort ca. 57% RAM-Auslastung durch den RTOS-Heap, hier nur ca. 3,6%) - erwartbar, da STM32 wesentlich mehr RAM zur Verfuegung hat.

**Status:** RTOS-Variante fuer beide Zielarchitekturen abgeschlossen (M5 vollstaendig). Bestaetigt FF4 (Generalitaet ueber Ausfuehrungsmodelle) fuer beide Plattformen.

## Proposal-Abgleich: identifizierte und geschlossene Luecken

Systematischer Abgleich des bisherigen Fortschritts gegen Proposal (Forschungsfragen, funktionale Anforderungen, Meilensteinplan) ergab drei konkrete Luecken, alle heute geschlossen:

### 1. M7-Integration auf STM32 nachgeholt

Bisher lief automatisch instrumentierter Code (aus instrument.py) nur auf AVR, nie auf STM32. instrumented_target.c aus m7-parser nach STM32 kopiert, neuer Treiber (m7_integration_main.c) gebaut und auf Hardware verifiziert: 0 und 1 covered, 2 korrekt als Luecke, identisch zu AVR.

**Nebenbefund beim Testen:** startup_stm32f411xe.s musste in zwei Varianten aufgeteilt werden (startup_stm32f411xe.s fuer Standard-Builds mit generischen CMSIS-Handlernamen, startup_stm32f411xe_rtos.s mit den FreeRTOS-Direct-Routing-Namen aus der gestrigen RTOS-Arbeit) - beide Build-Pfade nach der Trennung erneut auf Hardware verifiziert.

### 2. Host-Anwendung gegen STM32 verifiziert

uart_host_report.py lief bisher nur gegen AVR (/dev/ttyUSB0, 9600 Baud). Um Kommandozeilen-Parameter (--port, --baud) erweitert, funktioniert jetzt fuer beide Plattformen mit derselben Datei. Gegen STM32 (/dev/ttyACM0, 115200 Baud) getestet: identischer Report wie bei AVR (2/3 Probes, 66.7%).

### 3. Terminierungsstrategien-Vergleich vervollstaendigt (Bezug FF3)

Proposal Abschnitt 10.2 sieht vier Terminierungsstrategien als Vergleichspunkte vor. Bisher war nur Host-Kommando (M4) tatsaechlich gebaut. Die restlichen drei nachgeholt, alle auf AVR:

**Timer-Strategie:** Timer1 im CTC-Modus (Vorteiler 1024, 16 MHz), Compare-Match-Interrupt loest automatisch alle 3 Sekunden einen Dump aus, kein Host-Eingriff noetig. Verifiziert: wiederholter automatischer Dump im 3-Sekunden-Takt.

**Saettigungs-Strategie:** periodischer 1-Sekunden-Tick (gleicher Timer1-Mechanismus wie oben, aber nur als Pruefimpuls), Pruefsumme ueber alle gesetzten Bits verglichen, nach 3 Sekunden ohne Aenderung wird einmalig gedumpt. Zwei Iterationen noetig: erster Versuch loeste wiederholt aus (stable_ticks nicht persistent genug zurueckgesetzt), zweiter Versuch mit explizitem already_dumped-Flag korrekt: genau ein Dump, danach Stille.

**Idle-Erkennung:** FreeRTOS vApplicationIdleHook() genutzt (configUSE_IDLE_HOOK aktiviert), Dump wird ausgeloest sobald der Scheduler in den Idle-Task wechselt (Anwendungstask beendet sich selbst per vTaskDelete). Kleiner Stolperstein: vApplicationIdleHook() muss in FreeRTOSConfig.h deklariert werden (nicht in der eigenen main-Datei), da tasks.c nur die Config-Datei einbindet, nicht die Anwendungsdatei selbst. Nach Fix: einmaliger korrekter Dump.

**Status:** alle vier im Proposal genannten Terminierungsstrategien sind jetzt implementiert und auf Hardware verifiziert (Host-Kommando, Timer, Saettigung, Idle-Erkennung), womit FF3 vollstaendig empirisch beantwortet werden kann statt nur konzeptionell beschrieben zu sein.

