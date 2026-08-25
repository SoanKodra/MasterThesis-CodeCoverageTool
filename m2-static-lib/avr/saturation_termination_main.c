#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include "uart.h"
#include "../coverage.h"

void cov_dump_uart(void);

volatile uint8_t dump_requested = 0;
volatile uint8_t tick_occurred = 0;
volatile uint8_t already_dumped = 0;

// Gleicher Timer1-Tick-Mechanismus wie bei der Timer-Strategie, aber
// hier nur als periodischer "Pruef-Impuls" genutzt, nicht als direkter
// Dump-Trigger - Saettigungslogik (Proposal Abschnitt 10.2, Strategie 4)
// entscheidet in main(), ob wirklich gedumpt wird.
ISR(TIMER1_COMPA_vect) {
    tick_occurred = 1;
}

void timer1_init_tick(void) {
    TCCR1B |= (1 << WGM12);
    OCR1A = 15624;   // 1 Sekunde bei Vorteiler 1024, 16 MHz
    TIMSK1 |= (1 << OCIE1A);
    TCCR1B |= (1 << CS12) | (1 << CS10);
}

// Einfache Pruefsumme ueber alle aktuell gesetzten Bits - reicht aus,
// um "hat sich etwas veraendert" zu erkennen, ohne die Bitmap-Kapselung
// in coverage.c aufzubrechen.
static unsigned int bitmap_checksum(void) {
    unsigned int sum = 0;
    for (unsigned int id = 0; id < COV_MAX_PROBES; id++) {
        if (cov_is_covered(id)) {
            sum += id + 1;
        }
    }
    return sum;
}

int add(int a, int b) { cov_mark(0); return a + b; }
int subtract(int a, int b) { cov_mark(1); return a - b; }
int unused_function(void) { cov_mark(2); return 0; }

int main(void) {
    uart_init();
    sei();
    timer1_init_tick();

    add(2, 3);
    subtract(5, 1);

    unsigned int last_checksum = 0;
    unsigned int stable_ticks = 0;
    const unsigned int STABLE_THRESHOLD = 3;  // 3 Sekunden ohne Aenderung = Saettigung

    while (1) {
        if (tick_occurred) {
            tick_occurred = 0;

            unsigned int current_checksum = bitmap_checksum();
            if (current_checksum == last_checksum) {
                stable_ticks++;
            } else {
                stable_ticks = 0;
                last_checksum = current_checksum;
            }

            if (stable_ticks >= STABLE_THRESHOLD && !already_dumped) {
                dump_requested = 1;
                already_dumped = 1;  // verhindert erneutes Ausloesen,
                                    // solange die Bitmap stabil bleibt
            }
        }

        if (dump_requested) {
            dump_requested = 0;
            uart_puts("=== coverage dump start (saturation) ===\r\n");
            cov_dump_uart();
            uart_puts("=== coverage dump end ===\r\n");
        }
    }

    return 0;
}
