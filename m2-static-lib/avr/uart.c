// F_CPU = Taktfrequenz des Mega2560 in Hz. Wird für die Baudraten-
// Berechnung gebraucht. Nur setzen, falls nicht schon anderswo definiert
// (z.B. über einen Compiler-Flag -DF_CPU=...).
#ifndef F_CPU
#define F_CPU 16000000UL   // 16 MHz, Standardtakt auf dem AZ-Mega2560-Board
#endif

#include <avr/io.h>  // Definiert die Register-Namen (UBRR0H, UCSR0B, TXEN0, ...)
#include "uart.h"    // Eigener Header mit den Funktionsdeklarationen (lokale Datei -> "" statt <>)

// Ziel-Baudrate für die serielle Verbindung (muss auf Host-Seite,
// z.B. beim Terminal-Programm, exakt gleich eingestellt sein)
#define BAUD 9600

// Formel aus dem ATmega2560-Datenblatt zur Umrechnung Baudrate -> Registerwert.
// Wird vom Präprozessor zur Compile-Zeit berechnet, kostet also zur Laufzeit nichts.
#define UBRR_VALUE (F_CPU / 16 / BAUD - 1)

void uart_init(void) {
    // UBRR (Baud Rate Register) ist 16 Bit breit, aber nur in zwei
    // 8-Bit-Registern ansprechbar: High-Byte und Low-Byte getrennt.
    // ">> 8" verschiebt die oberen 8 Bit einer 16-Bit-Zahl nach unten,
    // damit sie in ein 8-Bit-Register passen.
    UBRR0H = (uint8_t)(UBRR_VALUE >> 8);   // oberes Byte
    UBRR0L = (uint8_t)UBRR_VALUE;          // unteres Byte (Cast schneidet automatisch ab)

    // UCSR0B = Control/Status Register B.
    // "1 << TXEN0" erzeugt eine Zahl mit einer 1 genau an der Bit-Position
    // von TXEN0 (Transmitter ENable), alle anderen Bits sind 0.
    // Direktes Zuweisen (=) setzt hier das gesamte Register neu,
    // das ist ok, weil wir gerade initialisieren (keine anderen Bits vorher gesetzt).
    UCSR0B = (1 << TXEN0);   // Sender aktivieren

    // UCSR0C = Control/Status Register C, legt das Datenformat fest.
    // UCSZ01 + UCSZ00 zusammen auf 1 = "8 Datenbits" (Tabelle im Datenblatt).
    // "|" (bitweises ODER) kombiniert zwei einzelne Bit-Muster in einem Wert.
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);   // 8 Datenbits, kein Paritätsbit, 1 Stopbit (8N1)
}

void uart_putchar(char c) {
    // UCSR0A = Status-Register. UDRE0-Bit zeigt an: "Sende-Puffer (UDR0) ist leer/bereit".
    // "UCSR0A & (1 << UDRE0)" blendet per UND (&) alle anderen Bits aus,
    // übrig bleibt nur der Zustand von UDRE0.
    // "!" kehrt die Bedingung um: solange NICHT bereit -> warten.
    while (!(UCSR0A & (1 << UDRE0))) {
        ;  // aktives Warten (Polling), Prozessor tut hier bewusst nichts anderes
    }

    // Sobald bereit: Byte in das Datenregister schreiben.
    // Die UART-Hardware übernimmt das serielle Rausschieben automatisch,
    // ohne dass wir uns um Timing pro Bit kümmern müssen.
    UDR0 = c;
}

void uart_puts(const char *s) {
    // "*s" liest das Zeichen, auf das der Pointer s aktuell zeigt.
    // C-Strings enden immer mit einem Nullbyte ('\0'), das in einer
    // Bedingung automatisch als "falsch" gilt -> Schleife stoppt von selbst am Stringende.
    while (*s) {
        uart_putchar(*s);  // aktuelles Zeichen senden
        s++;               // Pointer einen Schritt weiterrücken (nächstes Zeichen)
    }
}
