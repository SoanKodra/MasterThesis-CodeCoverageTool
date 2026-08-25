#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include "uart.h"
#include "../coverage.h"

void cov_dump_uart(void);

volatile uint8_t dump_requested = 0;

// Timer1 Compare-Match-Interrupt: wird automatisch nach fester Zeitspanne
// ausgeloest, kein Host-Eingriff noetig - im Gegensatz zur Host-Kommando-
// Strategie aus M4 (Proposal Abschnitt 10.2, Strategie 2 "Timer").
ISR(TIMER1_COMPA_vect) {
    dump_requested = 1;
}

// Timer1 im CTC-Modus fuer ein 3-Sekunden-Intervall bei 16 MHz.
// Vorteiler 1024 gewaehlt, damit der Vergleichswert in 16 Bit passt:
// (16.000.000 / 1024) * 3 Sekunden = 46875, passt in 16 Bit (max 65535)
void timer1_init_for_termination(void) {
    TCCR1B |= (1 << WGM12);              // CTC-Modus (Clear Timer on Compare)
    OCR1A = 46874;                        // Vergleichswert fuer 3 Sekunden
    TIMSK1 |= (1 << OCIE1A);              // Compare-Match-Interrupt aktivieren
    TCCR1B |= (1 << CS12) | (1 << CS10); // Vorteiler 1024 setzen, Timer startet
}

int add(int a, int b) { cov_mark(0); return a + b; }
int subtract(int a, int b) { cov_mark(1); return a - b; }
int unused_function(void) { cov_mark(2); return 0; }

int main(void) {
    uart_init();
    sei();  // globale Interrupts aktivieren (Pflicht fuer Timer-Interrupt)
    timer1_init_for_termination();

    add(2, 3);
    subtract(5, 1);

    while (1) {
        if (dump_requested) {
            dump_requested = 0;
            uart_puts("=== coverage dump start ===\r\n");
            cov_dump_uart();
            uart_puts("=== coverage dump end ===\r\n");
        }
    }

    return 0;
}
