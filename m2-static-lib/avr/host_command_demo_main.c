#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include "uart.h"
#include "../coverage.h"

void cov_dump_uart(void);

// "volatile" ist Pflicht bei Variablen, die eine ISR verändert und
// main() liest - ohne volatile könnte der Compiler fälschlich annehmen,
// der Wert ändert sich nie, und das Lesen wegoptimieren.
volatile uint8_t dump_requested = 0;

// ISR-Name folgt einem festen Muster aus avr/interrupt.h:
// USART0_RX_vect = "USART0 Receive Complete" Interrupt-Vektor

ISR(USART0_RX_vect) {
    // Empfangenes Byte muss so oder so gelesen werden (Pflicht,
    // sonst löst der Interrupt erneut aus) - jetzt aber ausgewertet
    // statt verworfen: nur 'D' löst den Dump aus, alles andere wird ignoriert.
    uint8_t received = UDR0;
    if (received == 'D') {
        dump_requested = 1;
    }
}

int add(int a, int b) { cov_mark(0); return a + b; }
int subtract(int a, int b) { cov_mark(1); return a - b; }
void unused_function(void) { cov_mark(2); }

int main(void) {
    uart_init();
    uart_enable_rx_interrupt();

    add(3, 4);
    subtract(10, 4);
    cov_mark(3);

    while (1) {
        if (dump_requested) {
            dump_requested = 0;
            uart_puts("=== coverage dump start ===\r\n");
            cov_dump_uart();
            uart_puts("=== coverage dump end ===\r\n");
        }
        // sonst: normale Endlosschleife, Zielcode würde hier weiterlaufen
    }

    return 0;
}
