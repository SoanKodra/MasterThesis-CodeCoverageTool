# Setup-Log: Code Coverage für Embedded Software

Log für die Masterarbeit — technische Entscheidungen, Probleme, Lösungen.

---

## Grundlagen

**Hardware:**
- STM32 NUCLEO-F411RE (ARM Cortex-M4, 512KB Flash, 128KB RAM, Onboard-ST-LINK)
- AVR AZDelivery Mega2560 (ATmega2560, 256KB Flash, 8KB RAM, CH340 USB-Chip)

**Warum zwei Plattformen:** Zwei verschiedene Architekturen (32-bit ARM vs. 8-bit AVR), beide koennen bare-metal und mit RTOS laufen, keine Spezialhardware noetig. Mega2560 statt Uno gewaehlt wegen mehr RAM (8KB statt 2KB) — fuer RTOS-Betrieb wichtig.

**Entwicklungsumgebung:** Dual-Boot Ubuntu/Windows, kein WSL2 — direkter USB-Zugriff auf ST-LINK und CH340 ohne Passthrough-Probleme.

**Toolchains:**

| Tool | Version |
|---|---|
| arm-none-eabi-gcc | 14.2.1 |
| avr-gcc | 14.3.0 |
| gdb-multiarch | 17.1 |
| openocd | 0.12.0 |
| avrdude | 7.1 |
| python3 | 3.14.4 |
| git | 2.53.0 |

**Kernkonzept:** Statische Library mit Bit-pro-Probe-Bitmap (coverage.c/coverage.h), portabler Kern ist fuer beide Plattformen identisch. Nur UART-Transport und Terminierung sind plattformspezifisch (Proposal Abschnitt 10.3).

**Repository:** GitHub `MasterThesis-CodeCoverageTool`, SSH eingerichtet.

---

## M1: Toolchains und Minimal-Build

**AVR:** Cross-Compile verifiziert, echtes Flashen auf Mega2560 erfolgreich. Bootloader-Eigenheit: Mega2560 unterstuetzt keinen Chip-Erase, `avrdude -D` noetig sonst Fehler.

**STM32:** Kein CubeIDE, komplettes Setup manuell gebaut (CMSIS-Header von STMicroelectronics/cmsis-device-f4 und ARM-software/CMSIS_5, eigenes Linker-Script fuer Flash/RAM-Layout).

Zwei Linker-Probleme beim ersten Build:
- `__libc_init_array` fehlte mit `-nostdlib` → gefixt mit `-nostartfiles --specs=nosys.specs`
- `_init` fehlte (newlib erwartet OS) → leerer `_init()`-Stub ergaenzt

Ergebnis: LED-Blink-Test kompiliert, geflasht, verifiziert (788 Bytes Flash).

Spaeter musste die Startup-Datei in zwei Varianten aufgeteilt werden: `startup_stm32f411xe.s` (normale Builds) und `startup_stm32f411xe_rtos.s` (fuer FreeRTOS, siehe M5) — unterschiedliche Interrupt-Handler-Namen noetig.

---

## M2: Static Library + UART-Transport

**Kern (coverage.c/coverage.h):** Bitmap-basierte Erfassung, `cov_mark(id)` setzt ein Bit, `cov_dump()` gibt IDs aus (PC-Test). Spaeter erweitert um `cov_is_covered(id)` — reiner Lesezugriff, damit Plattform-Code eigene Dump-Formate bauen kann ohne die Bitmap-Kapselung zu brechen.

**AVR UART (uart.c):** Direkte Registerprogrammierung (UBRR, UCSR0B/C, UDR0), kein Arduino-Framework. 9600 Baud. Verifiziert per `screen`.

**STM32 UART (USART2):** PA2/PA3, werkseitig mit ST-LINK verbunden, kein externer Chip noetig. 115200 Baud. Dreischritt: Takt einschalten (GPIOA + USART2 getrennt), Pins auf Alternate Function AF7, USART konfigurieren.

**STM32-Linker-Problem:** `sprintf` (fuer Coverage-Dump) braucht Heap, Linker-Script hatte keinen definiert → `undefined reference to 'end'`. Gefixt mit `end`/`_end`-Symbol am Ende von `.bss`.

---

## M3: Manueller Prototyp End-to-End

Testanwendung mit 3-4 Funktionen, eine bewusst nie aufgerufen (Coverage-Luecke). `coverage_dump_uart.c` (AVR und STM32 getrennt) sendet Bitmap ueber UART statt printf (zu schwer fuer Embedded).

Ergebnis auf beiden Boards: covered IDs korrekt, Luecke korrekt erkannt. Erster echter End-to-End-Beweis auf Hardware (nicht nur PC-Simulation).

STM32-Debugging: Bitmap-Inhalt per GDB direkt im RAM verifiziert (Breakpoint auf `cov_dump_uart`).

---

## M4: Terminierung und Interrupt-Sicherheit

**Interrupt-sicheres cov_mark():** AVR nutzt `ATOMIC_BLOCK` (util/atomic.h), STM32 nutzt PRIMASK-Register direkt per Inline-Assembler (kein CMSIS-Header noetig, haelt Kern portabel). Beides bedingt kompiliert (`__AVR__` / `__arm__`), Kern bleibt fuer beide Plattformen eine Datei.

**Host-Kommando-Terminierung (beide Plattformen):** UART-Empfangs-Interrupt aktiviert, ISR prueft auf Byte `'D'`, setzt nur ein Flag, Dump laeuft in der Hauptschleife.

STM32 hatte drei Bugs beim ersten Versuch, gefunden per GDB/OpenOCD-Debugging (Registerwerte direkt inspiziert):
1. Fehlende Klammer nach `uart_puts()` liess die naechste Funktion als ungueltige verschachtelte Funktion kompilieren
2. Zwei OpenOCD-Instanzen blockierten sich gegenseitig
3. `USART_CR1_RE` (Receiver Enable) fehlte — Empfaenger war nie aktiviert, obwohl NVIC und RXNEIE korrekt gesetzt waren

**Host-Anwendung (Python, `host/uart_host_report.py`):** verbindet sich per UART, sendet `'D'`, liest Dump, bildet IDs ueber `probe_map_auto.csv` auf Datei/Zeile/Funktion ab. Erst nur fuer AVR getestet, spaeter um `--port`/`--baud`-Parameter erweitert und auch gegen STM32 verifiziert (gleiches Skript, beide Plattformen).

**Drei weitere Terminierungsstrategien (AVR, laut Proposal als Vergleich zu Host-Kommando gefordert):**
- **Timer:** Timer1 im CTC-Modus, loest automatisch alle 3 Sekunden aus, kein Host-Eingriff
- **Saettigung:** periodischer Check ob sich die Bitmap veraendert hat, nach 3 Sekunden ohne Aenderung einmaliger Dump (brauchte ein extra Flag, sonst loeste es wiederholt aus)
- **Idle-Erkennung:** nutzt FreeRTOS `vApplicationIdleHook()`, Dump wenn der Scheduler in den Idle-Task wechselt (Deklaration muss in FreeRTOSConfig.h stehen, nicht in der eigenen Datei, weil tasks.c nur die Config einbindet)

Alle vier Strategien aus dem Proposal sind jetzt gebaut und einzeln verifiziert. Noch offen: direkter quantitativer Vergleich (Flash/RAM/Timing) zwischen den vier Strategien.

---

## M5: RTOS-Variante

**AVR:** FreeRTOS von feilipu/avrfreertos (Community-Fork mit Mega2560-Support). Zwei Probleme: Board-Config wollte Timer2 mit externem Quarz nutzen (Board hat keinen) → auf Timer1 umgestellt; `timers.h` fehlte trotz deaktivierter Timer-Funktion. Zwei Tasks gebaut (Anwendung + Dump-Handler), funktioniert wie erwartet.

**STM32:** Offizieller FreeRTOS-Kernel (ARM_CM4F-Port), lief technisch reibungsloser als AVR. Brauchte FPU-Flags (`-mfpu=fpv4-sp-d16 -mfloat-abi=hard`), da F411 eine Hardware-FPU hat. Ein zentraler Bug: Scheduler haengte sich beim Start auf. Grund gefunden per GDB Single-Stepping: neuere FreeRTOS-Versionen nutzen "Direct Routing" und erwarten die Funktionsnamen `vPortSVCHandler`/`xPortPendSVHandler`/`xPortSysTickHandler` direkt in der Vector-Table, nicht die generischen CMSIS-Namen. Gefixt durch Anpassung der Vector-Table in der Startup-Datei.

**Vergleich Bare-Metal vs. RTOS (Flash/RAM):**

| | AVR Bare-Metal | AVR RTOS | STM32 Bare-Metal | STM32 RTOS |
|---|---|---|---|---|
| Flash | 498 B | 5448 B | 680 B | 42104 B |
| RAM | 8 B | 4704 B (57% von 8KB) | 8 B | 4668 B (3,6% von 128KB) |

RTOS-Overhead ist bei AVR ein echtes Problem (RAM knapp), bei STM32 unproblematisch (viel mehr RAM verfuegbar).

Interrupt-Sicherheit gilt auch fuer RTOS: da der Scheduler selbst nur ueber Timer-Interrupts arbeitet, schuetzt das bestehende ATOMIC_BLOCK/PRIMASK auch vor Task-Wechseln, keine Aenderung noetig.

---

## M6: Portierung

Reihenfolge im Proposal war STM32→AVR, tatsaechlich lief es umgekehrt: AVR zuerst komplett durchgezogen, dann STM32 nachgezogen (M1-M5 fuer beide Plattformen). Inhaltlich ist die Portierungsarbeit trotzdem gemacht, nur nicht als eigener Schritt mit diesem Namen.

---

## M7: Parser und automatische Instrumentierung

**Werkzeugwahl:** tree-sitter (tree-sitter-c) — fehlertoleranter Parser, keine vollstaendige Praeprozessor-Verarbeitung noetig, gut fuer echten Embedded-Code mit Hersteller-Headern.

**Wichtige Erkenntnis:** Die urspruengliche manuelle Instrumentierung aus M2 war eigentlich nur Function Coverage (1 Probe pro Funktion), nicht echte Statement Coverage. Automatisches Tool (`instrument.py`) loest das: findet einzelne Anweisungen, fuegt `cov_mark(id)` rueckwaerts ein (damit sich Positionen nicht verschieben), erzeugt automatisch eine ID→Datei/Zeile-Map.

**Bugfix:** `instrument.py` instrumentierte anfangs auch `cov_dump()`-Aufrufe selbst — ergibt keinen Sinn (man misst nicht die eigene Messung). Ausschlussregel ergaenzt und mit Testfall verifiziert.

**Auf Hardware verifiziert:** automatisch instrumentierter Code lief zuerst nur auf AVR, spaeter auch auf STM32 nachgeholt (gleicher generierter Code, gleiches Ergebnis: 2 von 3 Probes covered, Luecke korrekt erkannt).

---

## M8: Intrusiveness-Messung und Gcov-Vergleich

**Flash/RAM (Baseline vs. instrumentiert, 3 Probes), mit `--gc-sections` bereinigt:**

| | AVR | STM32 |
|---|---|---|
| Flash-Overhead | +128 Bytes | +52 Bytes |
| RAM-Overhead | +8 Bytes | +8 Bytes |

RAM-Overhead ist bei beiden gleich (haengt nur von der Bitmap-Groesse ab). STM32 braucht weniger Flash — liegt am kompakteren Thumb-2-Befehlssatz.

**Zyklen pro cov_mark()-Aufruf:**
- AVR: 54 Zyklen (3,4 µs bei 16MHz), gemessen mit simulavr (zyklengenaue Simulation)
- STM32: 32 Zyklen (2 µs bei 16MHz), gemessen mit dem eingebauten DWT-Zykluszaehler (Hardware-Feature, direkt auf dem Chip)

STM32 ist bei beiden Metriken guenstiger — durchgehendes Muster, kein Zufall.

**Gcov-Vergleich (gleicher Testcode mit `--coverage` kompiliert):**

| | PC | AVR | STM32 |
|---|---|---|---|
| Ergebnis | funktioniert, erzeugt .gcda/.gcno | Linker-Fehler: `undefined reference to __gcov_exit` | identischer Fehler |

Zeigt: das Problem ist nicht "AVR ist zu schwach", sondern strukturell fuer alle Bare-Metal-Targets ohne Dateisystem — bestaetigt die Forschungsluecke aus dem Proposal fuer beide Architekturen, nicht nur eine.

---

## Offene Punkte

- [ ] Grosses Referenzprojekt (60-70+ Funktionen, echtes GitHub-Repo) fuer beide Plattformen
- [ ] Branch Coverage als optionale Erweiterung (M9)
- [ ] Quantitativer Vergleich der vier Terminierungsstrategien (bisher nur einzeln verifiziert, nicht gegeneinander gemessen)
